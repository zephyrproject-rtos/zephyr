/**
  ******************************************************************************
  * @file    stm32l051xx.h
  * @author  MCD Application Team
  * @version V1.7.0
  * @date    31-May-2016
  * @brief   CMSIS Cortex-M0+ Device Peripheral Access Layer Header File. 
  *          This file contains all the peripheral register's definitions, bits 
  *          definitions and memory mapping for stm32l051xx devices.  
  *          
  *          This file contains:
  *           - Data structures and the address mapping for all peripherals
  *           - Peripheral's registers declarations and bits definition
  *           - Macros to access peripheral's registers hardware
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2015 STMicroelectronics</center></h2>
  *
  * Redistribution and use in source and binary forms, with or without modification,
  * are permitted provided that the following conditions are met:
  *   1. Redistributions of source code must retain the above copyright notice,
  *      this list of conditions and the following disclaimer.
  *   2. Redistributions in binary form must reproduce the above copyright notice,
  *      this list of conditions and the following disclaimer in the documentation
  *      and/or other materials provided with the distribution.
  *   3. Neither the name of STMicroelectronics nor the names of its contributors
  *      may be used to endorse or promote products derived from this software
  *      without specific prior written permission.
  *
  * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
  * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
  * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
  * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
  * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
  * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
  * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
  * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
  * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
  * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32l051xx
  * @{
  */
    
#ifndef __STM32L051xx_H
#define __STM32L051xx_H

#ifdef __cplusplus
 extern "C" {
#endif 
  

/** @addtogroup Configuration_section_for_CMSIS
  * @{
  */
/**
  * @brief Configuration of the Cortex-M0+ Processor and Core Peripherals 
  */
#define __CM0PLUS_REV             0 /*!< Core Revision r0p0                            */
#define __MPU_PRESENT             1 /*!< STM32L0xx  provides an MPU                    */
#define __VTOR_PRESENT            1 /*!< Vector  Table  Register supported             */
#define __NVIC_PRIO_BITS          2 /*!< STM32L0xx uses 2 Bits for the Priority Levels */
#define __Vendor_SysTickConfig    0 /*!< Set to 1 if different SysTick Config is used  */

/**
  * @}
  */
   
/** @addtogroup Peripheral_interrupt_number_definition
  * @{
  */
   
/**
 * @brief stm32l051xx Interrupt Number Definition, according to the selected device 
 *        in @ref Library_configuration_section 
 */

/*!< Interrupt Number Definition */
typedef enum
{
/******  Cortex-M0 Processor Exceptions Numbers ******************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                */
  HardFault_IRQn              = -13,    /*!< 3 Cortex-M0+ Hard Fault Interrupt                       */
  SVC_IRQn                    = -5,     /*!< 11 Cortex-M0+ SV Call Interrupt                         */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M0+ Pend SV Interrupt                         */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M0+ System Tick Interrupt                     */

/******  STM32L-0 specific Interrupt Numbers *********************************************************/
  WWDG_IRQn                   = 0,      /*!< Window WatchDog Interrupt                               */
  PVD_IRQn                    = 1,      /*!< PVD through EXTI Line detect Interrupt                  */
  RTC_IRQn                    = 2,      /*!< RTC through EXTI Line Interrupt                         */
  FLASH_IRQn                  = 3,      /*!< FLASH Interrupt                                         */
  RCC_IRQn                    = 4,      /*!< RCC Interrupt                                           */
  EXTI0_1_IRQn                = 5,      /*!< EXTI Line 0 and 1 Interrupts                            */
  EXTI2_3_IRQn                = 6,      /*!< EXTI Line 2 and 3 Interrupts                            */
  EXTI4_15_IRQn               = 7,      /*!< EXTI Line 4 to 15 Interrupts                            */
  DMA1_Channel1_IRQn          = 9,      /*!< DMA1 Channel 1 Interrupt                                */
  DMA1_Channel2_3_IRQn        = 10,     /*!< DMA1 Channel 2 and Channel 3 Interrupts                 */
  DMA1_Channel4_5_6_7_IRQn    = 11,     /*!< DMA1 Channel 4, Channel 5, Channel 6 and Channel 7 Interrupts */
  ADC1_COMP_IRQn              = 12,     /*!< ADC1, COMP1 and COMP2 Interrupts                        */
  LPTIM1_IRQn                 = 13,     /*!< LPTIM1 Interrupt                                        */
  TIM2_IRQn                   = 15,     /*!< TIM2 Interrupt                                          */
  TIM6_IRQn                   = 17,     /*!< TIM6  Interrupt                                         */
  TIM21_IRQn                  = 20,     /*!< TIM21 Interrupt                                         */
  TIM22_IRQn                  = 22,     /*!< TIM22 Interrupt                                         */
  I2C1_IRQn                   = 23,     /*!< I2C1 Interrupt                                          */
  I2C2_IRQn                   = 24,     /*!< I2C2 Interrupt                                          */
  SPI1_IRQn                   = 25,     /*!< SPI1 Interrupt                                          */
  SPI2_IRQn                   = 26,     /*!< SPI2 Interrupt                                          */
  USART1_IRQn                 = 27,     /*!< USART1 Interrupt                                        */
  USART2_IRQn                 = 28,     /*!< USART2 Interrupt                                        */
  LPUART1_IRQn                = 29,     /*!< LPUART1 Interrupt                                       */
} IRQn_Type;

/**
  * @}
  */

#include "core_cm0plus.h"
#include "system_stm32l0xx.h"
#include <stdint.h>

/** @addtogroup Peripheral_registers_structures
  * @{
  */   

/** 
  * @brief Analog to Digital Converter  
  */

typedef struct
{
  __IO uint32_t ISR;          /*!< ADC Interrupt and Status register,                          Address offset:0x00 */
  __IO uint32_t IER;          /*!< ADC Interrupt Enable register,                              Address offset:0x04 */
  __IO uint32_t CR;           /*!< ADC Control register,                                       Address offset:0x08 */
  __IO uint32_t CFGR1;        /*!< ADC Configuration register 1,                               Address offset:0x0C */
  __IO uint32_t CFGR2;        /*!< ADC Configuration register 2,                               Address offset:0x10 */
  __IO uint32_t SMPR;         /*!< ADC Sampling time register,                                 Address offset:0x14 */
  uint32_t   RESERVED1;       /*!< Reserved,                                                                  0x18 */
  uint32_t   RESERVED2;       /*!< Reserved,                                                                  0x1C */
  __IO uint32_t TR;           /*!< ADC watchdog threshold register,                            Address offset:0x20 */
  uint32_t   RESERVED3;       /*!< Reserved,                                                                  0x24 */
  __IO uint32_t CHSELR;       /*!< ADC channel selection register,                             Address offset:0x28 */
  uint32_t   RESERVED4[5];    /*!< Reserved,                                                                  0x2C */
  __IO uint32_t DR;           /*!< ADC data register,                                          Address offset:0x40 */
  uint32_t   RESERVED5[28];   /*!< Reserved,                                                           0x44 - 0xB0 */
  __IO uint32_t CALFACT;      /*!< ADC data register,                                          Address offset:0xB4 */
} ADC_TypeDef;

typedef struct
{
  __IO uint32_t CCR;
} ADC_Common_TypeDef;


/**
  * @brief Comparator 
  */

typedef struct
{
  __IO uint32_t CSR;     /*!< COMP comparator control and status register, Address offset: 0x18 */
} COMP_TypeDef;

typedef struct
{
  __IO uint32_t CSR;         /*!< COMP control and status register, used for bits common to several COMP instances, Address offset: 0x00 */
} COMP_Common_TypeDef;


/**
* @brief CRC calculation unit
*/

typedef struct
{
__IO uint32_t DR;            /*!< CRC Data register,                            Address offset: 0x00 */
__IO uint8_t IDR;            /*!< CRC Independent data register,                Address offset: 0x04 */
uint8_t RESERVED0;           /*!< Reserved,                                                     0x05 */
uint16_t RESERVED1;          /*!< Reserved,                                                     0x06 */
__IO uint32_t CR;            /*!< CRC Control register,                         Address offset: 0x08 */
uint32_t RESERVED2;          /*!< Reserved,                                                     0x0C */
__IO uint32_t INIT;          /*!< Initial CRC value register,                   Address offset: 0x10 */
__IO uint32_t POL;           /*!< CRC polynomial register,                      Address offset: 0x14 */
} CRC_TypeDef;

/** 
  * @brief Debug MCU
  */

typedef struct
{
  __IO uint32_t IDCODE;       /*!< MCU device ID code,                          Address offset: 0x00 */
  __IO uint32_t CR;           /*!< Debug MCU configuration register,            Address offset: 0x04 */
  __IO uint32_t APB1FZ;       /*!< Debug MCU APB1 freeze register,              Address offset: 0x08 */
  __IO uint32_t APB2FZ;       /*!< Debug MCU APB2 freeze register,              Address offset: 0x0C */
}DBGMCU_TypeDef;

/** 
  * @brief DMA Controller
  */

typedef struct
{
  __IO uint32_t CCR;          /*!< DMA channel x configuration register */
  __IO uint32_t CNDTR;        /*!< DMA channel x number of data register */
  __IO uint32_t CPAR;         /*!< DMA channel x peripheral address register */
  __IO uint32_t CMAR;         /*!< DMA channel x memory address register */
} DMA_Channel_TypeDef;

typedef struct
{
  __IO uint32_t ISR;          /*!< DMA interrupt status register,               Address offset: 0x00 */
  __IO uint32_t IFCR;         /*!< DMA interrupt flag clear register,           Address offset: 0x04 */
} DMA_TypeDef;                                                                  
                                                                                
typedef struct                                                                  
{                                                                               
  __IO uint32_t CSELR;        /*!< DMA channel selection register,              Address offset: 0xA8 */
} DMA_Request_TypeDef;                                                          
                                                                                
/**                                                                             
  * @brief External Interrupt/Event Controller                                  
  */                                                                            
                                                                                
typedef struct                                                                  
{                                                                               
  __IO uint32_t IMR;          /*!<EXTI Interrupt mask register,                 Address offset: 0x00 */
  __IO uint32_t EMR;          /*!<EXTI Event mask register,                     Address offset: 0x04 */
  __IO uint32_t RTSR;         /*!<EXTI Rising trigger selection register ,      Address offset: 0x08 */
  __IO uint32_t FTSR;         /*!<EXTI Falling trigger selection register,      Address offset: 0x0C */
  __IO uint32_t SWIER;        /*!<EXTI Software interrupt event register,       Address offset: 0x10 */
  __IO uint32_t PR;           /*!<EXTI Pending register,                        Address offset: 0x14 */
}EXTI_TypeDef;

/** 
  * @brief FLASH Registers
  */
typedef struct
{
  __IO uint32_t ACR;           /*!< Access control register,                     Address offset: 0x00 */
  __IO uint32_t PECR;          /*!< Program/erase control register,              Address offset: 0x04 */
  __IO uint32_t PDKEYR;        /*!< Power down key register,                     Address offset: 0x08 */
  __IO uint32_t PEKEYR;        /*!< Program/erase key register,                  Address offset: 0x0c */
  __IO uint32_t PRGKEYR;       /*!< Program memory key register,                 Address offset: 0x10 */
  __IO uint32_t OPTKEYR;       /*!< Option byte key register,                    Address offset: 0x14 */
  __IO uint32_t SR;            /*!< Status register,                             Address offset: 0x18 */
  __IO uint32_t OPTR;          /*!< Option byte register,                        Address offset: 0x1c */
  __IO uint32_t WRPR;          /*!< Write protection register,                   Address offset: 0x20 */
} FLASH_TypeDef;


/** 
  * @brief Option Bytes Registers
  */
typedef struct
{
  __IO uint32_t RDP;               /*!< Read protection register,               Address offset: 0x00 */
  __IO uint32_t USER;              /*!< user register,                          Address offset: 0x04 */
  __IO uint32_t WRP01;             /*!< write protection Bytes 0 and 1          Address offset: 0x08 */
} OB_TypeDef;
  

/** 
  * @brief General Purpose IO
  */

typedef struct
{
  __IO uint32_t MODER;        /*!< GPIO port mode register,                     Address offset: 0x00 */
  __IO uint32_t OTYPER;       /*!< GPIO port output type register,              Address offset: 0x04 */
  __IO uint32_t OSPEEDR;      /*!< GPIO port output speed register,             Address offset: 0x08 */
  __IO uint32_t PUPDR;        /*!< GPIO port pull-up/pull-down register,        Address offset: 0x0C */
  __IO uint32_t IDR;          /*!< GPIO port input data register,               Address offset: 0x10 */
  __IO uint32_t ODR;          /*!< GPIO port output data register,              Address offset: 0x14 */
  __IO uint32_t BSRR;         /*!< GPIO port bit set/reset registerBSRR,        Address offset: 0x18 */
  __IO uint32_t LCKR;         /*!< GPIO port configuration lock register,       Address offset: 0x1C */
  __IO uint32_t AFR[2];       /*!< GPIO alternate function register,            Address offset: 0x20-0x24 */
  __IO uint32_t BRR;          /*!< GPIO bit reset register,                     Address offset: 0x28 */
}GPIO_TypeDef;

/** 
  * @brief LPTIMIMER
  */
typedef struct
{
  __IO uint32_t ISR;      /*!< LPTIM Interrupt and Status register,             Address offset: 0x00 */
  __IO uint32_t ICR;      /*!< LPTIM Interrupt Clear register,                  Address offset: 0x04 */
  __IO uint32_t IER;      /*!< LPTIM Interrupt Enable register,                 Address offset: 0x08 */
  __IO uint32_t CFGR;     /*!< LPTIM Configuration register,                    Address offset: 0x0C */
  __IO uint32_t CR;       /*!< LPTIM Control register,                          Address offset: 0x10 */
  __IO uint32_t CMP;      /*!< LPTIM Compare register,                          Address offset: 0x14 */
  __IO uint32_t ARR;      /*!< LPTIM Autoreload register,                       Address offset: 0x18 */
  __IO uint32_t CNT;      /*!< LPTIM Counter register,                          Address offset: 0x1C */
} LPTIM_TypeDef;

/** 
  * @brief SysTem Configuration
  */

typedef struct
{
  __IO uint32_t CFGR1;         /*!< SYSCFG configuration register 1,                    Address offset: 0x00 */
  __IO uint32_t CFGR2;         /*!< SYSCFG configuration register 2,                    Address offset: 0x04 */
  __IO uint32_t EXTICR[4];     /*!< SYSCFG external interrupt configuration register,   Address offset: 0x14-0x08 */
       uint32_t RESERVED[2];   /*!< Reserved,                                           0x18-0x1C */
  __IO uint32_t CFGR3;         /*!< SYSCFG configuration register 3,                    Address offset: 0x20 */       
} SYSCFG_TypeDef;



/** 
  * @brief Inter-integrated Circuit Interface
  */

typedef struct
{
  __IO uint32_t CR1;      /*!< I2C Control register 1,            Address offset: 0x00 */
  __IO uint32_t CR2;      /*!< I2C Control register 2,            Address offset: 0x04 */
  __IO uint32_t OAR1;     /*!< I2C Own address 1 register,        Address offset: 0x08 */
  __IO uint32_t OAR2;     /*!< I2C Own address 2 register,        Address offset: 0x0C */
  __IO uint32_t TIMINGR;  /*!< I2C Timing register,               Address offset: 0x10 */
  __IO uint32_t TIMEOUTR; /*!< I2C Timeout register,              Address offset: 0x14 */
  __IO uint32_t ISR;      /*!< I2C Interrupt and status register, Address offset: 0x18 */
  __IO uint32_t ICR;      /*!< I2C Interrupt clear register,      Address offset: 0x1C */
  __IO uint32_t PECR;     /*!< I2C PEC register,                  Address offset: 0x20 */
  __IO uint32_t RXDR;     /*!< I2C Receive data register,         Address offset: 0x24 */
  __IO uint32_t TXDR;     /*!< I2C Transmit data register,        Address offset: 0x28 */
}I2C_TypeDef;


/** 
  * @brief Independent WATCHDOG
  */
typedef struct
{
  __IO uint32_t KR;   /*!< IWDG Key register,       Address offset: 0x00 */
  __IO uint32_t PR;   /*!< IWDG Prescaler register, Address offset: 0x04 */
  __IO uint32_t RLR;  /*!< IWDG Reload register,    Address offset: 0x08 */
  __IO uint32_t SR;   /*!< IWDG Status register,    Address offset: 0x0C */
  __IO uint32_t WINR; /*!< IWDG Window register,    Address offset: 0x10 */
} IWDG_TypeDef;

/** 
  * @brief MIFARE Firewall
  */
typedef struct
{
  __IO uint32_t CSSA;     /*!< Code Segment Start Address register,               Address offset: 0x00 */
  __IO uint32_t CSL;      /*!< Code Segment Length register,                      Address offset: 0x04 */
  __IO uint32_t NVDSSA;   /*!< NON volatile data Segment Start Address register,  Address offset: 0x08 */
  __IO uint32_t NVDSL;    /*!< NON volatile data Segment Length register,         Address offset: 0x0C */
  __IO uint32_t VDSSA ;   /*!< Volatile data Segment Start Address register,      Address offset: 0x10 */
  __IO uint32_t VDSL ;    /*!< Volatile data Segment Length register,             Address offset: 0x14 */
  __IO uint32_t LSSA ;    /*!< Library Segment Start Address register,            Address offset: 0x18 */
  __IO uint32_t LSL ;     /*!< Library Segment Length register,                   Address offset: 0x1C */
  __IO uint32_t CR ;      /*!< Configuration  register,                           Address offset: 0x20 */
 
} FIREWALL_TypeDef;

/** 
  * @brief Power Control
  */
typedef struct
{
  __IO uint32_t CR;   /*!< PWR power control register,        Address offset: 0x00 */
  __IO uint32_t CSR;  /*!< PWR power control/status register, Address offset: 0x04 */
} PWR_TypeDef;

/** 
  * @brief Reset and Clock Control
  */
typedef struct
{
  __IO uint32_t CR;            /*!< RCC clock control register,                                   Address offset: 0x00 */
  __IO uint32_t ICSCR;         /*!< RCC Internal clock sources calibration register,              Address offset: 0x04 */
  __IO uint32_t CRRCR;         /*!< RCC Clock recovery RC register,                               Address offset: 0x08 */
  __IO uint32_t CFGR;          /*!< RCC Clock configuration register,                             Address offset: 0x0C */
  __IO uint32_t CIER;          /*!< RCC Clock interrupt enable register,                          Address offset: 0x10 */
  __IO uint32_t CIFR;          /*!< RCC Clock interrupt flag register,                            Address offset: 0x14 */
  __IO uint32_t CICR;          /*!< RCC Clock interrupt clear register,                           Address offset: 0x18 */
  __IO uint32_t IOPRSTR;       /*!< RCC IO port reset register,                                   Address offset: 0x1C */
  __IO uint32_t AHBRSTR;       /*!< RCC AHB peripheral reset register,                            Address offset: 0x20 */
  __IO uint32_t APB2RSTR;      /*!< RCC APB2 peripheral reset register,                           Address offset: 0x24 */
  __IO uint32_t APB1RSTR;      /*!< RCC APB1 peripheral reset register,                           Address offset: 0x28 */
  __IO uint32_t IOPENR;        /*!< RCC Clock IO port enable register,                            Address offset: 0x2C */
  __IO uint32_t AHBENR;        /*!< RCC AHB peripheral clock enable register,                     Address offset: 0x30 */
  __IO uint32_t APB2ENR;       /*!< RCC APB2 peripheral enable register,                          Address offset: 0x34 */
  __IO uint32_t APB1ENR;       /*!< RCC APB1 peripheral enable register,                          Address offset: 0x38 */
  __IO uint32_t IOPSMENR;      /*!< RCC IO port clock enable in sleep mode register,              Address offset: 0x3C */
  __IO uint32_t AHBSMENR;      /*!< RCC AHB peripheral clock enable in sleep mode register,       Address offset: 0x40 */
  __IO uint32_t APB2SMENR;     /*!< RCC APB2 peripheral clock enable in sleep mode register,      Address offset: 0x44 */
  __IO uint32_t APB1SMENR;     /*!< RCC APB1 peripheral clock enable in sleep mode register,      Address offset: 0x48 */
  __IO uint32_t CCIPR;         /*!< RCC clock configuration register,                             Address offset: 0x4C */
  __IO uint32_t CSR;           /*!< RCC Control/status register,                                  Address offset: 0x50 */
} RCC_TypeDef;

/** 
  * @brief Real-Time Clock
  */
typedef struct
{
  __IO uint32_t TR;         /*!< RTC time register,                                         Address offset: 0x00 */
  __IO uint32_t DR;         /*!< RTC date register,                                         Address offset: 0x04 */
  __IO uint32_t CR;         /*!< RTC control register,                                      Address offset: 0x08 */
  __IO uint32_t ISR;        /*!< RTC initialization and status register,                    Address offset: 0x0C */
  __IO uint32_t PRER;       /*!< RTC prescaler register,                                    Address offset: 0x10 */
  __IO uint32_t WUTR;       /*!< RTC wakeup timer register,                                 Address offset: 0x14 */
       uint32_t RESERVED;   /*!< Reserved,                                                  Address offset: 0x18 */
  __IO uint32_t ALRMAR;     /*!< RTC alarm A register,                                      Address offset: 0x1C */
  __IO uint32_t ALRMBR;     /*!< RTC alarm B register,                                      Address offset: 0x20 */
  __IO uint32_t WPR;        /*!< RTC write protection register,                             Address offset: 0x24 */
  __IO uint32_t SSR;        /*!< RTC sub second register,                                   Address offset: 0x28 */
  __IO uint32_t SHIFTR;     /*!< RTC shift control register,                                Address offset: 0x2C */
  __IO uint32_t TSTR;       /*!< RTC time stamp time register,                              Address offset: 0x30 */
  __IO uint32_t TSDR;       /*!< RTC time stamp date register,                              Address offset: 0x34 */
  __IO uint32_t TSSSR;      /*!< RTC time-stamp sub second register,                        Address offset: 0x38 */
  __IO uint32_t CALR;       /*!< RTC calibration register,                                  Address offset: 0x3C */
  __IO uint32_t TAMPCR;     /*!< RTC tamper configuration register,                         Address offset: 0x40 */
  __IO uint32_t ALRMASSR;   /*!< RTC alarm A sub second register,                           Address offset: 0x44 */
  __IO uint32_t ALRMBSSR;   /*!< RTC alarm B sub second register,                           Address offset: 0x48 */
  __IO uint32_t OR;         /*!< RTC option register,                                       Address offset  0x4C */
  __IO uint32_t BKP0R;      /*!< RTC backup register 0,                                     Address offset: 0x50 */
  __IO uint32_t BKP1R;      /*!< RTC backup register 1,                                     Address offset: 0x54 */
  __IO uint32_t BKP2R;      /*!< RTC backup register 2,                                     Address offset: 0x58 */
  __IO uint32_t BKP3R;      /*!< RTC backup register 3,                                     Address offset: 0x5C */
  __IO uint32_t BKP4R;      /*!< RTC backup register 4,                                     Address offset: 0x60 */
} RTC_TypeDef;


/** 
  * @brief Serial Peripheral Interface
  */
typedef struct
{
  __IO uint32_t CR1;      /*!< SPI Control register 1 (not used in I2S mode),       Address offset: 0x00 */
  __IO uint32_t CR2;      /*!< SPI Control register 2,                              Address offset: 0x04 */
  __IO uint32_t SR;       /*!< SPI Status register,                                 Address offset: 0x08 */
  __IO uint32_t DR;       /*!< SPI data register,                                   Address offset: 0x0C */
  __IO uint32_t CRCPR;    /*!< SPI CRC polynomial register (not used in I2S mode),  Address offset: 0x10 */
  __IO uint32_t RXCRCR;   /*!< SPI Rx CRC register (not used in I2S mode),          Address offset: 0x14 */
  __IO uint32_t TXCRCR;   /*!< SPI Tx CRC register (not used in I2S mode),          Address offset: 0x18 */
  __IO uint32_t I2SCFGR;  /*!< SPI_I2S configuration register,                      Address offset: 0x1C */
  __IO uint32_t I2SPR;    /*!< SPI_I2S prescaler register,                          Address offset: 0x20 */
} SPI_TypeDef;

/** 
  * @brief TIM
  */
typedef struct
{
  __IO uint32_t CR1;       /*!< TIM control register 1,                       Address offset: 0x00 */
  __IO uint32_t CR2;       /*!< TIM control register 2,                       Address offset: 0x04 */
  __IO uint32_t SMCR;      /*!< TIM slave Mode Control register,              Address offset: 0x08 */
  __IO uint32_t DIER;      /*!< TIM DMA/interrupt enable register,            Address offset: 0x0C */
  __IO uint32_t SR;        /*!< TIM status register,                          Address offset: 0x10 */
  __IO uint32_t EGR;       /*!< TIM event generation register,                Address offset: 0x14 */
  __IO uint32_t CCMR1;     /*!< TIM  capture/compare mode register 1,         Address offset: 0x18 */
  __IO uint32_t CCMR2;     /*!< TIM  capture/compare mode register 2,         Address offset: 0x1C */
  __IO uint32_t CCER;      /*!< TIM capture/compare enable register,          Address offset: 0x20 */
  __IO uint32_t CNT;       /*!< TIM counter register,                         Address offset: 0x24 */
  __IO uint32_t PSC;       /*!< TIM prescaler register,                       Address offset: 0x28 */
  __IO uint32_t ARR;       /*!< TIM auto-reload register,                     Address offset: 0x2C */
  uint32_t      RESERVED12;/*!< Reserved                                      Address offset: 0x30 */
  __IO uint32_t CCR1;      /*!< TIM capture/compare register 1,               Address offset: 0x34 */
  __IO uint32_t CCR2;      /*!< TIM capture/compare register 2,               Address offset: 0x38 */
  __IO uint32_t CCR3;      /*!< TIM capture/compare register 3,               Address offset: 0x3C */
  __IO uint32_t CCR4;      /*!< TIM capture/compare register 4,               Address offset: 0x40 */
  uint32_t      RESERVED17;/*!< Reserved,                                     Address offset: 0x44 */
  __IO uint32_t DCR;       /*!< TIM DMA control register,                     Address offset: 0x48 */
  __IO uint32_t DMAR;      /*!< TIM DMA address for full transfer register,   Address offset: 0x4C */
  __IO uint32_t OR;        /*!< TIM option register,                          Address offset: 0x50 */
} TIM_TypeDef;

/** 
  * @brief Universal Synchronous Asynchronous Receiver Transmitter
  */
typedef struct
{
  __IO uint32_t CR1;    /*!< USART Control register 1,                 Address offset: 0x00 */ 
  __IO uint32_t CR2;    /*!< USART Control register 2,                 Address offset: 0x04 */ 
  __IO uint32_t CR3;    /*!< USART Control register 3,                 Address offset: 0x08 */
  __IO uint32_t BRR;    /*!< USART Baud rate register,                 Address offset: 0x0C */  
  __IO uint32_t GTPR;   /*!< USART Guard time and prescaler register,  Address offset: 0x10 */
  __IO uint32_t RTOR;   /*!< USART Receiver Time Out register,         Address offset: 0x14 */  
  __IO uint32_t RQR;    /*!< USART Request register,                   Address offset: 0x18 */
  __IO uint32_t ISR;    /*!< USART Interrupt and status register,      Address offset: 0x1C */
  __IO uint32_t ICR;    /*!< USART Interrupt flag Clear register,      Address offset: 0x20 */
  __IO uint32_t RDR;    /*!< USART Receive Data register,              Address offset: 0x24 */
  __IO uint32_t TDR;    /*!< USART Transmit Data register,             Address offset: 0x28 */
} USART_TypeDef;

/** 
  * @brief Window WATCHDOG
  */
typedef struct
{
  __IO uint32_t CR;   /*!< WWDG Control register,       Address offset: 0x00 */
  __IO uint32_t CFR;  /*!< WWDG Configuration register, Address offset: 0x04 */
  __IO uint32_t SR;   /*!< WWDG Status register,        Address offset: 0x08 */
} WWDG_TypeDef;


/**
  * @}
  */
  
/** @addtogroup Peripheral_memory_map
  * @{
  */
#define FLASH_BASE             ((uint32_t)0x08000000U) /*!< FLASH base address in the alias region */
#define FLASH_END              ((uint32_t)0x0800FFFFU) /*!< FLASH end address in the alias region */
#define DATA_EEPROM_BASE       ((uint32_t)0x08080000U) /*!< DATA_EEPROM base address in the alias region */
#define DATA_EEPROM_END        ((uint32_t)0x080807FFU) /*!< DATA EEPROM end address in the alias region */
#define SRAM_BASE              ((uint32_t)0x20000000U) /*!< SRAM base address in the alias region */
#define SRAM_SIZE_MAX          ((uint32_t)0x00002000U) /*!< maximum SRAM size (up to 8KBytes) */

#define PERIPH_BASE            ((uint32_t)0x40000000U) /*!< Peripheral base address in the alias region */

/*!< Peripheral memory map */
#define APBPERIPH_BASE        PERIPH_BASE
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000U)
#define IOPPERIPH_BASE        (PERIPH_BASE + 0x10000000U)

#define TIM2_BASE             (APBPERIPH_BASE + 0x00000000U)
#define TIM6_BASE             (APBPERIPH_BASE + 0x00001000U)
#define RTC_BASE              (APBPERIPH_BASE + 0x00002800U)
#define WWDG_BASE             (APBPERIPH_BASE + 0x00002C00U)
#define IWDG_BASE             (APBPERIPH_BASE + 0x00003000U)
#define SPI2_BASE             (APBPERIPH_BASE + 0x00003800U)
#define USART2_BASE           (APBPERIPH_BASE + 0x00004400U)
#define LPUART1_BASE          (APBPERIPH_BASE + 0x00004800U)
#define I2C1_BASE             (APBPERIPH_BASE + 0x00005400U)
#define I2C2_BASE             (APBPERIPH_BASE + 0x00005800U)
#define PWR_BASE              (APBPERIPH_BASE + 0x00007000U)
#define LPTIM1_BASE           (APBPERIPH_BASE + 0x00007C00U)

#define SYSCFG_BASE           (APBPERIPH_BASE + 0x00010000U)
#define COMP1_BASE            (APBPERIPH_BASE + 0x00010018U)
#define COMP2_BASE            (APBPERIPH_BASE + 0x0001001CU)
#define COMP12_COMMON       ((COMP_Common_TypeDef *) COMP1_BASE)
#define EXTI_BASE             (APBPERIPH_BASE + 0x00010400U)
#define TIM21_BASE            (APBPERIPH_BASE + 0x00010800U)
#define TIM22_BASE            (APBPERIPH_BASE + 0x00011400U)
#define FIREWALL_BASE         (APBPERIPH_BASE + 0x00011C00U)
#define ADC1_BASE             (APBPERIPH_BASE + 0x00012400U)
#define ADC_BASE              (APBPERIPH_BASE + 0x00012708U)
#define SPI1_BASE             (APBPERIPH_BASE + 0x00013000U)
#define USART1_BASE           (APBPERIPH_BASE + 0x00013800U)
#define DBGMCU_BASE           (APBPERIPH_BASE + 0x00015800U)

#define DMA1_BASE             (AHBPERIPH_BASE + 0x00000000U)
#define DMA1_Channel1_BASE    (DMA1_BASE + 0x00000008U)
#define DMA1_Channel2_BASE    (DMA1_BASE + 0x0000001CU)
#define DMA1_Channel3_BASE    (DMA1_BASE + 0x00000030U)
#define DMA1_Channel4_BASE    (DMA1_BASE + 0x00000044U)
#define DMA1_Channel5_BASE    (DMA1_BASE + 0x00000058U)
#define DMA1_Channel6_BASE    (DMA1_BASE + 0x0000006CU)
#define DMA1_Channel7_BASE    (DMA1_BASE + 0x00000080U)
#define DMA1_CSELR_BASE       (DMA1_BASE + 0x000000A8U)


#define RCC_BASE              (AHBPERIPH_BASE + 0x00001000U)
#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x00002000U) /*!< FLASH registers base address */
#define OB_BASE               ((uint32_t)0x1FF80000U)        /*!< FLASH Option Bytes base address */
#define FLASHSIZE_BASE        ((uint32_t)0x1FF8007CU)        /*!< FLASH Size register base address */
#define UID_BASE              ((uint32_t)0x1FF80050U)        /*!< Unique device ID register base address  */
#define CRC_BASE              (AHBPERIPH_BASE + 0x00003000U)

#define GPIOA_BASE            (IOPPERIPH_BASE + 0x00000000U)
#define GPIOB_BASE            (IOPPERIPH_BASE + 0x00000400U)
#define GPIOC_BASE            (IOPPERIPH_BASE + 0x00000800U)
#define GPIOD_BASE            (IOPPERIPH_BASE + 0x00000C00U)
#define GPIOH_BASE            (IOPPERIPH_BASE + 0x00001C00U)

/**
  * @}
  */
  
/** @addtogroup Peripheral_declaration
  * @{
  */  

#define TIM2                ((TIM_TypeDef *) TIM2_BASE)
#define TIM6                ((TIM_TypeDef *) TIM6_BASE)
#define RTC                 ((RTC_TypeDef *) RTC_BASE)
#define WWDG                ((WWDG_TypeDef *) WWDG_BASE)
#define IWDG                ((IWDG_TypeDef *) IWDG_BASE)
#define SPI2                ((SPI_TypeDef *) SPI2_BASE)
#define USART2              ((USART_TypeDef *) USART2_BASE)
#define LPUART1             ((USART_TypeDef *) LPUART1_BASE)
#define I2C1                ((I2C_TypeDef *) I2C1_BASE)
#define I2C2                ((I2C_TypeDef *) I2C2_BASE)
#define PWR                 ((PWR_TypeDef *) PWR_BASE)
#define LPTIM1              ((LPTIM_TypeDef *) LPTIM1_BASE)

#define SYSCFG              ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define COMP1               ((COMP_TypeDef *) COMP1_BASE)
#define COMP2               ((COMP_TypeDef *) COMP2_BASE)
#define EXTI                ((EXTI_TypeDef *) EXTI_BASE)
#define TIM21               ((TIM_TypeDef *) TIM21_BASE)
#define TIM22               ((TIM_TypeDef *) TIM22_BASE)
#define FIREWALL            ((FIREWALL_TypeDef *) FIREWALL_BASE)
#define ADC1                ((ADC_TypeDef *) ADC1_BASE)
#define ADC1_COMMON         ((ADC_Common_TypeDef *) ADC_BASE)
/* Legacy defines */
#define ADC                 ADC1_COMMON
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)
#define USART1              ((USART_TypeDef *) USART1_BASE)
#define DBGMCU              ((DBGMCU_TypeDef *) DBGMCU_BASE)

#define DMA1                ((DMA_TypeDef *) DMA1_BASE)
#define DMA1_Channel1       ((DMA_Channel_TypeDef *) DMA1_Channel1_BASE)
#define DMA1_Channel2       ((DMA_Channel_TypeDef *) DMA1_Channel2_BASE)
#define DMA1_Channel3       ((DMA_Channel_TypeDef *) DMA1_Channel3_BASE)
#define DMA1_Channel4       ((DMA_Channel_TypeDef *) DMA1_Channel4_BASE)
#define DMA1_Channel5       ((DMA_Channel_TypeDef *) DMA1_Channel5_BASE)
#define DMA1_Channel6       ((DMA_Channel_TypeDef *) DMA1_Channel6_BASE)
#define DMA1_Channel7       ((DMA_Channel_TypeDef *) DMA1_Channel7_BASE)
#define DMA1_CSELR          ((DMA_Request_TypeDef *) DMA1_CSELR_BASE)


#define FLASH               ((FLASH_TypeDef *) FLASH_R_BASE)
#define OB                  ((OB_TypeDef *) OB_BASE) 
#define RCC                 ((RCC_TypeDef *) RCC_BASE)
#define CRC                 ((CRC_TypeDef *) CRC_BASE)

#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOH               ((GPIO_TypeDef *) GPIOH_BASE)

/**
  * @}
  */

/** @addtogroup Exported_constants
  * @{
  */
  
  /** @addtogroup Peripheral_Registers_Bits_Definition
  * @{
  */
    
/******************************************************************************/
/*                         Peripheral Registers Bits Definition               */
/******************************************************************************/
/******************************************************************************/
/*                                                                            */
/*                      Analog to Digital Converter (ADC)                     */
/*                                                                            */
/******************************************************************************/
/********************  Bits definition for ADC_ISR register  ******************/
#define ADC_ISR_EOCAL_Pos          (11U)                                       
#define ADC_ISR_EOCAL_Msk          (0x1U << ADC_ISR_EOCAL_Pos)                 /*!< 0x00000800 */
#define ADC_ISR_EOCAL              ADC_ISR_EOCAL_Msk                           /*!< End of calibration flag */
#define ADC_ISR_AWD_Pos            (7U)                                        
#define ADC_ISR_AWD_Msk            (0x1U << ADC_ISR_AWD_Pos)                   /*!< 0x00000080 */
#define ADC_ISR_AWD                ADC_ISR_AWD_Msk                             /*!< Analog watchdog flag */
#define ADC_ISR_OVR_Pos            (4U)                                        
#define ADC_ISR_OVR_Msk            (0x1U << ADC_ISR_OVR_Pos)                   /*!< 0x00000010 */
#define ADC_ISR_OVR                ADC_ISR_OVR_Msk                             /*!< Overrun flag */
#define ADC_ISR_EOSEQ_Pos          (3U)                                        
#define ADC_ISR_EOSEQ_Msk          (0x1U << ADC_ISR_EOSEQ_Pos)                 /*!< 0x00000008 */
#define ADC_ISR_EOSEQ              ADC_ISR_EOSEQ_Msk                           /*!< End of Sequence flag */
#define ADC_ISR_EOC_Pos            (2U)                                        
#define ADC_ISR_EOC_Msk            (0x1U << ADC_ISR_EOC_Pos)                   /*!< 0x00000004 */
#define ADC_ISR_EOC                ADC_ISR_EOC_Msk                             /*!< End of Conversion */
#define ADC_ISR_EOSMP_Pos          (1U)                                        
#define ADC_ISR_EOSMP_Msk          (0x1U << ADC_ISR_EOSMP_Pos)                 /*!< 0x00000002 */
#define ADC_ISR_EOSMP              ADC_ISR_EOSMP_Msk                           /*!< End of sampling flag */
#define ADC_ISR_ADRDY_Pos          (0U)                                        
#define ADC_ISR_ADRDY_Msk          (0x1U << ADC_ISR_ADRDY_Pos)                 /*!< 0x00000001 */
#define ADC_ISR_ADRDY              ADC_ISR_ADRDY_Msk                           /*!< ADC Ready */

/* Old EOSEQ bit definition, maintained for legacy purpose */
#define ADC_ISR_EOS                          ADC_ISR_EOSEQ

/********************  Bits definition for ADC_IER register  ******************/
#define ADC_IER_EOCALIE_Pos        (11U)                                       
#define ADC_IER_EOCALIE_Msk        (0x1U << ADC_IER_EOCALIE_Pos)               /*!< 0x00000800 */
#define ADC_IER_EOCALIE            ADC_IER_EOCALIE_Msk                         /*!< Enf Of Calibration interrupt enable */
#define ADC_IER_AWDIE_Pos          (7U)                                        
#define ADC_IER_AWDIE_Msk          (0x1U << ADC_IER_AWDIE_Pos)                 /*!< 0x00000080 */
#define ADC_IER_AWDIE              ADC_IER_AWDIE_Msk                           /*!< Analog Watchdog interrupt enable */
#define ADC_IER_OVRIE_Pos          (4U)                                        
#define ADC_IER_OVRIE_Msk          (0x1U << ADC_IER_OVRIE_Pos)                 /*!< 0x00000010 */
#define ADC_IER_OVRIE              ADC_IER_OVRIE_Msk                           /*!< Overrun interrupt enable */
#define ADC_IER_EOSEQIE_Pos        (3U)                                        
#define ADC_IER_EOSEQIE_Msk        (0x1U << ADC_IER_EOSEQIE_Pos)               /*!< 0x00000008 */
#define ADC_IER_EOSEQIE            ADC_IER_EOSEQIE_Msk                         /*!< End of Sequence of conversion interrupt enable */
#define ADC_IER_EOCIE_Pos          (2U)                                        
#define ADC_IER_EOCIE_Msk          (0x1U << ADC_IER_EOCIE_Pos)                 /*!< 0x00000004 */
#define ADC_IER_EOCIE              ADC_IER_EOCIE_Msk                           /*!< End of Conversion interrupt enable */
#define ADC_IER_EOSMPIE_Pos        (1U)                                        
#define ADC_IER_EOSMPIE_Msk        (0x1U << ADC_IER_EOSMPIE_Pos)               /*!< 0x00000002 */
#define ADC_IER_EOSMPIE            ADC_IER_EOSMPIE_Msk                         /*!< End of sampling interrupt enable */
#define ADC_IER_ADRDYIE_Pos        (0U)                                        
#define ADC_IER_ADRDYIE_Msk        (0x1U << ADC_IER_ADRDYIE_Pos)               /*!< 0x00000001 */
#define ADC_IER_ADRDYIE            ADC_IER_ADRDYIE_Msk                         /*!< ADC Ready interrupt enable */

/* Old EOSEQIE bit definition, maintained for legacy purpose */
#define ADC_IER_EOSIE                        ADC_IER_EOSEQIE

/********************  Bits definition for ADC_CR register  *******************/
#define ADC_CR_ADCAL_Pos           (31U)                                       
#define ADC_CR_ADCAL_Msk           (0x1U << ADC_CR_ADCAL_Pos)                  /*!< 0x80000000 */
#define ADC_CR_ADCAL               ADC_CR_ADCAL_Msk                            /*!< ADC calibration */
#define ADC_CR_ADVREGEN_Pos        (28U)                                       
#define ADC_CR_ADVREGEN_Msk        (0x1U << ADC_CR_ADVREGEN_Pos)               /*!< 0x10000000 */
#define ADC_CR_ADVREGEN            ADC_CR_ADVREGEN_Msk                         /*!< ADC Voltage Regulator Enable */
#define ADC_CR_ADSTP_Pos           (4U)                                        
#define ADC_CR_ADSTP_Msk           (0x1U << ADC_CR_ADSTP_Pos)                  /*!< 0x00000010 */
#define ADC_CR_ADSTP               ADC_CR_ADSTP_Msk                            /*!< ADC stop of conversion command */
#define ADC_CR_ADSTART_Pos         (2U)                                        
#define ADC_CR_ADSTART_Msk         (0x1U << ADC_CR_ADSTART_Pos)                /*!< 0x00000004 */
#define ADC_CR_ADSTART             ADC_CR_ADSTART_Msk                          /*!< ADC start of conversion */
#define ADC_CR_ADDIS_Pos           (1U)                                        
#define ADC_CR_ADDIS_Msk           (0x1U << ADC_CR_ADDIS_Pos)                  /*!< 0x00000002 */
#define ADC_CR_ADDIS               ADC_CR_ADDIS_Msk                            /*!< ADC disable command */
#define ADC_CR_ADEN_Pos            (0U)                                        
#define ADC_CR_ADEN_Msk            (0x1U << ADC_CR_ADEN_Pos)                   /*!< 0x00000001 */
#define ADC_CR_ADEN                ADC_CR_ADEN_Msk                             /*!< ADC enable control */ /*####   TBV  */

/*******************  Bits definition for ADC_CFGR1 register  *****************/
#define ADC_CFGR1_AWDCH_Pos        (26U)                                       
#define ADC_CFGR1_AWDCH_Msk        (0x1FU << ADC_CFGR1_AWDCH_Pos)              /*!< 0x7C000000 */
#define ADC_CFGR1_AWDCH            ADC_CFGR1_AWDCH_Msk                         /*!< AWDCH[4:0] bits (Analog watchdog channel select bits) */
#define ADC_CFGR1_AWDCH_0          (0x01U << ADC_CFGR1_AWDCH_Pos)              /*!< 0x04000000 */
#define ADC_CFGR1_AWDCH_1          (0x02U << ADC_CFGR1_AWDCH_Pos)              /*!< 0x08000000 */
#define ADC_CFGR1_AWDCH_2          (0x04U << ADC_CFGR1_AWDCH_Pos)              /*!< 0x10000000 */
#define ADC_CFGR1_AWDCH_3          (0x08U << ADC_CFGR1_AWDCH_Pos)              /*!< 0x20000000 */
#define ADC_CFGR1_AWDCH_4          (0x10U << ADC_CFGR1_AWDCH_Pos)              /*!< 0x40000000 */
#define ADC_CFGR1_AWDEN_Pos        (23U)                                       
#define ADC_CFGR1_AWDEN_Msk        (0x1U << ADC_CFGR1_AWDEN_Pos)               /*!< 0x00800000 */
#define ADC_CFGR1_AWDEN            ADC_CFGR1_AWDEN_Msk                         /*!< Analog watchdog enable on regular channels */
#define ADC_CFGR1_AWDSGL_Pos       (22U)                                       
#define ADC_CFGR1_AWDSGL_Msk       (0x1U << ADC_CFGR1_AWDSGL_Pos)              /*!< 0x00400000 */
#define ADC_CFGR1_AWDSGL           ADC_CFGR1_AWDSGL_Msk                        /*!< Enable the watchdog on a single channel or on all channels  */
#define ADC_CFGR1_DISCEN_Pos       (16U)                                       
#define ADC_CFGR1_DISCEN_Msk       (0x1U << ADC_CFGR1_DISCEN_Pos)              /*!< 0x00010000 */
#define ADC_CFGR1_DISCEN           ADC_CFGR1_DISCEN_Msk                        /*!< Discontinuous mode on regular channels */
#define ADC_CFGR1_AUTOFF_Pos       (15U)                                       
#define ADC_CFGR1_AUTOFF_Msk       (0x1U << ADC_CFGR1_AUTOFF_Pos)              /*!< 0x00008000 */
#define ADC_CFGR1_AUTOFF           ADC_CFGR1_AUTOFF_Msk                        /*!< ADC auto power off */
#define ADC_CFGR1_WAIT_Pos         (14U)                                       
#define ADC_CFGR1_WAIT_Msk         (0x1U << ADC_CFGR1_WAIT_Pos)                /*!< 0x00004000 */
#define ADC_CFGR1_WAIT             ADC_CFGR1_WAIT_Msk                          /*!< ADC wait conversion mode */
#define ADC_CFGR1_CONT_Pos         (13U)                                       
#define ADC_CFGR1_CONT_Msk         (0x1U << ADC_CFGR1_CONT_Pos)                /*!< 0x00002000 */
#define ADC_CFGR1_CONT             ADC_CFGR1_CONT_Msk                          /*!< Continuous Conversion */
#define ADC_CFGR1_OVRMOD_Pos       (12U)                                       
#define ADC_CFGR1_OVRMOD_Msk       (0x1U << ADC_CFGR1_OVRMOD_Pos)              /*!< 0x00001000 */
#define ADC_CFGR1_OVRMOD           ADC_CFGR1_OVRMOD_Msk                        /*!< Overrun mode */
#define ADC_CFGR1_EXTEN_Pos        (10U)                                       
#define ADC_CFGR1_EXTEN_Msk        (0x3U << ADC_CFGR1_EXTEN_Pos)               /*!< 0x00000C00 */
#define ADC_CFGR1_EXTEN            ADC_CFGR1_EXTEN_Msk                         /*!< EXTEN[1:0] bits (External Trigger Conversion mode for regular channels) */
#define ADC_CFGR1_EXTEN_0          (0x1U << ADC_CFGR1_EXTEN_Pos)               /*!< 0x00000400 */
#define ADC_CFGR1_EXTEN_1          (0x2U << ADC_CFGR1_EXTEN_Pos)               /*!< 0x00000800 */
#define ADC_CFGR1_EXTSEL_Pos       (6U)                                        
#define ADC_CFGR1_EXTSEL_Msk       (0x7U << ADC_CFGR1_EXTSEL_Pos)              /*!< 0x000001C0 */
#define ADC_CFGR1_EXTSEL           ADC_CFGR1_EXTSEL_Msk                        /*!< EXTSEL[2:0] bits (External Event Select for regular group) */
#define ADC_CFGR1_EXTSEL_0         (0x1U << ADC_CFGR1_EXTSEL_Pos)              /*!< 0x00000040 */
#define ADC_CFGR1_EXTSEL_1         (0x2U << ADC_CFGR1_EXTSEL_Pos)              /*!< 0x00000080 */
#define ADC_CFGR1_EXTSEL_2         (0x4U << ADC_CFGR1_EXTSEL_Pos)              /*!< 0x00000100 */
#define ADC_CFGR1_ALIGN_Pos        (5U)                                        
#define ADC_CFGR1_ALIGN_Msk        (0x1U << ADC_CFGR1_ALIGN_Pos)               /*!< 0x00000020 */
#define ADC_CFGR1_ALIGN            ADC_CFGR1_ALIGN_Msk                         /*!< Data Alignment */
#define ADC_CFGR1_RES_Pos          (3U)                                        
#define ADC_CFGR1_RES_Msk          (0x3U << ADC_CFGR1_RES_Pos)                 /*!< 0x00000018 */
#define ADC_CFGR1_RES              ADC_CFGR1_RES_Msk                           /*!< RES[1:0] bits (Resolution) */
#define ADC_CFGR1_RES_0            (0x1U << ADC_CFGR1_RES_Pos)                 /*!< 0x00000008 */
#define ADC_CFGR1_RES_1            (0x2U << ADC_CFGR1_RES_Pos)                 /*!< 0x00000010 */
#define ADC_CFGR1_SCANDIR_Pos      (2U)                                        
#define ADC_CFGR1_SCANDIR_Msk      (0x1U << ADC_CFGR1_SCANDIR_Pos)             /*!< 0x00000004 */
#define ADC_CFGR1_SCANDIR          ADC_CFGR1_SCANDIR_Msk                       /*!< Sequence scan direction */
#define ADC_CFGR1_DMACFG_Pos       (1U)                                        
#define ADC_CFGR1_DMACFG_Msk       (0x1U << ADC_CFGR1_DMACFG_Pos)              /*!< 0x00000002 */
#define ADC_CFGR1_DMACFG           ADC_CFGR1_DMACFG_Msk                        /*!< Direct memory access configuration */
#define ADC_CFGR1_DMAEN_Pos        (0U)                                        
#define ADC_CFGR1_DMAEN_Msk        (0x1U << ADC_CFGR1_DMAEN_Pos)               /*!< 0x00000001 */
#define ADC_CFGR1_DMAEN            ADC_CFGR1_DMAEN_Msk                         /*!< Direct memory access enable */

/* Old WAIT bit definition, maintained for legacy purpose */
#define ADC_CFGR1_AUTDLY                    ADC_CFGR1_WAIT

/*******************  Bits definition for ADC_CFGR2 register  *****************/
#define ADC_CFGR2_TOVS_Pos         (9U)                                        
#define ADC_CFGR2_TOVS_Msk         (0x400001U << ADC_CFGR2_TOVS_Pos)           /*!< 0x80000200 */
#define ADC_CFGR2_TOVS             ADC_CFGR2_TOVS_Msk                          /*!< Triggered Oversampling */
#define ADC_CFGR2_OVSS_Pos         (5U)                                        
#define ADC_CFGR2_OVSS_Msk         (0xFU << ADC_CFGR2_OVSS_Pos)                /*!< 0x000001E0 */
#define ADC_CFGR2_OVSS             ADC_CFGR2_OVSS_Msk                          /*!< OVSS [3:0] bits (Oversampling shift) */
#define ADC_CFGR2_OVSS_0           (0x1U << ADC_CFGR2_OVSS_Pos)                /*!< 0x00000020 */
#define ADC_CFGR2_OVSS_1           (0x2U << ADC_CFGR2_OVSS_Pos)                /*!< 0x00000040 */
#define ADC_CFGR2_OVSS_2           (0x4U << ADC_CFGR2_OVSS_Pos)                /*!< 0x00000080 */
#define ADC_CFGR2_OVSS_3           (0x8U << ADC_CFGR2_OVSS_Pos)                /*!< 0x00000100 */
#define ADC_CFGR2_OVSR_Pos         (2U)                                        
#define ADC_CFGR2_OVSR_Msk         (0x7U << ADC_CFGR2_OVSR_Pos)                /*!< 0x0000001C */
#define ADC_CFGR2_OVSR             ADC_CFGR2_OVSR_Msk                          /*!< OVSR  [2:0] bits (Oversampling ratio) */
#define ADC_CFGR2_OVSR_0           (0x1U << ADC_CFGR2_OVSR_Pos)                /*!< 0x00000004 */
#define ADC_CFGR2_OVSR_1           (0x2U << ADC_CFGR2_OVSR_Pos)                /*!< 0x00000008 */
#define ADC_CFGR2_OVSR_2           (0x4U << ADC_CFGR2_OVSR_Pos)                /*!< 0x00000010 */
#define ADC_CFGR2_OVSE_Pos         (0U)                                        
#define ADC_CFGR2_OVSE_Msk         (0x1U << ADC_CFGR2_OVSE_Pos)                /*!< 0x00000001 */
#define ADC_CFGR2_OVSE             ADC_CFGR2_OVSE_Msk                          /*!< Oversampler Enable */
#define ADC_CFGR2_CKMODE_Pos       (30U)                                       
#define ADC_CFGR2_CKMODE_Msk       (0x3U << ADC_CFGR2_CKMODE_Pos)              /*!< 0xC0000000 */
#define ADC_CFGR2_CKMODE           ADC_CFGR2_CKMODE_Msk                        /*!< CKMODE [1:0] bits (ADC clock mode) */
#define ADC_CFGR2_CKMODE_0         (0x1U << ADC_CFGR2_CKMODE_Pos)              /*!< 0x40000000 */
#define ADC_CFGR2_CKMODE_1         (0x2U << ADC_CFGR2_CKMODE_Pos)              /*!< 0x80000000 */


/******************  Bit definition for ADC_SMPR register  ********************/
#define ADC_SMPR_SMP_Pos           (0U)                                        
#define ADC_SMPR_SMP_Msk           (0x7U << ADC_SMPR_SMP_Pos)                  /*!< 0x00000007 */
#define ADC_SMPR_SMP               ADC_SMPR_SMP_Msk                            /*!< SMPR[2:0] bits (Sampling time selection) */
#define ADC_SMPR_SMP_0             (0x1U << ADC_SMPR_SMP_Pos)                  /*!< 0x00000001 */
#define ADC_SMPR_SMP_1             (0x2U << ADC_SMPR_SMP_Pos)                  /*!< 0x00000002 */
#define ADC_SMPR_SMP_2             (0x4U << ADC_SMPR_SMP_Pos)                  /*!< 0x00000004 */

/* Legacy defines */
#define ADC_SMPR_SMPR                       ADC_SMPR_SMP
#define ADC_SMPR_SMPR_0                     ADC_SMPR_SMP_0
#define ADC_SMPR_SMPR_1                     ADC_SMPR_SMP_1
#define ADC_SMPR_SMPR_2                     ADC_SMPR_SMP_2

/*******************  Bit definition for ADC_TR register  ********************/
#define ADC_TR_HT_Pos              (16U)                                       
#define ADC_TR_HT_Msk              (0xFFFU << ADC_TR_HT_Pos)                   /*!< 0x0FFF0000 */
#define ADC_TR_HT                  ADC_TR_HT_Msk                               /*!< Analog watchdog high threshold */
#define ADC_TR_LT_Pos              (0U)                                        
#define ADC_TR_LT_Msk              (0xFFFU << ADC_TR_LT_Pos)                   /*!< 0x00000FFF */
#define ADC_TR_LT                  ADC_TR_LT_Msk                               /*!< Analog watchdog low threshold */

/******************  Bit definition for ADC_CHSELR register  ******************/
#define ADC_CHSELR_CHSEL_Pos       (0U)                                        
#define ADC_CHSELR_CHSEL_Msk       (0x7FFFFU << ADC_CHSELR_CHSEL_Pos)          /*!< 0x0007FFFF */
#define ADC_CHSELR_CHSEL           ADC_CHSELR_CHSEL_Msk                        /*!< ADC group regular sequencer channels */
#define ADC_CHSELR_CHSEL18_Pos     (18U)                                       
#define ADC_CHSELR_CHSEL18_Msk     (0x1U << ADC_CHSELR_CHSEL18_Pos)            /*!< 0x00040000 */
#define ADC_CHSELR_CHSEL18         ADC_CHSELR_CHSEL18_Msk                      /*!< Channel 18 selection */
#define ADC_CHSELR_CHSEL17_Pos     (17U)                                       
#define ADC_CHSELR_CHSEL17_Msk     (0x1U << ADC_CHSELR_CHSEL17_Pos)            /*!< 0x00020000 */
#define ADC_CHSELR_CHSEL17         ADC_CHSELR_CHSEL17_Msk                      /*!< Channel 17 selection */
#define ADC_CHSELR_CHSEL15_Pos     (15U)                                       
#define ADC_CHSELR_CHSEL15_Msk     (0x1U << ADC_CHSELR_CHSEL15_Pos)            /*!< 0x00008000 */
#define ADC_CHSELR_CHSEL15         ADC_CHSELR_CHSEL15_Msk                      /*!< Channel 15 selection */
#define ADC_CHSELR_CHSEL14_Pos     (14U)                                       
#define ADC_CHSELR_CHSEL14_Msk     (0x1U << ADC_CHSELR_CHSEL14_Pos)            /*!< 0x00004000 */
#define ADC_CHSELR_CHSEL14         ADC_CHSELR_CHSEL14_Msk                      /*!< Channel 14 selection */
#define ADC_CHSELR_CHSEL13_Pos     (13U)                                       
#define ADC_CHSELR_CHSEL13_Msk     (0x1U << ADC_CHSELR_CHSEL13_Pos)            /*!< 0x00002000 */
#define ADC_CHSELR_CHSEL13         ADC_CHSELR_CHSEL13_Msk                      /*!< Channel 13 selection */
#define ADC_CHSELR_CHSEL12_Pos     (12U)                                       
#define ADC_CHSELR_CHSEL12_Msk     (0x1U << ADC_CHSELR_CHSEL12_Pos)            /*!< 0x00001000 */
#define ADC_CHSELR_CHSEL12         ADC_CHSELR_CHSEL12_Msk                      /*!< Channel 12 selection */
#define ADC_CHSELR_CHSEL11_Pos     (11U)                                       
#define ADC_CHSELR_CHSEL11_Msk     (0x1U << ADC_CHSELR_CHSEL11_Pos)            /*!< 0x00000800 */
#define ADC_CHSELR_CHSEL11         ADC_CHSELR_CHSEL11_Msk                      /*!< Channel 11 selection */
#define ADC_CHSELR_CHSEL10_Pos     (10U)                                       
#define ADC_CHSELR_CHSEL10_Msk     (0x1U << ADC_CHSELR_CHSEL10_Pos)            /*!< 0x00000400 */
#define ADC_CHSELR_CHSEL10         ADC_CHSELR_CHSEL10_Msk                      /*!< Channel 10 selection */
#define ADC_CHSELR_CHSEL9_Pos      (9U)                                        
#define ADC_CHSELR_CHSEL9_Msk      (0x1U << ADC_CHSELR_CHSEL9_Pos)             /*!< 0x00000200 */
#define ADC_CHSELR_CHSEL9          ADC_CHSELR_CHSEL9_Msk                       /*!< Channel 9 selection */
#define ADC_CHSELR_CHSEL8_Pos      (8U)                                        
#define ADC_CHSELR_CHSEL8_Msk      (0x1U << ADC_CHSELR_CHSEL8_Pos)             /*!< 0x00000100 */
#define ADC_CHSELR_CHSEL8          ADC_CHSELR_CHSEL8_Msk                       /*!< Channel 8 selection */
#define ADC_CHSELR_CHSEL7_Pos      (7U)                                        
#define ADC_CHSELR_CHSEL7_Msk      (0x1U << ADC_CHSELR_CHSEL7_Pos)             /*!< 0x00000080 */
#define ADC_CHSELR_CHSEL7          ADC_CHSELR_CHSEL7_Msk                       /*!< Channel 7 selection */
#define ADC_CHSELR_CHSEL6_Pos      (6U)                                        
#define ADC_CHSELR_CHSEL6_Msk      (0x1U << ADC_CHSELR_CHSEL6_Pos)             /*!< 0x00000040 */
#define ADC_CHSELR_CHSEL6          ADC_CHSELR_CHSEL6_Msk                       /*!< Channel 6 selection */
#define ADC_CHSELR_CHSEL5_Pos      (5U)                                        
#define ADC_CHSELR_CHSEL5_Msk      (0x1U << ADC_CHSELR_CHSEL5_Pos)             /*!< 0x00000020 */
#define ADC_CHSELR_CHSEL5          ADC_CHSELR_CHSEL5_Msk                       /*!< Channel 5 selection */
#define ADC_CHSELR_CHSEL4_Pos      (4U)                                        
#define ADC_CHSELR_CHSEL4_Msk      (0x1U << ADC_CHSELR_CHSEL4_Pos)             /*!< 0x00000010 */
#define ADC_CHSELR_CHSEL4          ADC_CHSELR_CHSEL4_Msk                       /*!< Channel 4 selection */
#define ADC_CHSELR_CHSEL3_Pos      (3U)                                        
#define ADC_CHSELR_CHSEL3_Msk      (0x1U << ADC_CHSELR_CHSEL3_Pos)             /*!< 0x00000008 */
#define ADC_CHSELR_CHSEL3          ADC_CHSELR_CHSEL3_Msk                       /*!< Channel 3 selection */
#define ADC_CHSELR_CHSEL2_Pos      (2U)                                        
#define ADC_CHSELR_CHSEL2_Msk      (0x1U << ADC_CHSELR_CHSEL2_Pos)             /*!< 0x00000004 */
#define ADC_CHSELR_CHSEL2          ADC_CHSELR_CHSEL2_Msk                       /*!< Channel 2 selection */
#define ADC_CHSELR_CHSEL1_Pos      (1U)                                        
#define ADC_CHSELR_CHSEL1_Msk      (0x1U << ADC_CHSELR_CHSEL1_Pos)             /*!< 0x00000002 */
#define ADC_CHSELR_CHSEL1          ADC_CHSELR_CHSEL1_Msk                       /*!< Channel 1 selection */
#define ADC_CHSELR_CHSEL0_Pos      (0U)                                        
#define ADC_CHSELR_CHSEL0_Msk      (0x1U << ADC_CHSELR_CHSEL0_Pos)             /*!< 0x00000001 */
#define ADC_CHSELR_CHSEL0          ADC_CHSELR_CHSEL0_Msk                       /*!< Channel 0 selection */

/********************  Bit definition for ADC_DR register  ********************/
#define ADC_DR_DATA_Pos            (0U)                                        
#define ADC_DR_DATA_Msk            (0xFFFFU << ADC_DR_DATA_Pos)                /*!< 0x0000FFFF */
#define ADC_DR_DATA                ADC_DR_DATA_Msk                             /*!< Regular data */

/********************  Bit definition for ADC_CALFACT register  ********************/
#define ADC_CALFACT_CALFACT_Pos    (0U)                                        
#define ADC_CALFACT_CALFACT_Msk    (0x7FU << ADC_CALFACT_CALFACT_Pos)          /*!< 0x0000007F */
#define ADC_CALFACT_CALFACT        ADC_CALFACT_CALFACT_Msk                     /*!< Calibration factor */

/*******************  Bit definition for ADC_CCR register  ********************/
#define ADC_CCR_LFMEN_Pos          (25U)                                       
#define ADC_CCR_LFMEN_Msk          (0x1U << ADC_CCR_LFMEN_Pos)                 /*!< 0x02000000 */
#define ADC_CCR_LFMEN              ADC_CCR_LFMEN_Msk                           /*!< Low Frequency Mode enable */
#define ADC_CCR_TSEN_Pos           (23U)                                       
#define ADC_CCR_TSEN_Msk           (0x1U << ADC_CCR_TSEN_Pos)                  /*!< 0x00800000 */
#define ADC_CCR_TSEN               ADC_CCR_TSEN_Msk                            /*!< Temperature sensore enable */
#define ADC_CCR_VREFEN_Pos         (22U)                                       
#define ADC_CCR_VREFEN_Msk         (0x1U << ADC_CCR_VREFEN_Pos)                /*!< 0x00400000 */
#define ADC_CCR_VREFEN             ADC_CCR_VREFEN_Msk                          /*!< Vrefint enable */
#define ADC_CCR_PRESC_Pos          (18U)                                       
#define ADC_CCR_PRESC_Msk          (0xFU << ADC_CCR_PRESC_Pos)                 /*!< 0x003C0000 */
#define ADC_CCR_PRESC              ADC_CCR_PRESC_Msk                           /*!< PRESC  [3:0] bits (ADC prescaler) */
#define ADC_CCR_PRESC_0            (0x1U << ADC_CCR_PRESC_Pos)                 /*!< 0x00040000 */
#define ADC_CCR_PRESC_1            (0x2U << ADC_CCR_PRESC_Pos)                 /*!< 0x00080000 */
#define ADC_CCR_PRESC_2            (0x4U << ADC_CCR_PRESC_Pos)                 /*!< 0x00100000 */
#define ADC_CCR_PRESC_3            (0x8U << ADC_CCR_PRESC_Pos)                 /*!< 0x00200000 */

/******************************************************************************/
/*                                                                            */
/*                      Analog Comparators (COMP)                             */
/*                                                                            */
/******************************************************************************/
/*************  Bit definition for COMP_CSR register (COMP1 and COMP2)  **************/
/* COMP1 bits definition */
#define COMP_CSR_COMP1EN_Pos           (0U)                                    
#define COMP_CSR_COMP1EN_Msk           (0x1U << COMP_CSR_COMP1EN_Pos)          /*!< 0x00000001 */
#define COMP_CSR_COMP1EN               COMP_CSR_COMP1EN_Msk                    /*!< COMP1 enable */
#define COMP_CSR_COMP1INNSEL_Pos       (4U)                                    
#define COMP_CSR_COMP1INNSEL_Msk       (0x3U << COMP_CSR_COMP1INNSEL_Pos)      /*!< 0x00000030 */
#define COMP_CSR_COMP1INNSEL           COMP_CSR_COMP1INNSEL_Msk                /*!< COMP1 inverting input select */
#define COMP_CSR_COMP1INNSEL_0         (0x1U << COMP_CSR_COMP1INNSEL_Pos)      /*!< 0x00000010 */
#define COMP_CSR_COMP1INNSEL_1         (0x2U << COMP_CSR_COMP1INNSEL_Pos)      /*!< 0x00000020 */
#define COMP_CSR_COMP1WM_Pos           (8U)                                    
#define COMP_CSR_COMP1WM_Msk           (0x1U << COMP_CSR_COMP1WM_Pos)          /*!< 0x00000100 */
#define COMP_CSR_COMP1WM               COMP_CSR_COMP1WM_Msk                    /*!< Comparators window mode enable */
#define COMP_CSR_COMP1LPTIM1IN1_Pos    (12U)                                   
#define COMP_CSR_COMP1LPTIM1IN1_Msk    (0x1U << COMP_CSR_COMP1LPTIM1IN1_Pos)   /*!< 0x00001000 */
#define COMP_CSR_COMP1LPTIM1IN1        COMP_CSR_COMP1LPTIM1IN1_Msk             /*!< COMP1 LPTIM1 IN1 connection */
#define COMP_CSR_COMP1POLARITY_Pos     (15U)                                   
#define COMP_CSR_COMP1POLARITY_Msk     (0x1U << COMP_CSR_COMP1POLARITY_Pos)    /*!< 0x00008000 */
#define COMP_CSR_COMP1POLARITY         COMP_CSR_COMP1POLARITY_Msk              /*!< COMP1 output polarity */
#define COMP_CSR_COMP1VALUE_Pos        (30U)                                   
#define COMP_CSR_COMP1VALUE_Msk        (0x1U << COMP_CSR_COMP1VALUE_Pos)       /*!< 0x40000000 */
#define COMP_CSR_COMP1VALUE            COMP_CSR_COMP1VALUE_Msk                 /*!< COMP1 output level */
#define COMP_CSR_COMP1LOCK_Pos         (31U)                                   
#define COMP_CSR_COMP1LOCK_Msk         (0x1U << COMP_CSR_COMP1LOCK_Pos)        /*!< 0x80000000 */
#define COMP_CSR_COMP1LOCK             COMP_CSR_COMP1LOCK_Msk                  /*!< COMP1 lock */
/* COMP2 bits definition */
#define COMP_CSR_COMP2EN_Pos           (0U)                                    
#define COMP_CSR_COMP2EN_Msk           (0x1U << COMP_CSR_COMP2EN_Pos)          /*!< 0x00000001 */
#define COMP_CSR_COMP2EN               COMP_CSR_COMP2EN_Msk                    /*!< COMP2 enable */
#define COMP_CSR_COMP2SPEED_Pos        (3U)                                    
#define COMP_CSR_COMP2SPEED_Msk        (0x1U << COMP_CSR_COMP2SPEED_Pos)       /*!< 0x00000008 */
#define COMP_CSR_COMP2SPEED            COMP_CSR_COMP2SPEED_Msk                 /*!< COMP2 power mode */
#define COMP_CSR_COMP2INNSEL_Pos       (4U)                                    
#define COMP_CSR_COMP2INNSEL_Msk       (0x7U << COMP_CSR_COMP2INNSEL_Pos)      /*!< 0x00000070 */
#define COMP_CSR_COMP2INNSEL           COMP_CSR_COMP2INNSEL_Msk                /*!< COMP2 inverting input select */
#define COMP_CSR_COMP2INNSEL_0         (0x1U << COMP_CSR_COMP2INNSEL_Pos)      /*!< 0x00000010 */
#define COMP_CSR_COMP2INNSEL_1         (0x2U << COMP_CSR_COMP2INNSEL_Pos)      /*!< 0x00000020 */
#define COMP_CSR_COMP2INNSEL_2         (0x4U << COMP_CSR_COMP2INNSEL_Pos)      /*!< 0x00000040 */
#define COMP_CSR_COMP2INPSEL_Pos       (8U)                                    
#define COMP_CSR_COMP2INPSEL_Msk       (0x7U << COMP_CSR_COMP2INPSEL_Pos)      /*!< 0x00000700 */
#define COMP_CSR_COMP2INPSEL           COMP_CSR_COMP2INPSEL_Msk                /*!< COMPx non inverting input select */
#define COMP_CSR_COMP2INPSEL_0         (0x1U << COMP_CSR_COMP2INPSEL_Pos)      /*!< 0x00000100 */
#define COMP_CSR_COMP2INPSEL_1         (0x2U << COMP_CSR_COMP2INPSEL_Pos)      /*!< 0x00000200 */
#define COMP_CSR_COMP2INPSEL_2         (0x4U << COMP_CSR_COMP2INPSEL_Pos)      /*!< 0x00000400 */
#define COMP_CSR_COMP2LPTIM1IN2_Pos    (12U)                                   
#define COMP_CSR_COMP2LPTIM1IN2_Msk    (0x1U << COMP_CSR_COMP2LPTIM1IN2_Pos)   /*!< 0x00001000 */
#define COMP_CSR_COMP2LPTIM1IN2        COMP_CSR_COMP2LPTIM1IN2_Msk             /*!< COMP2 LPTIM1 IN2 connection */
#define COMP_CSR_COMP2LPTIM1IN1_Pos    (13U)                                   
#define COMP_CSR_COMP2LPTIM1IN1_Msk    (0x1U << COMP_CSR_COMP2LPTIM1IN1_Pos)   /*!< 0x00002000 */
#define COMP_CSR_COMP2LPTIM1IN1        COMP_CSR_COMP2LPTIM1IN1_Msk             /*!< COMP2 LPTIM1 IN1 connection */
#define COMP_CSR_COMP2POLARITY_Pos     (15U)                                   
#define COMP_CSR_COMP2POLARITY_Msk     (0x1U << COMP_CSR_COMP2POLARITY_Pos)    /*!< 0x00008000 */
#define COMP_CSR_COMP2POLARITY         COMP_CSR_COMP2POLARITY_Msk              /*!< COMP2 output polarity */
#define COMP_CSR_COMP2VALUE_Pos        (30U)                                   
#define COMP_CSR_COMP2VALUE_Msk        (0x1U << COMP_CSR_COMP2VALUE_Pos)       /*!< 0x40000000 */
#define COMP_CSR_COMP2VALUE            COMP_CSR_COMP2VALUE_Msk                 /*!< COMP2 output level */
#define COMP_CSR_COMP2LOCK_Pos         (31U)                                   
#define COMP_CSR_COMP2LOCK_Msk         (0x1U << COMP_CSR_COMP2LOCK_Pos)        /*!< 0x80000000 */
#define COMP_CSR_COMP2LOCK             COMP_CSR_COMP2LOCK_Msk                  /*!< COMP2 lock */

/**********************  Bit definition for COMP_CSR register common  ****************/
#define COMP_CSR_COMPxEN_Pos           (0U)                                    
#define COMP_CSR_COMPxEN_Msk           (0x1U << COMP_CSR_COMPxEN_Pos)          /*!< 0x00000001 */
#define COMP_CSR_COMPxEN               COMP_CSR_COMPxEN_Msk                    /*!< COMPx enable */
#define COMP_CSR_COMPxPOLARITY_Pos     (15U)                                   
#define COMP_CSR_COMPxPOLARITY_Msk     (0x1U << COMP_CSR_COMPxPOLARITY_Pos)    /*!< 0x00008000 */
#define COMP_CSR_COMPxPOLARITY         COMP_CSR_COMPxPOLARITY_Msk              /*!< COMPx output polarity */
#define COMP_CSR_COMPxOUTVALUE_Pos     (30U)                                   
#define COMP_CSR_COMPxOUTVALUE_Msk     (0x1U << COMP_CSR_COMPxOUTVALUE_Pos)    /*!< 0x40000000 */
#define COMP_CSR_COMPxOUTVALUE         COMP_CSR_COMPxOUTVALUE_Msk              /*!< COMPx output level */
#define COMP_CSR_COMPxLOCK_Pos         (31U)                                   
#define COMP_CSR_COMPxLOCK_Msk         (0x1U << COMP_CSR_COMPxLOCK_Pos)        /*!< 0x80000000 */
#define COMP_CSR_COMPxLOCK             COMP_CSR_COMPxLOCK_Msk                  /*!< COMPx lock */

/* Reference defines */
#define COMP_CSR_WINMODE   COMP_CSR_COMP1WM /*!< Bit intended to be used with COMP common instance (COMP_Common_TypeDef)  */

/******************************************************************************/
/*                                                                            */
/*                       CRC calculation unit (CRC)                           */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for CRC_DR register  *********************/
#define CRC_DR_DR_Pos            (0U)                                          
#define CRC_DR_DR_Msk            (0xFFFFFFFFU << CRC_DR_DR_Pos)                /*!< 0xFFFFFFFF */
#define CRC_DR_DR                CRC_DR_DR_Msk                                 /*!< Data register bits */

/*******************  Bit definition for CRC_IDR register  ********************/
#define CRC_IDR_IDR              ((uint8_t)0xFFU)                              /*!< General-purpose 8-bit data register bits */

/********************  Bit definition for CRC_CR register  ********************/
#define CRC_CR_RESET_Pos         (0U)                                          
#define CRC_CR_RESET_Msk         (0x1U << CRC_CR_RESET_Pos)                    /*!< 0x00000001 */
#define CRC_CR_RESET             CRC_CR_RESET_Msk                              /*!< RESET the CRC computation unit bit */
#define CRC_CR_POLYSIZE_Pos      (3U)                                          
#define CRC_CR_POLYSIZE_Msk      (0x3U << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000018 */
#define CRC_CR_POLYSIZE          CRC_CR_POLYSIZE_Msk                           /*!< Polynomial size bits */
#define CRC_CR_POLYSIZE_0        (0x1U << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000008 */
#define CRC_CR_POLYSIZE_1        (0x2U << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000010 */
#define CRC_CR_REV_IN_Pos        (5U)                                          
#define CRC_CR_REV_IN_Msk        (0x3U << CRC_CR_REV_IN_Pos)                   /*!< 0x00000060 */
#define CRC_CR_REV_IN            CRC_CR_REV_IN_Msk                             /*!< REV_IN Reverse Input Data bits */
#define CRC_CR_REV_IN_0          (0x1U << CRC_CR_REV_IN_Pos)                   /*!< 0x00000020 */
#define CRC_CR_REV_IN_1          (0x2U << CRC_CR_REV_IN_Pos)                   /*!< 0x00000040 */
#define CRC_CR_REV_OUT_Pos       (7U)                                          
#define CRC_CR_REV_OUT_Msk       (0x1U << CRC_CR_REV_OUT_Pos)                  /*!< 0x00000080 */
#define CRC_CR_REV_OUT           CRC_CR_REV_OUT_Msk                            /*!< REV_OUT Reverse Output Data bits */

/*******************  Bit definition for CRC_INIT register  *******************/
#define CRC_INIT_INIT_Pos        (0U)                                          
#define CRC_INIT_INIT_Msk        (0xFFFFFFFFU << CRC_INIT_INIT_Pos)            /*!< 0xFFFFFFFF */
#define CRC_INIT_INIT            CRC_INIT_INIT_Msk                             /*!< Initial CRC value bits */

/*******************  Bit definition for CRC_POL register  ********************/
#define CRC_POL_POL_Pos          (0U)                                          
#define CRC_POL_POL_Msk          (0xFFFFFFFFU << CRC_POL_POL_Pos)              /*!< 0xFFFFFFFF */
#define CRC_POL_POL              CRC_POL_POL_Msk                               /*!< Coefficients of the polynomial */

/******************************************************************************/
/*                                                                            */
/*                           Debug MCU (DBGMCU)                               */
/*                                                                            */
/******************************************************************************/

/****************  Bit definition for DBGMCU_IDCODE register  *****************/
#define DBGMCU_IDCODE_DEV_ID_Pos               (0U)                            
#define DBGMCU_IDCODE_DEV_ID_Msk               (0xFFFU << DBGMCU_IDCODE_DEV_ID_Pos) /*!< 0x00000FFF */
#define DBGMCU_IDCODE_DEV_ID                   DBGMCU_IDCODE_DEV_ID_Msk        /*!< Device Identifier */

#define DBGMCU_IDCODE_REV_ID_Pos               (16U)                           
#define DBGMCU_IDCODE_REV_ID_Msk               (0xFFFFU << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0xFFFF0000 */
#define DBGMCU_IDCODE_REV_ID                   DBGMCU_IDCODE_REV_ID_Msk        /*!< REV_ID[15:0] bits (Revision Identifier) */
#define DBGMCU_IDCODE_REV_ID_0                 (0x0001U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00010000 */
#define DBGMCU_IDCODE_REV_ID_1                 (0x0002U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00020000 */
#define DBGMCU_IDCODE_REV_ID_2                 (0x0004U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00040000 */
#define DBGMCU_IDCODE_REV_ID_3                 (0x0008U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00080000 */
#define DBGMCU_IDCODE_REV_ID_4                 (0x0010U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00100000 */
#define DBGMCU_IDCODE_REV_ID_5                 (0x0020U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00200000 */
#define DBGMCU_IDCODE_REV_ID_6                 (0x0040U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00400000 */
#define DBGMCU_IDCODE_REV_ID_7                 (0x0080U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00800000 */
#define DBGMCU_IDCODE_REV_ID_8                 (0x0100U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x01000000 */
#define DBGMCU_IDCODE_REV_ID_9                 (0x0200U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x02000000 */
#define DBGMCU_IDCODE_REV_ID_10                (0x0400U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x04000000 */
#define DBGMCU_IDCODE_REV_ID_11                (0x0800U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x08000000 */
#define DBGMCU_IDCODE_REV_ID_12                (0x1000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x10000000 */
#define DBGMCU_IDCODE_REV_ID_13                (0x2000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x20000000 */
#define DBGMCU_IDCODE_REV_ID_14                (0x4000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x40000000 */
#define DBGMCU_IDCODE_REV_ID_15                (0x8000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x80000000 */

/******************  Bit definition for DBGMCU_CR register  *******************/
#define DBGMCU_CR_DBG_Pos                      (0U)                            
#define DBGMCU_CR_DBG_Msk                      (0x7U << DBGMCU_CR_DBG_Pos)     /*!< 0x00000007 */
#define DBGMCU_CR_DBG                          DBGMCU_CR_DBG_Msk               /*!< Debug mode mask */
#define DBGMCU_CR_DBG_SLEEP_Pos                (0U)                            
#define DBGMCU_CR_DBG_SLEEP_Msk                (0x1U << DBGMCU_CR_DBG_SLEEP_Pos) /*!< 0x00000001 */
#define DBGMCU_CR_DBG_SLEEP                    DBGMCU_CR_DBG_SLEEP_Msk         /*!< Debug Sleep Mode */
#define DBGMCU_CR_DBG_STOP_Pos                 (1U)                            
#define DBGMCU_CR_DBG_STOP_Msk                 (0x1U << DBGMCU_CR_DBG_STOP_Pos) /*!< 0x00000002 */
#define DBGMCU_CR_DBG_STOP                     DBGMCU_CR_DBG_STOP_Msk          /*!< Debug Stop Mode */
#define DBGMCU_CR_DBG_STANDBY_Pos              (2U)                            
#define DBGMCU_CR_DBG_STANDBY_Msk              (0x1U << DBGMCU_CR_DBG_STANDBY_Pos) /*!< 0x00000004 */
#define DBGMCU_CR_DBG_STANDBY                  DBGMCU_CR_DBG_STANDBY_Msk       /*!< Debug Standby mode */

/******************  Bit definition for DBGMCU_APB1_FZ register  **************/
#define DBGMCU_APB1_FZ_DBG_TIM2_STOP_Pos       (0U)                            
#define DBGMCU_APB1_FZ_DBG_TIM2_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_TIM2_STOP_Pos) /*!< 0x00000001 */
#define DBGMCU_APB1_FZ_DBG_TIM2_STOP           DBGMCU_APB1_FZ_DBG_TIM2_STOP_Msk /*!< TIM2 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP_Pos       (4U)                            
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_TIM6_STOP_Pos) /*!< 0x00000010 */
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP           DBGMCU_APB1_FZ_DBG_TIM6_STOP_Msk /*!< TIM6 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_RTC_STOP_Pos        (10U)                           
#define DBGMCU_APB1_FZ_DBG_RTC_STOP_Msk        (0x1U << DBGMCU_APB1_FZ_DBG_RTC_STOP_Pos) /*!< 0x00000400 */
#define DBGMCU_APB1_FZ_DBG_RTC_STOP            DBGMCU_APB1_FZ_DBG_RTC_STOP_Msk /*!< RTC Calendar frozen when core is halted */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos       (11U)                           
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos) /*!< 0x00000800 */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP           DBGMCU_APB1_FZ_DBG_WWDG_STOP_Msk /*!< Debug Window Watchdog stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos       (12U)                           
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos) /*!< 0x00001000 */
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP           DBGMCU_APB1_FZ_DBG_IWDG_STOP_Msk /*!< Debug Independent Watchdog stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_I2C1_STOP_Pos       (21U)                           
#define DBGMCU_APB1_FZ_DBG_I2C1_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_I2C1_STOP_Pos) /*!< 0x00200000 */
#define DBGMCU_APB1_FZ_DBG_I2C1_STOP           DBGMCU_APB1_FZ_DBG_I2C1_STOP_Msk /*!< I2C1 SMBUS timeout mode stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_I2C2_STOP_Pos       (22U)                           
#define DBGMCU_APB1_FZ_DBG_I2C2_STOP_Msk       (0x1U << DBGMCU_APB1_FZ_DBG_I2C2_STOP_Pos) /*!< 0x00400000 */
#define DBGMCU_APB1_FZ_DBG_I2C2_STOP           DBGMCU_APB1_FZ_DBG_I2C2_STOP_Msk /*!< I2C2 SMBUS timeout mode stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_LPTIMER_STOP_Pos    (31U)                           
#define DBGMCU_APB1_FZ_DBG_LPTIMER_STOP_Msk    (0x1U << DBGMCU_APB1_FZ_DBG_LPTIMER_STOP_Pos) /*!< 0x80000000 */
#define DBGMCU_APB1_FZ_DBG_LPTIMER_STOP        DBGMCU_APB1_FZ_DBG_LPTIMER_STOP_Msk /*!< LPTIM1 counter stopped when core is halted */
/******************  Bit definition for DBGMCU_APB2_FZ register  **************/
#define DBGMCU_APB2_FZ_DBG_TIM22_STOP_Pos      (5U)                            
#define DBGMCU_APB2_FZ_DBG_TIM22_STOP_Msk      (0x1U << DBGMCU_APB2_FZ_DBG_TIM22_STOP_Pos) /*!< 0x00000020 */
#define DBGMCU_APB2_FZ_DBG_TIM22_STOP          DBGMCU_APB2_FZ_DBG_TIM22_STOP_Msk /*!< TIM22 counter stopped when core is halted */
#define DBGMCU_APB2_FZ_DBG_TIM21_STOP_Pos      (2U)                            
#define DBGMCU_APB2_FZ_DBG_TIM21_STOP_Msk      (0x1U << DBGMCU_APB2_FZ_DBG_TIM21_STOP_Pos) /*!< 0x00000004 */
#define DBGMCU_APB2_FZ_DBG_TIM21_STOP          DBGMCU_APB2_FZ_DBG_TIM21_STOP_Msk /*!< TIM21 counter stopped when core is halted */

/******************************************************************************/
/*                                                                            */
/*                           DMA Controller (DMA)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for DMA_ISR register  ********************/
#define DMA_ISR_GIF1_Pos       (0U)                                            
#define DMA_ISR_GIF1_Msk       (0x1U << DMA_ISR_GIF1_Pos)                      /*!< 0x00000001 */
#define DMA_ISR_GIF1           DMA_ISR_GIF1_Msk                                /*!< Channel 1 Global interrupt flag    */
#define DMA_ISR_TCIF1_Pos      (1U)                                            
#define DMA_ISR_TCIF1_Msk      (0x1U << DMA_ISR_TCIF1_Pos)                     /*!< 0x00000002 */
#define DMA_ISR_TCIF1          DMA_ISR_TCIF1_Msk                               /*!< Channel 1 Transfer Complete flag   */
#define DMA_ISR_HTIF1_Pos      (2U)                                            
#define DMA_ISR_HTIF1_Msk      (0x1U << DMA_ISR_HTIF1_Pos)                     /*!< 0x00000004 */
#define DMA_ISR_HTIF1          DMA_ISR_HTIF1_Msk                               /*!< Channel 1 Half Transfer flag       */
#define DMA_ISR_TEIF1_Pos      (3U)                                            
#define DMA_ISR_TEIF1_Msk      (0x1U << DMA_ISR_TEIF1_Pos)                     /*!< 0x00000008 */
#define DMA_ISR_TEIF1          DMA_ISR_TEIF1_Msk                               /*!< Channel 1 Transfer Error flag      */
#define DMA_ISR_GIF2_Pos       (4U)                                            
#define DMA_ISR_GIF2_Msk       (0x1U << DMA_ISR_GIF2_Pos)                      /*!< 0x00000010 */
#define DMA_ISR_GIF2           DMA_ISR_GIF2_Msk                                /*!< Channel 2 Global interrupt flag    */
#define DMA_ISR_TCIF2_Pos      (5U)                                            
#define DMA_ISR_TCIF2_Msk      (0x1U << DMA_ISR_TCIF2_Pos)                     /*!< 0x00000020 */
#define DMA_ISR_TCIF2          DMA_ISR_TCIF2_Msk                               /*!< Channel 2 Transfer Complete flag   */
#define DMA_ISR_HTIF2_Pos      (6U)                                            
#define DMA_ISR_HTIF2_Msk      (0x1U << DMA_ISR_HTIF2_Pos)                     /*!< 0x00000040 */
#define DMA_ISR_HTIF2          DMA_ISR_HTIF2_Msk                               /*!< Channel 2 Half Transfer flag       */
#define DMA_ISR_TEIF2_Pos      (7U)                                            
#define DMA_ISR_TEIF2_Msk      (0x1U << DMA_ISR_TEIF2_Pos)                     /*!< 0x00000080 */
#define DMA_ISR_TEIF2          DMA_ISR_TEIF2_Msk                               /*!< Channel 2 Transfer Error flag      */
#define DMA_ISR_GIF3_Pos       (8U)                                            
#define DMA_ISR_GIF3_Msk       (0x1U << DMA_ISR_GIF3_Pos)                      /*!< 0x00000100 */
#define DMA_ISR_GIF3           DMA_ISR_GIF3_Msk                                /*!< Channel 3 Global interrupt flag    */
#define DMA_ISR_TCIF3_Pos      (9U)                                            
#define DMA_ISR_TCIF3_Msk      (0x1U << DMA_ISR_TCIF3_Pos)                     /*!< 0x00000200 */
#define DMA_ISR_TCIF3          DMA_ISR_TCIF3_Msk                               /*!< Channel 3 Transfer Complete flag   */
#define DMA_ISR_HTIF3_Pos      (10U)                                           
#define DMA_ISR_HTIF3_Msk      (0x1U << DMA_ISR_HTIF3_Pos)                     /*!< 0x00000400 */
#define DMA_ISR_HTIF3          DMA_ISR_HTIF3_Msk                               /*!< Channel 3 Half Transfer flag       */
#define DMA_ISR_TEIF3_Pos      (11U)                                           
#define DMA_ISR_TEIF3_Msk      (0x1U << DMA_ISR_TEIF3_Pos)                     /*!< 0x00000800 */
#define DMA_ISR_TEIF3          DMA_ISR_TEIF3_Msk                               /*!< Channel 3 Transfer Error flag      */
#define DMA_ISR_GIF4_Pos       (12U)                                           
#define DMA_ISR_GIF4_Msk       (0x1U << DMA_ISR_GIF4_Pos)                      /*!< 0x00001000 */
#define DMA_ISR_GIF4           DMA_ISR_GIF4_Msk                                /*!< Channel 4 Global interrupt flag    */
#define DMA_ISR_TCIF4_Pos      (13U)                                           
#define DMA_ISR_TCIF4_Msk      (0x1U << DMA_ISR_TCIF4_Pos)                     /*!< 0x00002000 */
#define DMA_ISR_TCIF4          DMA_ISR_TCIF4_Msk                               /*!< Channel 4 Transfer Complete flag   */
#define DMA_ISR_HTIF4_Pos      (14U)                                           
#define DMA_ISR_HTIF4_Msk      (0x1U << DMA_ISR_HTIF4_Pos)                     /*!< 0x00004000 */
#define DMA_ISR_HTIF4          DMA_ISR_HTIF4_Msk                               /*!< Channel 4 Half Transfer flag       */
#define DMA_ISR_TEIF4_Pos      (15U)                                           
#define DMA_ISR_TEIF4_Msk      (0x1U << DMA_ISR_TEIF4_Pos)                     /*!< 0x00008000 */
#define DMA_ISR_TEIF4          DMA_ISR_TEIF4_Msk                               /*!< Channel 4 Transfer Error flag      */
#define DMA_ISR_GIF5_Pos       (16U)                                           
#define DMA_ISR_GIF5_Msk       (0x1U << DMA_ISR_GIF5_Pos)                      /*!< 0x00010000 */
#define DMA_ISR_GIF5           DMA_ISR_GIF5_Msk                                /*!< Channel 5 Global interrupt flag    */
#define DMA_ISR_TCIF5_Pos      (17U)                                           
#define DMA_ISR_TCIF5_Msk      (0x1U << DMA_ISR_TCIF5_Pos)                     /*!< 0x00020000 */
#define DMA_ISR_TCIF5          DMA_ISR_TCIF5_Msk                               /*!< Channel 5 Transfer Complete flag   */
#define DMA_ISR_HTIF5_Pos      (18U)                                           
#define DMA_ISR_HTIF5_Msk      (0x1U << DMA_ISR_HTIF5_Pos)                     /*!< 0x00040000 */
#define DMA_ISR_HTIF5          DMA_ISR_HTIF5_Msk                               /*!< Channel 5 Half Transfer flag       */
#define DMA_ISR_TEIF5_Pos      (19U)                                           
#define DMA_ISR_TEIF5_Msk      (0x1U << DMA_ISR_TEIF5_Pos)                     /*!< 0x00080000 */
#define DMA_ISR_TEIF5          DMA_ISR_TEIF5_Msk                               /*!< Channel 5 Transfer Error flag      */
#define DMA_ISR_GIF6_Pos       (20U)                                           
#define DMA_ISR_GIF6_Msk       (0x1U << DMA_ISR_GIF6_Pos)                      /*!< 0x00100000 */
#define DMA_ISR_GIF6           DMA_ISR_GIF6_Msk                                /*!< Channel 6 Global interrupt flag */
#define DMA_ISR_TCIF6_Pos      (21U)                                           
#define DMA_ISR_TCIF6_Msk      (0x1U << DMA_ISR_TCIF6_Pos)                     /*!< 0x00200000 */
#define DMA_ISR_TCIF6          DMA_ISR_TCIF6_Msk                               /*!< Channel 6 Transfer Complete flag */
#define DMA_ISR_HTIF6_Pos      (22U)                                           
#define DMA_ISR_HTIF6_Msk      (0x1U << DMA_ISR_HTIF6_Pos)                     /*!< 0x00400000 */
#define DMA_ISR_HTIF6          DMA_ISR_HTIF6_Msk                               /*!< Channel 6 Half Transfer flag */
#define DMA_ISR_TEIF6_Pos      (23U)                                           
#define DMA_ISR_TEIF6_Msk      (0x1U << DMA_ISR_TEIF6_Pos)                     /*!< 0x00800000 */
#define DMA_ISR_TEIF6          DMA_ISR_TEIF6_Msk                               /*!< Channel 6 Transfer Error flag */
#define DMA_ISR_GIF7_Pos       (24U)                                           
#define DMA_ISR_GIF7_Msk       (0x1U << DMA_ISR_GIF7_Pos)                      /*!< 0x01000000 */
#define DMA_ISR_GIF7           DMA_ISR_GIF7_Msk                                /*!< Channel 7 Global interrupt flag */
#define DMA_ISR_TCIF7_Pos      (25U)                                           
#define DMA_ISR_TCIF7_Msk      (0x1U << DMA_ISR_TCIF7_Pos)                     /*!< 0x02000000 */
#define DMA_ISR_TCIF7          DMA_ISR_TCIF7_Msk                               /*!< Channel 7 Transfer Complete flag */
#define DMA_ISR_HTIF7_Pos      (26U)                                           
#define DMA_ISR_HTIF7_Msk      (0x1U << DMA_ISR_HTIF7_Pos)                     /*!< 0x04000000 */
#define DMA_ISR_HTIF7          DMA_ISR_HTIF7_Msk                               /*!< Channel 7 Half Transfer flag */
#define DMA_ISR_TEIF7_Pos      (27U)                                           
#define DMA_ISR_TEIF7_Msk      (0x1U << DMA_ISR_TEIF7_Pos)                     /*!< 0x08000000 */
#define DMA_ISR_TEIF7          DMA_ISR_TEIF7_Msk                               /*!< Channel 7 Transfer Error flag */

/*******************  Bit definition for DMA_IFCR register  *******************/
#define DMA_IFCR_CGIF1_Pos     (0U)                                            
#define DMA_IFCR_CGIF1_Msk     (0x1U << DMA_IFCR_CGIF1_Pos)                    /*!< 0x00000001 */
#define DMA_IFCR_CGIF1         DMA_IFCR_CGIF1_Msk                              /*!< Channel 1 Global interrupt clear    */
#define DMA_IFCR_CTCIF1_Pos    (1U)                                            
#define DMA_IFCR_CTCIF1_Msk    (0x1U << DMA_IFCR_CTCIF1_Pos)                   /*!< 0x00000002 */
#define DMA_IFCR_CTCIF1        DMA_IFCR_CTCIF1_Msk                             /*!< Channel 1 Transfer Complete clear   */
#define DMA_IFCR_CHTIF1_Pos    (2U)                                            
#define DMA_IFCR_CHTIF1_Msk    (0x1U << DMA_IFCR_CHTIF1_Pos)                   /*!< 0x00000004 */
#define DMA_IFCR_CHTIF1        DMA_IFCR_CHTIF1_Msk                             /*!< Channel 1 Half Transfer clear       */
#define DMA_IFCR_CTEIF1_Pos    (3U)                                            
#define DMA_IFCR_CTEIF1_Msk    (0x1U << DMA_IFCR_CTEIF1_Pos)                   /*!< 0x00000008 */
#define DMA_IFCR_CTEIF1        DMA_IFCR_CTEIF1_Msk                             /*!< Channel 1 Transfer Error clear      */
#define DMA_IFCR_CGIF2_Pos     (4U)                                            
#define DMA_IFCR_CGIF2_Msk     (0x1U << DMA_IFCR_CGIF2_Pos)                    /*!< 0x00000010 */
#define DMA_IFCR_CGIF2         DMA_IFCR_CGIF2_Msk                              /*!< Channel 2 Global interrupt clear    */
#define DMA_IFCR_CTCIF2_Pos    (5U)                                            
#define DMA_IFCR_CTCIF2_Msk    (0x1U << DMA_IFCR_CTCIF2_Pos)                   /*!< 0x00000020 */
#define DMA_IFCR_CTCIF2        DMA_IFCR_CTCIF2_Msk                             /*!< Channel 2 Transfer Complete clear   */
#define DMA_IFCR_CHTIF2_Pos    (6U)                                            
#define DMA_IFCR_CHTIF2_Msk    (0x1U << DMA_IFCR_CHTIF2_Pos)                   /*!< 0x00000040 */
#define DMA_IFCR_CHTIF2        DMA_IFCR_CHTIF2_Msk                             /*!< Channel 2 Half Transfer clear       */
#define DMA_IFCR_CTEIF2_Pos    (7U)                                            
#define DMA_IFCR_CTEIF2_Msk    (0x1U << DMA_IFCR_CTEIF2_Pos)                   /*!< 0x00000080 */
#define DMA_IFCR_CTEIF2        DMA_IFCR_CTEIF2_Msk                             /*!< Channel 2 Transfer Error clear      */
#define DMA_IFCR_CGIF3_Pos     (8U)                                            
#define DMA_IFCR_CGIF3_Msk     (0x1U << DMA_IFCR_CGIF3_Pos)                    /*!< 0x00000100 */
#define DMA_IFCR_CGIF3         DMA_IFCR_CGIF3_Msk                              /*!< Channel 3 Global interrupt clear    */
#define DMA_IFCR_CTCIF3_Pos    (9U)                                            
#define DMA_IFCR_CTCIF3_Msk    (0x1U << DMA_IFCR_CTCIF3_Pos)                   /*!< 0x00000200 */
#define DMA_IFCR_CTCIF3        DMA_IFCR_CTCIF3_Msk                             /*!< Channel 3 Transfer Complete clear   */
#define DMA_IFCR_CHTIF3_Pos    (10U)                                           
#define DMA_IFCR_CHTIF3_Msk    (0x1U << DMA_IFCR_CHTIF3_Pos)                   /*!< 0x00000400 */
#define DMA_IFCR_CHTIF3        DMA_IFCR_CHTIF3_Msk                             /*!< Channel 3 Half Transfer clear       */
#define DMA_IFCR_CTEIF3_Pos    (11U)                                           
#define DMA_IFCR_CTEIF3_Msk    (0x1U << DMA_IFCR_CTEIF3_Pos)                   /*!< 0x00000800 */
#define DMA_IFCR_CTEIF3        DMA_IFCR_CTEIF3_Msk                             /*!< Channel 3 Transfer Error clear      */
#define DMA_IFCR_CGIF4_Pos     (12U)                                           
#define DMA_IFCR_CGIF4_Msk     (0x1U << DMA_IFCR_CGIF4_Pos)                    /*!< 0x00001000 */
#define DMA_IFCR_CGIF4         DMA_IFCR_CGIF4_Msk                              /*!< Channel 4 Global interrupt clear    */
#define DMA_IFCR_CTCIF4_Pos    (13U)                                           
#define DMA_IFCR_CTCIF4_Msk    (0x1U << DMA_IFCR_CTCIF4_Pos)                   /*!< 0x00002000 */
#define DMA_IFCR_CTCIF4        DMA_IFCR_CTCIF4_Msk                             /*!< Channel 4 Transfer Complete clear   */
#define DMA_IFCR_CHTIF4_Pos    (14U)                                           
#define DMA_IFCR_CHTIF4_Msk    (0x1U << DMA_IFCR_CHTIF4_Pos)                   /*!< 0x00004000 */
#define DMA_IFCR_CHTIF4        DMA_IFCR_CHTIF4_Msk                             /*!< Channel 4 Half Transfer clear       */
#define DMA_IFCR_CTEIF4_Pos    (15U)                                           
#define DMA_IFCR_CTEIF4_Msk    (0x1U << DMA_IFCR_CTEIF4_Pos)                   /*!< 0x00008000 */
#define DMA_IFCR_CTEIF4        DMA_IFCR_CTEIF4_Msk                             /*!< Channel 4 Transfer Error clear      */
#define DMA_IFCR_CGIF5_Pos     (16U)                                           
#define DMA_IFCR_CGIF5_Msk     (0x1U << DMA_IFCR_CGIF5_Pos)                    /*!< 0x00010000 */
#define DMA_IFCR_CGIF5         DMA_IFCR_CGIF5_Msk                              /*!< Channel 5 Global interrupt clear    */
#define DMA_IFCR_CTCIF5_Pos    (17U)                                           
#define DMA_IFCR_CTCIF5_Msk    (0x1U << DMA_IFCR_CTCIF5_Pos)                   /*!< 0x00020000 */
#define DMA_IFCR_CTCIF5        DMA_IFCR_CTCIF5_Msk                             /*!< Channel 5 Transfer Complete clear   */
#define DMA_IFCR_CHTIF5_Pos    (18U)                                           
#define DMA_IFCR_CHTIF5_Msk    (0x1U << DMA_IFCR_CHTIF5_Pos)                   /*!< 0x00040000 */
#define DMA_IFCR_CHTIF5        DMA_IFCR_CHTIF5_Msk                             /*!< Channel 5 Half Transfer clear       */
#define DMA_IFCR_CTEIF5_Pos    (19U)                                           
#define DMA_IFCR_CTEIF5_Msk    (0x1U << DMA_IFCR_CTEIF5_Pos)                   /*!< 0x00080000 */
#define DMA_IFCR_CTEIF5        DMA_IFCR_CTEIF5_Msk                             /*!< Channel 5 Transfer Error clear      */
#define DMA_IFCR_CGIF6_Pos     (20U)                                           
#define DMA_IFCR_CGIF6_Msk     (0x1U << DMA_IFCR_CGIF6_Pos)                    /*!< 0x00100000 */
#define DMA_IFCR_CGIF6         DMA_IFCR_CGIF6_Msk                              /*!< Channel 6 Global interrupt clear */
#define DMA_IFCR_CTCIF6_Pos    (21U)                                           
#define DMA_IFCR_CTCIF6_Msk    (0x1U << DMA_IFCR_CTCIF6_Pos)                   /*!< 0x00200000 */
#define DMA_IFCR_CTCIF6        DMA_IFCR_CTCIF6_Msk                             /*!< Channel 6 Transfer Complete clear */
#define DMA_IFCR_CHTIF6_Pos    (22U)                                           
#define DMA_IFCR_CHTIF6_Msk    (0x1U << DMA_IFCR_CHTIF6_Pos)                   /*!< 0x00400000 */
#define DMA_IFCR_CHTIF6        DMA_IFCR_CHTIF6_Msk                             /*!< Channel 6 Half Transfer clear */
#define DMA_IFCR_CTEIF6_Pos    (23U)                                           
#define DMA_IFCR_CTEIF6_Msk    (0x1U << DMA_IFCR_CTEIF6_Pos)                   /*!< 0x00800000 */
#define DMA_IFCR_CTEIF6        DMA_IFCR_CTEIF6_Msk                             /*!< Channel 6 Transfer Error clear */
#define DMA_IFCR_CGIF7_Pos     (24U)                                           
#define DMA_IFCR_CGIF7_Msk     (0x1U << DMA_IFCR_CGIF7_Pos)                    /*!< 0x01000000 */
#define DMA_IFCR_CGIF7         DMA_IFCR_CGIF7_Msk                              /*!< Channel 7 Global interrupt clear */
#define DMA_IFCR_CTCIF7_Pos    (25U)                                           
#define DMA_IFCR_CTCIF7_Msk    (0x1U << DMA_IFCR_CTCIF7_Pos)                   /*!< 0x02000000 */
#define DMA_IFCR_CTCIF7        DMA_IFCR_CTCIF7_Msk                             /*!< Channel 7 Transfer Complete clear */
#define DMA_IFCR_CHTIF7_Pos    (26U)                                           
#define DMA_IFCR_CHTIF7_Msk    (0x1U << DMA_IFCR_CHTIF7_Pos)                   /*!< 0x04000000 */
#define DMA_IFCR_CHTIF7        DMA_IFCR_CHTIF7_Msk                             /*!< Channel 7 Half Transfer clear */
#define DMA_IFCR_CTEIF7_Pos    (27U)                                           
#define DMA_IFCR_CTEIF7_Msk    (0x1U << DMA_IFCR_CTEIF7_Pos)                   /*!< 0x08000000 */
#define DMA_IFCR_CTEIF7        DMA_IFCR_CTEIF7_Msk                             /*!< Channel 7 Transfer Error clear */

/*******************  Bit definition for DMA_CCR register  ********************/
#define DMA_CCR_EN_Pos         (0U)                                            
#define DMA_CCR_EN_Msk         (0x1U << DMA_CCR_EN_Pos)                        /*!< 0x00000001 */
#define DMA_CCR_EN             DMA_CCR_EN_Msk                                  /*!< Channel enable                      */
#define DMA_CCR_TCIE_Pos       (1U)                                            
#define DMA_CCR_TCIE_Msk       (0x1U << DMA_CCR_TCIE_Pos)                      /*!< 0x00000002 */
#define DMA_CCR_TCIE           DMA_CCR_TCIE_Msk                                /*!< Transfer complete interrupt enable  */
#define DMA_CCR_HTIE_Pos       (2U)                                            
#define DMA_CCR_HTIE_Msk       (0x1U << DMA_CCR_HTIE_Pos)                      /*!< 0x00000004 */
#define DMA_CCR_HTIE           DMA_CCR_HTIE_Msk                                /*!< Half Transfer interrupt enable      */
#define DMA_CCR_TEIE_Pos       (3U)                                            
#define DMA_CCR_TEIE_Msk       (0x1U << DMA_CCR_TEIE_Pos)                      /*!< 0x00000008 */
#define DMA_CCR_TEIE           DMA_CCR_TEIE_Msk                                /*!< Transfer error interrupt enable     */
#define DMA_CCR_DIR_Pos        (4U)                                            
#define DMA_CCR_DIR_Msk        (0x1U << DMA_CCR_DIR_Pos)                       /*!< 0x00000010 */
#define DMA_CCR_DIR            DMA_CCR_DIR_Msk                                 /*!< Data transfer direction             */
#define DMA_CCR_CIRC_Pos       (5U)                                            
#define DMA_CCR_CIRC_Msk       (0x1U << DMA_CCR_CIRC_Pos)                      /*!< 0x00000020 */
#define DMA_CCR_CIRC           DMA_CCR_CIRC_Msk                                /*!< Circular mode                       */
#define DMA_CCR_PINC_Pos       (6U)                                            
#define DMA_CCR_PINC_Msk       (0x1U << DMA_CCR_PINC_Pos)                      /*!< 0x00000040 */
#define DMA_CCR_PINC           DMA_CCR_PINC_Msk                                /*!< Peripheral increment mode           */
#define DMA_CCR_MINC_Pos       (7U)                                            
#define DMA_CCR_MINC_Msk       (0x1U << DMA_CCR_MINC_Pos)                      /*!< 0x00000080 */
#define DMA_CCR_MINC           DMA_CCR_MINC_Msk                                /*!< Memory increment mode               */

#define DMA_CCR_PSIZE_Pos      (8U)                                            
#define DMA_CCR_PSIZE_Msk      (0x3U << DMA_CCR_PSIZE_Pos)                     /*!< 0x00000300 */
#define DMA_CCR_PSIZE          DMA_CCR_PSIZE_Msk                               /*!< PSIZE[1:0] bits (Peripheral size)   */
#define DMA_CCR_PSIZE_0        (0x1U << DMA_CCR_PSIZE_Pos)                     /*!< 0x00000100 */
#define DMA_CCR_PSIZE_1        (0x2U << DMA_CCR_PSIZE_Pos)                     /*!< 0x00000200 */

#define DMA_CCR_MSIZE_Pos      (10U)                                           
#define DMA_CCR_MSIZE_Msk      (0x3U << DMA_CCR_MSIZE_Pos)                     /*!< 0x00000C00 */
#define DMA_CCR_MSIZE          DMA_CCR_MSIZE_Msk                               /*!< MSIZE[1:0] bits (Memory size)       */
#define DMA_CCR_MSIZE_0        (0x1U << DMA_CCR_MSIZE_Pos)                     /*!< 0x00000400 */
#define DMA_CCR_MSIZE_1        (0x2U << DMA_CCR_MSIZE_Pos)                     /*!< 0x00000800 */

#define DMA_CCR_PL_Pos         (12U)                                           
#define DMA_CCR_PL_Msk         (0x3U << DMA_CCR_PL_Pos)                        /*!< 0x00003000 */
#define DMA_CCR_PL             DMA_CCR_PL_Msk                                  /*!< PL[1:0] bits(Channel Priority level)*/
#define DMA_CCR_PL_0           (0x1U << DMA_CCR_PL_Pos)                        /*!< 0x00001000 */
#define DMA_CCR_PL_1           (0x2U << DMA_CCR_PL_Pos)                        /*!< 0x00002000 */

#define DMA_CCR_MEM2MEM_Pos    (14U)                                           
#define DMA_CCR_MEM2MEM_Msk    (0x1U << DMA_CCR_MEM2MEM_Pos)                   /*!< 0x00004000 */
#define DMA_CCR_MEM2MEM        DMA_CCR_MEM2MEM_Msk                             /*!< Memory to memory mode               */

/******************  Bit definition for DMA_CNDTR register  *******************/
#define DMA_CNDTR_NDT_Pos      (0U)                                            
#define DMA_CNDTR_NDT_Msk      (0xFFFFU << DMA_CNDTR_NDT_Pos)                  /*!< 0x0000FFFF */
#define DMA_CNDTR_NDT          DMA_CNDTR_NDT_Msk                               /*!< Number of data to Transfer          */

/******************  Bit definition for DMA_CPAR register  ********************/
#define DMA_CPAR_PA_Pos        (0U)                                            
#define DMA_CPAR_PA_Msk        (0xFFFFFFFFU << DMA_CPAR_PA_Pos)                /*!< 0xFFFFFFFF */
#define DMA_CPAR_PA            DMA_CPAR_PA_Msk                                 /*!< Peripheral Address                  */

/******************  Bit definition for DMA_CMAR register  ********************/
#define DMA_CMAR_MA_Pos        (0U)                                            
#define DMA_CMAR_MA_Msk        (0xFFFFFFFFU << DMA_CMAR_MA_Pos)                /*!< 0xFFFFFFFF */
#define DMA_CMAR_MA            DMA_CMAR_MA_Msk                                 /*!< Memory Address                      */


/*******************  Bit definition for DMA_CSELR register  *******************/
#define DMA_CSELR_C1S_Pos      (0U)                                            
#define DMA_CSELR_C1S_Msk      (0xFU << DMA_CSELR_C1S_Pos)                     /*!< 0x0000000F */
#define DMA_CSELR_C1S          DMA_CSELR_C1S_Msk                               /*!< Channel 1 Selection */ 
#define DMA_CSELR_C2S_Pos      (4U)                                            
#define DMA_CSELR_C2S_Msk      (0xFU << DMA_CSELR_C2S_Pos)                     /*!< 0x000000F0 */
#define DMA_CSELR_C2S          DMA_CSELR_C2S_Msk                               /*!< Channel 2 Selection */ 
#define DMA_CSELR_C3S_Pos      (8U)                                            
#define DMA_CSELR_C3S_Msk      (0xFU << DMA_CSELR_C3S_Pos)                     /*!< 0x00000F00 */
#define DMA_CSELR_C3S          DMA_CSELR_C3S_Msk                               /*!< Channel 3 Selection */ 
#define DMA_CSELR_C4S_Pos      (12U)                                           
#define DMA_CSELR_C4S_Msk      (0xFU << DMA_CSELR_C4S_Pos)                     /*!< 0x0000F000 */
#define DMA_CSELR_C4S          DMA_CSELR_C4S_Msk                               /*!< Channel 4 Selection */ 
#define DMA_CSELR_C5S_Pos      (16U)                                           
#define DMA_CSELR_C5S_Msk      (0xFU << DMA_CSELR_C5S_Pos)                     /*!< 0x000F0000 */
#define DMA_CSELR_C5S          DMA_CSELR_C5S_Msk                               /*!< Channel 5 Selection */ 
#define DMA_CSELR_C6S_Pos      (20U)                                           
#define DMA_CSELR_C6S_Msk      (0xFU << DMA_CSELR_C6S_Pos)                     /*!< 0x00F00000 */
#define DMA_CSELR_C6S          DMA_CSELR_C6S_Msk                               /*!< Channel 6 Selection */ 
#define DMA_CSELR_C7S_Pos      (24U)                                           
#define DMA_CSELR_C7S_Msk      (0xFU << DMA_CSELR_C7S_Pos)                     /*!< 0x0F000000 */
#define DMA_CSELR_C7S          DMA_CSELR_C7S_Msk                               /*!< Channel 7 Selection */

/******************************************************************************/
/*                                                                            */
/*                 External Interrupt/Event Controller (EXTI)                 */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for EXTI_IMR register  *******************/
#define EXTI_IMR_IM0_Pos        (0U)                                           
#define EXTI_IMR_IM0_Msk        (0x1U << EXTI_IMR_IM0_Pos)                     /*!< 0x00000001 */
#define EXTI_IMR_IM0            EXTI_IMR_IM0_Msk                               /*!< Interrupt Mask on line 0  */
#define EXTI_IMR_IM1_Pos        (1U)                                           
#define EXTI_IMR_IM1_Msk        (0x1U << EXTI_IMR_IM1_Pos)                     /*!< 0x00000002 */
#define EXTI_IMR_IM1            EXTI_IMR_IM1_Msk                               /*!< Interrupt Mask on line 1  */
#define EXTI_IMR_IM2_Pos        (2U)                                           
#define EXTI_IMR_IM2_Msk        (0x1U << EXTI_IMR_IM2_Pos)                     /*!< 0x00000004 */
#define EXTI_IMR_IM2            EXTI_IMR_IM2_Msk                               /*!< Interrupt Mask on line 2  */
#define EXTI_IMR_IM3_Pos        (3U)                                           
#define EXTI_IMR_IM3_Msk        (0x1U << EXTI_IMR_IM3_Pos)                     /*!< 0x00000008 */
#define EXTI_IMR_IM3            EXTI_IMR_IM3_Msk                               /*!< Interrupt Mask on line 3  */
#define EXTI_IMR_IM4_Pos        (4U)                                           
#define EXTI_IMR_IM4_Msk        (0x1U << EXTI_IMR_IM4_Pos)                     /*!< 0x00000010 */
#define EXTI_IMR_IM4            EXTI_IMR_IM4_Msk                               /*!< Interrupt Mask on line 4  */
#define EXTI_IMR_IM5_Pos        (5U)                                           
#define EXTI_IMR_IM5_Msk        (0x1U << EXTI_IMR_IM5_Pos)                     /*!< 0x00000020 */
#define EXTI_IMR_IM5            EXTI_IMR_IM5_Msk                               /*!< Interrupt Mask on line 5  */
#define EXTI_IMR_IM6_Pos        (6U)                                           
#define EXTI_IMR_IM6_Msk        (0x1U << EXTI_IMR_IM6_Pos)                     /*!< 0x00000040 */
#define EXTI_IMR_IM6            EXTI_IMR_IM6_Msk                               /*!< Interrupt Mask on line 6  */
#define EXTI_IMR_IM7_Pos        (7U)                                           
#define EXTI_IMR_IM7_Msk        (0x1U << EXTI_IMR_IM7_Pos)                     /*!< 0x00000080 */
#define EXTI_IMR_IM7            EXTI_IMR_IM7_Msk                               /*!< Interrupt Mask on line 7  */
#define EXTI_IMR_IM8_Pos        (8U)                                           
#define EXTI_IMR_IM8_Msk        (0x1U << EXTI_IMR_IM8_Pos)                     /*!< 0x00000100 */
#define EXTI_IMR_IM8            EXTI_IMR_IM8_Msk                               /*!< Interrupt Mask on line 8  */
#define EXTI_IMR_IM9_Pos        (9U)                                           
#define EXTI_IMR_IM9_Msk        (0x1U << EXTI_IMR_IM9_Pos)                     /*!< 0x00000200 */
#define EXTI_IMR_IM9            EXTI_IMR_IM9_Msk                               /*!< Interrupt Mask on line 9  */
#define EXTI_IMR_IM10_Pos       (10U)                                          
#define EXTI_IMR_IM10_Msk       (0x1U << EXTI_IMR_IM10_Pos)                    /*!< 0x00000400 */
#define EXTI_IMR_IM10           EXTI_IMR_IM10_Msk                              /*!< Interrupt Mask on line 10 */
#define EXTI_IMR_IM11_Pos       (11U)                                          
#define EXTI_IMR_IM11_Msk       (0x1U << EXTI_IMR_IM11_Pos)                    /*!< 0x00000800 */
#define EXTI_IMR_IM11           EXTI_IMR_IM11_Msk                              /*!< Interrupt Mask on line 11 */
#define EXTI_IMR_IM12_Pos       (12U)                                          
#define EXTI_IMR_IM12_Msk       (0x1U << EXTI_IMR_IM12_Pos)                    /*!< 0x00001000 */
#define EXTI_IMR_IM12           EXTI_IMR_IM12_Msk                              /*!< Interrupt Mask on line 12 */
#define EXTI_IMR_IM13_Pos       (13U)                                          
#define EXTI_IMR_IM13_Msk       (0x1U << EXTI_IMR_IM13_Pos)                    /*!< 0x00002000 */
#define EXTI_IMR_IM13           EXTI_IMR_IM13_Msk                              /*!< Interrupt Mask on line 13 */
#define EXTI_IMR_IM14_Pos       (14U)                                          
#define EXTI_IMR_IM14_Msk       (0x1U << EXTI_IMR_IM14_Pos)                    /*!< 0x00004000 */
#define EXTI_IMR_IM14           EXTI_IMR_IM14_Msk                              /*!< Interrupt Mask on line 14 */
#define EXTI_IMR_IM15_Pos       (15U)                                          
#define EXTI_IMR_IM15_Msk       (0x1U << EXTI_IMR_IM15_Pos)                    /*!< 0x00008000 */
#define EXTI_IMR_IM15           EXTI_IMR_IM15_Msk                              /*!< Interrupt Mask on line 15 */
#define EXTI_IMR_IM16_Pos       (16U)                                          
#define EXTI_IMR_IM16_Msk       (0x1U << EXTI_IMR_IM16_Pos)                    /*!< 0x00010000 */
#define EXTI_IMR_IM16           EXTI_IMR_IM16_Msk                              /*!< Interrupt Mask on line 16 */
#define EXTI_IMR_IM17_Pos       (17U)                                          
#define EXTI_IMR_IM17_Msk       (0x1U << EXTI_IMR_IM17_Pos)                    /*!< 0x00020000 */
#define EXTI_IMR_IM17           EXTI_IMR_IM17_Msk                              /*!< Interrupt Mask on line 17 */
#define EXTI_IMR_IM18_Pos       (18U)                                          
#define EXTI_IMR_IM18_Msk       (0x1U << EXTI_IMR_IM18_Pos)                    /*!< 0x00040000 */
#define EXTI_IMR_IM18           EXTI_IMR_IM18_Msk                              /*!< Interrupt Mask on line 18 */
#define EXTI_IMR_IM19_Pos       (19U)                                          
#define EXTI_IMR_IM19_Msk       (0x1U << EXTI_IMR_IM19_Pos)                    /*!< 0x00080000 */
#define EXTI_IMR_IM19           EXTI_IMR_IM19_Msk                              /*!< Interrupt Mask on line 19 */
#define EXTI_IMR_IM20_Pos       (20U)                                          
#define EXTI_IMR_IM20_Msk       (0x1U << EXTI_IMR_IM20_Pos)                    /*!< 0x00100000 */
#define EXTI_IMR_IM20           EXTI_IMR_IM20_Msk                              /*!< Interrupt Mask on line 20 */
#define EXTI_IMR_IM21_Pos       (21U)                                          
#define EXTI_IMR_IM21_Msk       (0x1U << EXTI_IMR_IM21_Pos)                    /*!< 0x00200000 */
#define EXTI_IMR_IM21           EXTI_IMR_IM21_Msk                              /*!< Interrupt Mask on line 21 */
#define EXTI_IMR_IM22_Pos       (22U)                                          
#define EXTI_IMR_IM22_Msk       (0x1U << EXTI_IMR_IM22_Pos)                    /*!< 0x00400000 */
#define EXTI_IMR_IM22           EXTI_IMR_IM22_Msk                              /*!< Interrupt Mask on line 22 */
#define EXTI_IMR_IM23_Pos       (23U)                                          
#define EXTI_IMR_IM23_Msk       (0x1U << EXTI_IMR_IM23_Pos)                    /*!< 0x00800000 */
#define EXTI_IMR_IM23           EXTI_IMR_IM23_Msk                              /*!< Interrupt Mask on line 23 */
#define EXTI_IMR_IM25_Pos       (25U)                                          
#define EXTI_IMR_IM25_Msk       (0x1U << EXTI_IMR_IM25_Pos)                    /*!< 0x02000000 */
#define EXTI_IMR_IM25           EXTI_IMR_IM25_Msk                              /*!< Interrupt Mask on line 25 */
#define EXTI_IMR_IM26_Pos       (26U)                                          
#define EXTI_IMR_IM26_Msk       (0x1U << EXTI_IMR_IM26_Pos)                    /*!< 0x04000000 */
#define EXTI_IMR_IM26           EXTI_IMR_IM26_Msk                              /*!< Interrupt Mask on line 26 */
#define EXTI_IMR_IM28_Pos       (28U)                                          
#define EXTI_IMR_IM28_Msk       (0x1U << EXTI_IMR_IM28_Pos)                    /*!< 0x10000000 */
#define EXTI_IMR_IM28           EXTI_IMR_IM28_Msk                              /*!< Interrupt Mask on line 28 */
#define EXTI_IMR_IM29_Pos       (29U)                                          
#define EXTI_IMR_IM29_Msk       (0x1U << EXTI_IMR_IM29_Pos)                    /*!< 0x20000000 */
#define EXTI_IMR_IM29           EXTI_IMR_IM29_Msk                              /*!< Interrupt Mask on line 29 */

#define EXTI_IMR_IM_Pos         (0U)                                           
#define EXTI_IMR_IM_Msk         (0x36FFFFFFU << EXTI_IMR_IM_Pos)               /*!< 0x36FFFFFF */
#define EXTI_IMR_IM             EXTI_IMR_IM_Msk                                /*!< Interrupt Mask All */

/******************  Bit definition for EXTI_EMR register  ********************/
#define EXTI_EMR_EM0_Pos        (0U)                                           
#define EXTI_EMR_EM0_Msk        (0x1U << EXTI_EMR_EM0_Pos)                     /*!< 0x00000001 */
#define EXTI_EMR_EM0            EXTI_EMR_EM0_Msk                               /*!< Event Mask on line 0  */
#define EXTI_EMR_EM1_Pos        (1U)                                           
#define EXTI_EMR_EM1_Msk        (0x1U << EXTI_EMR_EM1_Pos)                     /*!< 0x00000002 */
#define EXTI_EMR_EM1            EXTI_EMR_EM1_Msk                               /*!< Event Mask on line 1  */
#define EXTI_EMR_EM2_Pos        (2U)                                           
#define EXTI_EMR_EM2_Msk        (0x1U << EXTI_EMR_EM2_Pos)                     /*!< 0x00000004 */
#define EXTI_EMR_EM2            EXTI_EMR_EM2_Msk                               /*!< Event Mask on line 2  */
#define EXTI_EMR_EM3_Pos        (3U)                                           
#define EXTI_EMR_EM3_Msk        (0x1U << EXTI_EMR_EM3_Pos)                     /*!< 0x00000008 */
#define EXTI_EMR_EM3            EXTI_EMR_EM3_Msk                               /*!< Event Mask on line 3  */
#define EXTI_EMR_EM4_Pos        (4U)                                           
#define EXTI_EMR_EM4_Msk        (0x1U << EXTI_EMR_EM4_Pos)                     /*!< 0x00000010 */
#define EXTI_EMR_EM4            EXTI_EMR_EM4_Msk                               /*!< Event Mask on line 4  */
#define EXTI_EMR_EM5_Pos        (5U)                                           
#define EXTI_EMR_EM5_Msk        (0x1U << EXTI_EMR_EM5_Pos)                     /*!< 0x00000020 */
#define EXTI_EMR_EM5            EXTI_EMR_EM5_Msk                               /*!< Event Mask on line 5  */
#define EXTI_EMR_EM6_Pos        (6U)                                           
#define EXTI_EMR_EM6_Msk        (0x1U << EXTI_EMR_EM6_Pos)                     /*!< 0x00000040 */
#define EXTI_EMR_EM6            EXTI_EMR_EM6_Msk                               /*!< Event Mask on line 6  */
#define EXTI_EMR_EM7_Pos        (7U)                                           
#define EXTI_EMR_EM7_Msk        (0x1U << EXTI_EMR_EM7_Pos)                     /*!< 0x00000080 */
#define EXTI_EMR_EM7            EXTI_EMR_EM7_Msk                               /*!< Event Mask on line 7  */
#define EXTI_EMR_EM8_Pos        (8U)                                           
#define EXTI_EMR_EM8_Msk        (0x1U << EXTI_EMR_EM8_Pos)                     /*!< 0x00000100 */
#define EXTI_EMR_EM8            EXTI_EMR_EM8_Msk                               /*!< Event Mask on line 8  */
#define EXTI_EMR_EM9_Pos        (9U)                                           
#define EXTI_EMR_EM9_Msk        (0x1U << EXTI_EMR_EM9_Pos)                     /*!< 0x00000200 */
#define EXTI_EMR_EM9            EXTI_EMR_EM9_Msk                               /*!< Event Mask on line 9  */
#define EXTI_EMR_EM10_Pos       (10U)                                          
#define EXTI_EMR_EM10_Msk       (0x1U << EXTI_EMR_EM10_Pos)                    /*!< 0x00000400 */
#define EXTI_EMR_EM10           EXTI_EMR_EM10_Msk                              /*!< Event Mask on line 10 */
#define EXTI_EMR_EM11_Pos       (11U)                                          
#define EXTI_EMR_EM11_Msk       (0x1U << EXTI_EMR_EM11_Pos)                    /*!< 0x00000800 */
#define EXTI_EMR_EM11           EXTI_EMR_EM11_Msk                              /*!< Event Mask on line 11 */
#define EXTI_EMR_EM12_Pos       (12U)                                          
#define EXTI_EMR_EM12_Msk       (0x1U << EXTI_EMR_EM12_Pos)                    /*!< 0x00001000 */
#define EXTI_EMR_EM12           EXTI_EMR_EM12_Msk                              /*!< Event Mask on line 12 */
#define EXTI_EMR_EM13_Pos       (13U)                                          
#define EXTI_EMR_EM13_Msk       (0x1U << EXTI_EMR_EM13_Pos)                    /*!< 0x00002000 */
#define EXTI_EMR_EM13           EXTI_EMR_EM13_Msk                              /*!< Event Mask on line 13 */
#define EXTI_EMR_EM14_Pos       (14U)                                          
#define EXTI_EMR_EM14_Msk       (0x1U << EXTI_EMR_EM14_Pos)                    /*!< 0x00004000 */
#define EXTI_EMR_EM14           EXTI_EMR_EM14_Msk                              /*!< Event Mask on line 14 */
#define EXTI_EMR_EM15_Pos       (15U)                                          
#define EXTI_EMR_EM15_Msk       (0x1U << EXTI_EMR_EM15_Pos)                    /*!< 0x00008000 */
#define EXTI_EMR_EM15           EXTI_EMR_EM15_Msk                              /*!< Event Mask on line 15 */
#define EXTI_EMR_EM16_Pos       (16U)                                          
#define EXTI_EMR_EM16_Msk       (0x1U << EXTI_EMR_EM16_Pos)                    /*!< 0x00010000 */
#define EXTI_EMR_EM16           EXTI_EMR_EM16_Msk                              /*!< Event Mask on line 16 */
#define EXTI_EMR_EM17_Pos       (17U)                                          
#define EXTI_EMR_EM17_Msk       (0x1U << EXTI_EMR_EM17_Pos)                    /*!< 0x00020000 */
#define EXTI_EMR_EM17           EXTI_EMR_EM17_Msk                              /*!< Event Mask on line 17 */
#define EXTI_EMR_EM18_Pos       (18U)                                          
#define EXTI_EMR_EM18_Msk       (0x1U << EXTI_EMR_EM18_Pos)                    /*!< 0x00040000 */
#define EXTI_EMR_EM18           EXTI_EMR_EM18_Msk                              /*!< Event Mask on line 18 */
#define EXTI_EMR_EM19_Pos       (19U)                                          
#define EXTI_EMR_EM19_Msk       (0x1U << EXTI_EMR_EM19_Pos)                    /*!< 0x00080000 */
#define EXTI_EMR_EM19           EXTI_EMR_EM19_Msk                              /*!< Event Mask on line 19 */
#define EXTI_EMR_EM20_Pos       (20U)                                          
#define EXTI_EMR_EM20_Msk       (0x1U << EXTI_EMR_EM20_Pos)                    /*!< 0x00100000 */
#define EXTI_EMR_EM20           EXTI_EMR_EM20_Msk                              /*!< Event Mask on line 20 */
#define EXTI_EMR_EM21_Pos       (21U)                                          
#define EXTI_EMR_EM21_Msk       (0x1U << EXTI_EMR_EM21_Pos)                    /*!< 0x00200000 */
#define EXTI_EMR_EM21           EXTI_EMR_EM21_Msk                              /*!< Event Mask on line 21 */
#define EXTI_EMR_EM22_Pos       (22U)                                          
#define EXTI_EMR_EM22_Msk       (0x1U << EXTI_EMR_EM22_Pos)                    /*!< 0x00400000 */
#define EXTI_EMR_EM22           EXTI_EMR_EM22_Msk                              /*!< Event Mask on line 22 */
#define EXTI_EMR_EM23_Pos       (23U)                                          
#define EXTI_EMR_EM23_Msk       (0x1U << EXTI_EMR_EM23_Pos)                    /*!< 0x00800000 */
#define EXTI_EMR_EM23           EXTI_EMR_EM23_Msk                              /*!< Event Mask on line 23 */
#define EXTI_EMR_EM25_Pos       (25U)                                          
#define EXTI_EMR_EM25_Msk       (0x1U << EXTI_EMR_EM25_Pos)                    /*!< 0x02000000 */
#define EXTI_EMR_EM25           EXTI_EMR_EM25_Msk                              /*!< Event Mask on line 25 */
#define EXTI_EMR_EM26_Pos       (26U)                                          
#define EXTI_EMR_EM26_Msk       (0x1U << EXTI_EMR_EM26_Pos)                    /*!< 0x04000000 */
#define EXTI_EMR_EM26           EXTI_EMR_EM26_Msk                              /*!< Event Mask on line 26 */
#define EXTI_EMR_EM28_Pos       (28U)                                          
#define EXTI_EMR_EM28_Msk       (0x1U << EXTI_EMR_EM28_Pos)                    /*!< 0x10000000 */
#define EXTI_EMR_EM28           EXTI_EMR_EM28_Msk                              /*!< Event Mask on line 28 */
#define EXTI_EMR_EM29_Pos       (29U)                                          
#define EXTI_EMR_EM29_Msk       (0x1U << EXTI_EMR_EM29_Pos)                    /*!< 0x20000000 */
#define EXTI_EMR_EM29           EXTI_EMR_EM29_Msk                              /*!< Event Mask on line 29 */

/*******************  Bit definition for EXTI_RTSR register  ******************/
#define EXTI_RTSR_RT0_Pos       (0U)                                           
#define EXTI_RTSR_RT0_Msk       (0x1U << EXTI_RTSR_RT0_Pos)                    /*!< 0x00000001 */
#define EXTI_RTSR_RT0           EXTI_RTSR_RT0_Msk                              /*!< Rising trigger event configuration bit of line 0 */
#define EXTI_RTSR_RT1_Pos       (1U)                                           
#define EXTI_RTSR_RT1_Msk       (0x1U << EXTI_RTSR_RT1_Pos)                    /*!< 0x00000002 */
#define EXTI_RTSR_RT1           EXTI_RTSR_RT1_Msk                              /*!< Rising trigger event configuration bit of line 1 */
#define EXTI_RTSR_RT2_Pos       (2U)                                           
#define EXTI_RTSR_RT2_Msk       (0x1U << EXTI_RTSR_RT2_Pos)                    /*!< 0x00000004 */
#define EXTI_RTSR_RT2           EXTI_RTSR_RT2_Msk                              /*!< Rising trigger event configuration bit of line 2 */
#define EXTI_RTSR_RT3_Pos       (3U)                                           
#define EXTI_RTSR_RT3_Msk       (0x1U << EXTI_RTSR_RT3_Pos)                    /*!< 0x00000008 */
#define EXTI_RTSR_RT3           EXTI_RTSR_RT3_Msk                              /*!< Rising trigger event configuration bit of line 3 */
#define EXTI_RTSR_RT4_Pos       (4U)                                           
#define EXTI_RTSR_RT4_Msk       (0x1U << EXTI_RTSR_RT4_Pos)                    /*!< 0x00000010 */
#define EXTI_RTSR_RT4           EXTI_RTSR_RT4_Msk                              /*!< Rising trigger event configuration bit of line 4 */
#define EXTI_RTSR_RT5_Pos       (5U)                                           
#define EXTI_RTSR_RT5_Msk       (0x1U << EXTI_RTSR_RT5_Pos)                    /*!< 0x00000020 */
#define EXTI_RTSR_RT5           EXTI_RTSR_RT5_Msk                              /*!< Rising trigger event configuration bit of line 5 */
#define EXTI_RTSR_RT6_Pos       (6U)                                           
#define EXTI_RTSR_RT6_Msk       (0x1U << EXTI_RTSR_RT6_Pos)                    /*!< 0x00000040 */
#define EXTI_RTSR_RT6           EXTI_RTSR_RT6_Msk                              /*!< Rising trigger event configuration bit of line 6 */
#define EXTI_RTSR_RT7_Pos       (7U)                                           
#define EXTI_RTSR_RT7_Msk       (0x1U << EXTI_RTSR_RT7_Pos)                    /*!< 0x00000080 */
#define EXTI_RTSR_RT7           EXTI_RTSR_RT7_Msk                              /*!< Rising trigger event configuration bit of line 7 */
#define EXTI_RTSR_RT8_Pos       (8U)                                           
#define EXTI_RTSR_RT8_Msk       (0x1U << EXTI_RTSR_RT8_Pos)                    /*!< 0x00000100 */
#define EXTI_RTSR_RT8           EXTI_RTSR_RT8_Msk                              /*!< Rising trigger event configuration bit of line 8 */
#define EXTI_RTSR_RT9_Pos       (9U)                                           
#define EXTI_RTSR_RT9_Msk       (0x1U << EXTI_RTSR_RT9_Pos)                    /*!< 0x00000200 */
#define EXTI_RTSR_RT9           EXTI_RTSR_RT9_Msk                              /*!< Rising trigger event configuration bit of line 9 */
#define EXTI_RTSR_RT10_Pos      (10U)                                          
#define EXTI_RTSR_RT10_Msk      (0x1U << EXTI_RTSR_RT10_Pos)                   /*!< 0x00000400 */
#define EXTI_RTSR_RT10          EXTI_RTSR_RT10_Msk                             /*!< Rising trigger event configuration bit of line 10 */
#define EXTI_RTSR_RT11_Pos      (11U)                                          
#define EXTI_RTSR_RT11_Msk      (0x1U << EXTI_RTSR_RT11_Pos)                   /*!< 0x00000800 */
#define EXTI_RTSR_RT11          EXTI_RTSR_RT11_Msk                             /*!< Rising trigger event configuration bit of line 11 */
#define EXTI_RTSR_RT12_Pos      (12U)                                          
#define EXTI_RTSR_RT12_Msk      (0x1U << EXTI_RTSR_RT12_Pos)                   /*!< 0x00001000 */
#define EXTI_RTSR_RT12          EXTI_RTSR_RT12_Msk                             /*!< Rising trigger event configuration bit of line 12 */
#define EXTI_RTSR_RT13_Pos      (13U)                                          
#define EXTI_RTSR_RT13_Msk      (0x1U << EXTI_RTSR_RT13_Pos)                   /*!< 0x00002000 */
#define EXTI_RTSR_RT13          EXTI_RTSR_RT13_Msk                             /*!< Rising trigger event configuration bit of line 13 */
#define EXTI_RTSR_RT14_Pos      (14U)                                          
#define EXTI_RTSR_RT14_Msk      (0x1U << EXTI_RTSR_RT14_Pos)                   /*!< 0x00004000 */
#define EXTI_RTSR_RT14          EXTI_RTSR_RT14_Msk                             /*!< Rising trigger event configuration bit of line 14 */
#define EXTI_RTSR_RT15_Pos      (15U)                                          
#define EXTI_RTSR_RT15_Msk      (0x1U << EXTI_RTSR_RT15_Pos)                   /*!< 0x00008000 */
#define EXTI_RTSR_RT15          EXTI_RTSR_RT15_Msk                             /*!< Rising trigger event configuration bit of line 15 */
#define EXTI_RTSR_RT16_Pos      (16U)                                          
#define EXTI_RTSR_RT16_Msk      (0x1U << EXTI_RTSR_RT16_Pos)                   /*!< 0x00010000 */
#define EXTI_RTSR_RT16          EXTI_RTSR_RT16_Msk                             /*!< Rising trigger event configuration bit of line 16 */
#define EXTI_RTSR_RT17_Pos      (17U)                                          
#define EXTI_RTSR_RT17_Msk      (0x1U << EXTI_RTSR_RT17_Pos)                   /*!< 0x00020000 */
#define EXTI_RTSR_RT17          EXTI_RTSR_RT17_Msk                             /*!< Rising trigger event configuration bit of line 17 */
#define EXTI_RTSR_RT19_Pos      (19U)                                          
#define EXTI_RTSR_RT19_Msk      (0x1U << EXTI_RTSR_RT19_Pos)                   /*!< 0x00080000 */
#define EXTI_RTSR_RT19          EXTI_RTSR_RT19_Msk                             /*!< Rising trigger event configuration bit of line 19 */
#define EXTI_RTSR_RT20_Pos      (20U)                                          
#define EXTI_RTSR_RT20_Msk      (0x1U << EXTI_RTSR_RT20_Pos)                   /*!< 0x00100000 */
#define EXTI_RTSR_RT20          EXTI_RTSR_RT20_Msk                             /*!< Rising trigger event configuration bit of line 20 */
#define EXTI_RTSR_RT21_Pos      (21U)                                          
#define EXTI_RTSR_RT21_Msk      (0x1U << EXTI_RTSR_RT21_Pos)                   /*!< 0x00200000 */
#define EXTI_RTSR_RT21          EXTI_RTSR_RT21_Msk                             /*!< Rising trigger event configuration bit of line 21 */
#define EXTI_RTSR_RT22_Pos      (22U)                                          
#define EXTI_RTSR_RT22_Msk      (0x1U << EXTI_RTSR_RT22_Pos)                   /*!< 0x00400000 */
#define EXTI_RTSR_RT22          EXTI_RTSR_RT22_Msk                             /*!< Rising trigger event configuration bit of line 22 */

/* Legacy defines */
#define EXTI_RTSR_TR0                       EXTI_RTSR_RT0
#define EXTI_RTSR_TR1                       EXTI_RTSR_RT1
#define EXTI_RTSR_TR2                       EXTI_RTSR_RT2
#define EXTI_RTSR_TR3                       EXTI_RTSR_RT3
#define EXTI_RTSR_TR4                       EXTI_RTSR_RT4
#define EXTI_RTSR_TR5                       EXTI_RTSR_RT5
#define EXTI_RTSR_TR6                       EXTI_RTSR_RT6
#define EXTI_RTSR_TR7                       EXTI_RTSR_RT7
#define EXTI_RTSR_TR8                       EXTI_RTSR_RT8
#define EXTI_RTSR_TR9                       EXTI_RTSR_RT9
#define EXTI_RTSR_TR10                      EXTI_RTSR_RT10
#define EXTI_RTSR_TR11                      EXTI_RTSR_RT11
#define EXTI_RTSR_TR12                      EXTI_RTSR_RT12
#define EXTI_RTSR_TR13                      EXTI_RTSR_RT13
#define EXTI_RTSR_TR14                      EXTI_RTSR_RT14
#define EXTI_RTSR_TR15                      EXTI_RTSR_RT15
#define EXTI_RTSR_TR16                      EXTI_RTSR_RT16
#define EXTI_RTSR_TR17                      EXTI_RTSR_RT17
#define EXTI_RTSR_TR19                      EXTI_RTSR_RT19
#define EXTI_RTSR_TR20                      EXTI_RTSR_RT20
#define EXTI_RTSR_TR21                      EXTI_RTSR_RT21
#define EXTI_RTSR_TR22                      EXTI_RTSR_RT22

/*******************  Bit definition for EXTI_FTSR register *******************/
#define EXTI_FTSR_FT0_Pos       (0U)                                           
#define EXTI_FTSR_FT0_Msk       (0x1U << EXTI_FTSR_FT0_Pos)                    /*!< 0x00000001 */
#define EXTI_FTSR_FT0           EXTI_FTSR_FT0_Msk                              /*!< Falling trigger event configuration bit of line 0 */
#define EXTI_FTSR_FT1_Pos       (1U)                                           
#define EXTI_FTSR_FT1_Msk       (0x1U << EXTI_FTSR_FT1_Pos)                    /*!< 0x00000002 */
#define EXTI_FTSR_FT1           EXTI_FTSR_FT1_Msk                              /*!< Falling trigger event configuration bit of line 1 */
#define EXTI_FTSR_FT2_Pos       (2U)                                           
#define EXTI_FTSR_FT2_Msk       (0x1U << EXTI_FTSR_FT2_Pos)                    /*!< 0x00000004 */
#define EXTI_FTSR_FT2           EXTI_FTSR_FT2_Msk                              /*!< Falling trigger event configuration bit of line 2 */
#define EXTI_FTSR_FT3_Pos       (3U)                                           
#define EXTI_FTSR_FT3_Msk       (0x1U << EXTI_FTSR_FT3_Pos)                    /*!< 0x00000008 */
#define EXTI_FTSR_FT3           EXTI_FTSR_FT3_Msk                              /*!< Falling trigger event configuration bit of line 3 */
#define EXTI_FTSR_FT4_Pos       (4U)                                           
#define EXTI_FTSR_FT4_Msk       (0x1U << EXTI_FTSR_FT4_Pos)                    /*!< 0x00000010 */
#define EXTI_FTSR_FT4           EXTI_FTSR_FT4_Msk                              /*!< Falling trigger event configuration bit of line 4 */
#define EXTI_FTSR_FT5_Pos       (5U)                                           
#define EXTI_FTSR_FT5_Msk       (0x1U << EXTI_FTSR_FT5_Pos)                    /*!< 0x00000020 */
#define EXTI_FTSR_FT5           EXTI_FTSR_FT5_Msk                              /*!< Falling trigger event configuration bit of line 5 */
#define EXTI_FTSR_FT6_Pos       (6U)                                           
#define EXTI_FTSR_FT6_Msk       (0x1U << EXTI_FTSR_FT6_Pos)                    /*!< 0x00000040 */
#define EXTI_FTSR_FT6           EXTI_FTSR_FT6_Msk                              /*!< Falling trigger event configuration bit of line 6 */
#define EXTI_FTSR_FT7_Pos       (7U)                                           
#define EXTI_FTSR_FT7_Msk       (0x1U << EXTI_FTSR_FT7_Pos)                    /*!< 0x00000080 */
#define EXTI_FTSR_FT7           EXTI_FTSR_FT7_Msk                              /*!< Falling trigger event configuration bit of line 7 */
#define EXTI_FTSR_FT8_Pos       (8U)                                           
#define EXTI_FTSR_FT8_Msk       (0x1U << EXTI_FTSR_FT8_Pos)                    /*!< 0x00000100 */
#define EXTI_FTSR_FT8           EXTI_FTSR_FT8_Msk                              /*!< Falling trigger event configuration bit of line 8 */
#define EXTI_FTSR_FT9_Pos       (9U)                                           
#define EXTI_FTSR_FT9_Msk       (0x1U << EXTI_FTSR_FT9_Pos)                    /*!< 0x00000200 */
#define EXTI_FTSR_FT9           EXTI_FTSR_FT9_Msk                              /*!< Falling trigger event configuration bit of line 9 */
#define EXTI_FTSR_FT10_Pos      (10U)                                          
#define EXTI_FTSR_FT10_Msk      (0x1U << EXTI_FTSR_FT10_Pos)                   /*!< 0x00000400 */
#define EXTI_FTSR_FT10          EXTI_FTSR_FT10_Msk                             /*!< Falling trigger event configuration bit of line 10 */
#define EXTI_FTSR_FT11_Pos      (11U)                                          
#define EXTI_FTSR_FT11_Msk      (0x1U << EXTI_FTSR_FT11_Pos)                   /*!< 0x00000800 */
#define EXTI_FTSR_FT11          EXTI_FTSR_FT11_Msk                             /*!< Falling trigger event configuration bit of line 11 */
#define EXTI_FTSR_FT12_Pos      (12U)                                          
#define EXTI_FTSR_FT12_Msk      (0x1U << EXTI_FTSR_FT12_Pos)                   /*!< 0x00001000 */
#define EXTI_FTSR_FT12          EXTI_FTSR_FT12_Msk                             /*!< Falling trigger event configuration bit of line 12 */
#define EXTI_FTSR_FT13_Pos      (13U)                                          
#define EXTI_FTSR_FT13_Msk      (0x1U << EXTI_FTSR_FT13_Pos)                   /*!< 0x00002000 */
#define EXTI_FTSR_FT13          EXTI_FTSR_FT13_Msk                             /*!< Falling trigger event configuration bit of line 13 */
#define EXTI_FTSR_FT14_Pos      (14U)                                          
#define EXTI_FTSR_FT14_Msk      (0x1U << EXTI_FTSR_FT14_Pos)                   /*!< 0x00004000 */
#define EXTI_FTSR_FT14          EXTI_FTSR_FT14_Msk                             /*!< Falling trigger event configuration bit of line 14 */
#define EXTI_FTSR_FT15_Pos      (15U)                                          
#define EXTI_FTSR_FT15_Msk      (0x1U << EXTI_FTSR_FT15_Pos)                   /*!< 0x00008000 */
#define EXTI_FTSR_FT15          EXTI_FTSR_FT15_Msk                             /*!< Falling trigger event configuration bit of line 15 */
#define EXTI_FTSR_FT16_Pos      (16U)                                          
#define EXTI_FTSR_FT16_Msk      (0x1U << EXTI_FTSR_FT16_Pos)                   /*!< 0x00010000 */
#define EXTI_FTSR_FT16          EXTI_FTSR_FT16_Msk                             /*!< Falling trigger event configuration bit of line 16 */
#define EXTI_FTSR_FT17_Pos      (17U)                                          
#define EXTI_FTSR_FT17_Msk      (0x1U << EXTI_FTSR_FT17_Pos)                   /*!< 0x00020000 */
#define EXTI_FTSR_FT17          EXTI_FTSR_FT17_Msk                             /*!< Falling trigger event configuration bit of line 17 */
#define EXTI_FTSR_FT19_Pos      (19U)                                          
#define EXTI_FTSR_FT19_Msk      (0x1U << EXTI_FTSR_FT19_Pos)                   /*!< 0x00080000 */
#define EXTI_FTSR_FT19          EXTI_FTSR_FT19_Msk                             /*!< Falling trigger event configuration bit of line 19 */
#define EXTI_FTSR_FT20_Pos      (20U)                                          
#define EXTI_FTSR_FT20_Msk      (0x1U << EXTI_FTSR_FT20_Pos)                   /*!< 0x00100000 */
#define EXTI_FTSR_FT20          EXTI_FTSR_FT20_Msk                             /*!< Falling trigger event configuration bit of line 20 */
#define EXTI_FTSR_FT21_Pos      (21U)                                          
#define EXTI_FTSR_FT21_Msk      (0x1U << EXTI_FTSR_FT21_Pos)                   /*!< 0x00200000 */
#define EXTI_FTSR_FT21          EXTI_FTSR_FT21_Msk                             /*!< Falling trigger event configuration bit of line 21 */
#define EXTI_FTSR_FT22_Pos      (22U)                                          
#define EXTI_FTSR_FT22_Msk      (0x1U << EXTI_FTSR_FT22_Pos)                   /*!< 0x00400000 */
#define EXTI_FTSR_FT22          EXTI_FTSR_FT22_Msk                             /*!< Falling trigger event configuration bit of line 22 */

/* Legacy defines */
#define EXTI_FTSR_TR0                       EXTI_FTSR_FT0
#define EXTI_FTSR_TR1                       EXTI_FTSR_FT1
#define EXTI_FTSR_TR2                       EXTI_FTSR_FT2
#define EXTI_FTSR_TR3                       EXTI_FTSR_FT3
#define EXTI_FTSR_TR4                       EXTI_FTSR_FT4
#define EXTI_FTSR_TR5                       EXTI_FTSR_FT5
#define EXTI_FTSR_TR6                       EXTI_FTSR_FT6
#define EXTI_FTSR_TR7                       EXTI_FTSR_FT7
#define EXTI_FTSR_TR8                       EXTI_FTSR_FT8
#define EXTI_FTSR_TR9                       EXTI_FTSR_FT9
#define EXTI_FTSR_TR10                      EXTI_FTSR_FT10
#define EXTI_FTSR_TR11                      EXTI_FTSR_FT11
#define EXTI_FTSR_TR12                      EXTI_FTSR_FT12
#define EXTI_FTSR_TR13                      EXTI_FTSR_FT13
#define EXTI_FTSR_TR14                      EXTI_FTSR_FT14
#define EXTI_FTSR_TR15                      EXTI_FTSR_FT15
#define EXTI_FTSR_TR16                      EXTI_FTSR_FT16
#define EXTI_FTSR_TR17                      EXTI_FTSR_FT17
#define EXTI_FTSR_TR19                      EXTI_FTSR_FT19
#define EXTI_FTSR_TR20                      EXTI_FTSR_FT20
#define EXTI_FTSR_TR21                      EXTI_FTSR_FT21
#define EXTI_FTSR_TR22                      EXTI_FTSR_FT22

/******************* Bit definition for EXTI_SWIER register *******************/
#define EXTI_SWIER_SWI0_Pos     (0U)                                           
#define EXTI_SWIER_SWI0_Msk     (0x1U << EXTI_SWIER_SWI0_Pos)                  /*!< 0x00000001 */
#define EXTI_SWIER_SWI0         EXTI_SWIER_SWI0_Msk                            /*!< Software Interrupt on line 0  */
#define EXTI_SWIER_SWI1_Pos     (1U)                                           
#define EXTI_SWIER_SWI1_Msk     (0x1U << EXTI_SWIER_SWI1_Pos)                  /*!< 0x00000002 */
#define EXTI_SWIER_SWI1         EXTI_SWIER_SWI1_Msk                            /*!< Software Interrupt on line 1  */
#define EXTI_SWIER_SWI2_Pos     (2U)                                           
#define EXTI_SWIER_SWI2_Msk     (0x1U << EXTI_SWIER_SWI2_Pos)                  /*!< 0x00000004 */
#define EXTI_SWIER_SWI2         EXTI_SWIER_SWI2_Msk                            /*!< Software Interrupt on line 2  */
#define EXTI_SWIER_SWI3_Pos     (3U)                                           
#define EXTI_SWIER_SWI3_Msk     (0x1U << EXTI_SWIER_SWI3_Pos)                  /*!< 0x00000008 */
#define EXTI_SWIER_SWI3         EXTI_SWIER_SWI3_Msk                            /*!< Software Interrupt on line 3  */
#define EXTI_SWIER_SWI4_Pos     (4U)                                           
#define EXTI_SWIER_SWI4_Msk     (0x1U << EXTI_SWIER_SWI4_Pos)                  /*!< 0x00000010 */
#define EXTI_SWIER_SWI4         EXTI_SWIER_SWI4_Msk                            /*!< Software Interrupt on line 4  */
#define EXTI_SWIER_SWI5_Pos     (5U)                                           
#define EXTI_SWIER_SWI5_Msk     (0x1U << EXTI_SWIER_SWI5_Pos)                  /*!< 0x00000020 */
#define EXTI_SWIER_SWI5         EXTI_SWIER_SWI5_Msk                            /*!< Software Interrupt on line 5  */
#define EXTI_SWIER_SWI6_Pos     (6U)                                           
#define EXTI_SWIER_SWI6_Msk     (0x1U << EXTI_SWIER_SWI6_Pos)                  /*!< 0x00000040 */
#define EXTI_SWIER_SWI6         EXTI_SWIER_SWI6_Msk                            /*!< Software Interrupt on line 6  */
#define EXTI_SWIER_SWI7_Pos     (7U)                                           
#define EXTI_SWIER_SWI7_Msk     (0x1U << EXTI_SWIER_SWI7_Pos)                  /*!< 0x00000080 */
#define EXTI_SWIER_SWI7         EXTI_SWIER_SWI7_Msk                            /*!< Software Interrupt on line 7  */
#define EXTI_SWIER_SWI8_Pos     (8U)                                           
#define EXTI_SWIER_SWI8_Msk     (0x1U << EXTI_SWIER_SWI8_Pos)                  /*!< 0x00000100 */
#define EXTI_SWIER_SWI8         EXTI_SWIER_SWI8_Msk                            /*!< Software Interrupt on line 8  */
#define EXTI_SWIER_SWI9_Pos     (9U)                                           
#define EXTI_SWIER_SWI9_Msk     (0x1U << EXTI_SWIER_SWI9_Pos)                  /*!< 0x00000200 */
#define EXTI_SWIER_SWI9         EXTI_SWIER_SWI9_Msk                            /*!< Software Interrupt on line 9  */
#define EXTI_SWIER_SWI10_Pos    (10U)                                          
#define EXTI_SWIER_SWI10_Msk    (0x1U << EXTI_SWIER_SWI10_Pos)                 /*!< 0x00000400 */
#define EXTI_SWIER_SWI10        EXTI_SWIER_SWI10_Msk                           /*!< Software Interrupt on line 10 */
#define EXTI_SWIER_SWI11_Pos    (11U)                                          
#define EXTI_SWIER_SWI11_Msk    (0x1U << EXTI_SWIER_SWI11_Pos)                 /*!< 0x00000800 */
#define EXTI_SWIER_SWI11        EXTI_SWIER_SWI11_Msk                           /*!< Software Interrupt on line 11 */
#define EXTI_SWIER_SWI12_Pos    (12U)                                          
#define EXTI_SWIER_SWI12_Msk    (0x1U << EXTI_SWIER_SWI12_Pos)                 /*!< 0x00001000 */
#define EXTI_SWIER_SWI12        EXTI_SWIER_SWI12_Msk                           /*!< Software Interrupt on line 12 */
#define EXTI_SWIER_SWI13_Pos    (13U)                                          
#define EXTI_SWIER_SWI13_Msk    (0x1U << EXTI_SWIER_SWI13_Pos)                 /*!< 0x00002000 */
#define EXTI_SWIER_SWI13        EXTI_SWIER_SWI13_Msk                           /*!< Software Interrupt on line 13 */
#define EXTI_SWIER_SWI14_Pos    (14U)                                          
#define EXTI_SWIER_SWI14_Msk    (0x1U << EXTI_SWIER_SWI14_Pos)                 /*!< 0x00004000 */
#define EXTI_SWIER_SWI14        EXTI_SWIER_SWI14_Msk                           /*!< Software Interrupt on line 14 */
#define EXTI_SWIER_SWI15_Pos    (15U)                                          
#define EXTI_SWIER_SWI15_Msk    (0x1U << EXTI_SWIER_SWI15_Pos)                 /*!< 0x00008000 */
#define EXTI_SWIER_SWI15        EXTI_SWIER_SWI15_Msk                           /*!< Software Interrupt on line 15 */
#define EXTI_SWIER_SWI16_Pos    (16U)                                          
#define EXTI_SWIER_SWI16_Msk    (0x1U << EXTI_SWIER_SWI16_Pos)                 /*!< 0x00010000 */
#define EXTI_SWIER_SWI16        EXTI_SWIER_SWI16_Msk                           /*!< Software Interrupt on line 16 */
#define EXTI_SWIER_SWI17_Pos    (17U)                                          
#define EXTI_SWIER_SWI17_Msk    (0x1U << EXTI_SWIER_SWI17_Pos)                 /*!< 0x00020000 */
#define EXTI_SWIER_SWI17        EXTI_SWIER_SWI17_Msk                           /*!< Software Interrupt on line 17 */
#define EXTI_SWIER_SWI19_Pos    (19U)                                          
#define EXTI_SWIER_SWI19_Msk    (0x1U << EXTI_SWIER_SWI19_Pos)                 /*!< 0x00080000 */
#define EXTI_SWIER_SWI19        EXTI_SWIER_SWI19_Msk                           /*!< Software Interrupt on line 19 */
#define EXTI_SWIER_SWI20_Pos    (20U)                                          
#define EXTI_SWIER_SWI20_Msk    (0x1U << EXTI_SWIER_SWI20_Pos)                 /*!< 0x00100000 */
#define EXTI_SWIER_SWI20        EXTI_SWIER_SWI20_Msk                           /*!< Software Interrupt on line 20 */
#define EXTI_SWIER_SWI21_Pos    (21U)                                          
#define EXTI_SWIER_SWI21_Msk    (0x1U << EXTI_SWIER_SWI21_Pos)                 /*!< 0x00200000 */
#define EXTI_SWIER_SWI21        EXTI_SWIER_SWI21_Msk                           /*!< Software Interrupt on line 21 */
#define EXTI_SWIER_SWI22_Pos    (22U)                                          
#define EXTI_SWIER_SWI22_Msk    (0x1U << EXTI_SWIER_SWI22_Pos)                 /*!< 0x00400000 */
#define EXTI_SWIER_SWI22        EXTI_SWIER_SWI22_Msk                           /*!< Software Interrupt on line 22 */

/* Legacy defines */
#define EXTI_SWIER_SWIER0                   EXTI_SWIER_SWI0
#define EXTI_SWIER_SWIER1                   EXTI_SWIER_SWI1
#define EXTI_SWIER_SWIER2                   EXTI_SWIER_SWI2
#define EXTI_SWIER_SWIER3                   EXTI_SWIER_SWI3
#define EXTI_SWIER_SWIER4                   EXTI_SWIER_SWI4
#define EXTI_SWIER_SWIER5                   EXTI_SWIER_SWI5
#define EXTI_SWIER_SWIER6                   EXTI_SWIER_SWI6
#define EXTI_SWIER_SWIER7                   EXTI_SWIER_SWI7
#define EXTI_SWIER_SWIER8                   EXTI_SWIER_SWI8
#define EXTI_SWIER_SWIER9                   EXTI_SWIER_SWI9
#define EXTI_SWIER_SWIER10                  EXTI_SWIER_SWI10
#define EXTI_SWIER_SWIER11                  EXTI_SWIER_SWI11
#define EXTI_SWIER_SWIER12                  EXTI_SWIER_SWI12
#define EXTI_SWIER_SWIER13                  EXTI_SWIER_SWI13
#define EXTI_SWIER_SWIER14                  EXTI_SWIER_SWI14
#define EXTI_SWIER_SWIER15                  EXTI_SWIER_SWI15
#define EXTI_SWIER_SWIER16                  EXTI_SWIER_SWI16
#define EXTI_SWIER_SWIER17                  EXTI_SWIER_SWI17
#define EXTI_SWIER_SWIER19                  EXTI_SWIER_SWI19
#define EXTI_SWIER_SWIER20                  EXTI_SWIER_SWI20
#define EXTI_SWIER_SWIER21                  EXTI_SWIER_SWI21
#define EXTI_SWIER_SWIER22                  EXTI_SWIER_SWI22

/******************  Bit definition for EXTI_PR register  *********************/
#define EXTI_PR_PIF0_Pos        (0U)                                           
#define EXTI_PR_PIF0_Msk        (0x1U << EXTI_PR_PIF0_Pos)                     /*!< 0x00000001 */
#define EXTI_PR_PIF0            EXTI_PR_PIF0_Msk                               /*!< Pending bit 0  */
#define EXTI_PR_PIF1_Pos        (1U)                                           
#define EXTI_PR_PIF1_Msk        (0x1U << EXTI_PR_PIF1_Pos)                     /*!< 0x00000002 */
#define EXTI_PR_PIF1            EXTI_PR_PIF1_Msk                               /*!< Pending bit 1  */
#define EXTI_PR_PIF2_Pos        (2U)                                           
#define EXTI_PR_PIF2_Msk        (0x1U << EXTI_PR_PIF2_Pos)                     /*!< 0x00000004 */
#define EXTI_PR_PIF2            EXTI_PR_PIF2_Msk                               /*!< Pending bit 2  */
#define EXTI_PR_PIF3_Pos        (3U)                                           
#define EXTI_PR_PIF3_Msk        (0x1U << EXTI_PR_PIF3_Pos)                     /*!< 0x00000008 */
#define EXTI_PR_PIF3            EXTI_PR_PIF3_Msk                               /*!< Pending bit 3  */
#define EXTI_PR_PIF4_Pos        (4U)                                           
#define EXTI_PR_PIF4_Msk        (0x1U << EXTI_PR_PIF4_Pos)                     /*!< 0x00000010 */
#define EXTI_PR_PIF4            EXTI_PR_PIF4_Msk                               /*!< Pending bit 4  */
#define EXTI_PR_PIF5_Pos        (5U)                                           
#define EXTI_PR_PIF5_Msk        (0x1U << EXTI_PR_PIF5_Pos)                     /*!< 0x00000020 */
#define EXTI_PR_PIF5            EXTI_PR_PIF5_Msk                               /*!< Pending bit 5  */
#define EXTI_PR_PIF6_Pos        (6U)                                           
#define EXTI_PR_PIF6_Msk        (0x1U << EXTI_PR_PIF6_Pos)                     /*!< 0x00000040 */
#define EXTI_PR_PIF6            EXTI_PR_PIF6_Msk                               /*!< Pending bit 6  */
#define EXTI_PR_PIF7_Pos        (7U)                                           
#define EXTI_PR_PIF7_Msk        (0x1U << EXTI_PR_PIF7_Pos)                     /*!< 0x00000080 */
#define EXTI_PR_PIF7            EXTI_PR_PIF7_Msk                               /*!< Pending bit 7  */
#define EXTI_PR_PIF8_Pos        (8U)                                           
#define EXTI_PR_PIF8_Msk        (0x1U << EXTI_PR_PIF8_Pos)                     /*!< 0x00000100 */
#define EXTI_PR_PIF8            EXTI_PR_PIF8_Msk                               /*!< Pending bit 8  */
#define EXTI_PR_PIF9_Pos        (9U)                                           
#define EXTI_PR_PIF9_Msk        (0x1U << EXTI_PR_PIF9_Pos)                     /*!< 0x00000200 */
#define EXTI_PR_PIF9            EXTI_PR_PIF9_Msk                               /*!< Pending bit 9  */
#define EXTI_PR_PIF10_Pos       (10U)                                          
#define EXTI_PR_PIF10_Msk       (0x1U << EXTI_PR_PIF10_Pos)                    /*!< 0x00000400 */
#define EXTI_PR_PIF10           EXTI_PR_PIF10_Msk                              /*!< Pending bit 10 */
#define EXTI_PR_PIF11_Pos       (11U)                                          
#define EXTI_PR_PIF11_Msk       (0x1U << EXTI_PR_PIF11_Pos)                    /*!< 0x00000800 */
#define EXTI_PR_PIF11           EXTI_PR_PIF11_Msk                              /*!< Pending bit 11 */
#define EXTI_PR_PIF12_Pos       (12U)                                          
#define EXTI_PR_PIF12_Msk       (0x1U << EXTI_PR_PIF12_Pos)                    /*!< 0x00001000 */
#define EXTI_PR_PIF12           EXTI_PR_PIF12_Msk                              /*!< Pending bit 12 */
#define EXTI_PR_PIF13_Pos       (13U)                                          
#define EXTI_PR_PIF13_Msk       (0x1U << EXTI_PR_PIF13_Pos)                    /*!< 0x00002000 */
#define EXTI_PR_PIF13           EXTI_PR_PIF13_Msk                              /*!< Pending bit 13 */
#define EXTI_PR_PIF14_Pos       (14U)                                          
#define EXTI_PR_PIF14_Msk       (0x1U << EXTI_PR_PIF14_Pos)                    /*!< 0x00004000 */
#define EXTI_PR_PIF14           EXTI_PR_PIF14_Msk                              /*!< Pending bit 14 */
#define EXTI_PR_PIF15_Pos       (15U)                                          
#define EXTI_PR_PIF15_Msk       (0x1U << EXTI_PR_PIF15_Pos)                    /*!< 0x00008000 */
#define EXTI_PR_PIF15           EXTI_PR_PIF15_Msk                              /*!< Pending bit 15 */
#define EXTI_PR_PIF16_Pos       (16U)                                          
#define EXTI_PR_PIF16_Msk       (0x1U << EXTI_PR_PIF16_Pos)                    /*!< 0x00010000 */
#define EXTI_PR_PIF16           EXTI_PR_PIF16_Msk                              /*!< Pending bit 16 */
#define EXTI_PR_PIF17_Pos       (17U)                                          
#define EXTI_PR_PIF17_Msk       (0x1U << EXTI_PR_PIF17_Pos)                    /*!< 0x00020000 */
#define EXTI_PR_PIF17           EXTI_PR_PIF17_Msk                              /*!< Pending bit 17 */
#define EXTI_PR_PIF19_Pos       (19U)                                          
#define EXTI_PR_PIF19_Msk       (0x1U << EXTI_PR_PIF19_Pos)                    /*!< 0x00080000 */
#define EXTI_PR_PIF19           EXTI_PR_PIF19_Msk                              /*!< Pending bit 19 */
#define EXTI_PR_PIF20_Pos       (20U)                                          
#define EXTI_PR_PIF20_Msk       (0x1U << EXTI_PR_PIF20_Pos)                    /*!< 0x00100000 */
#define EXTI_PR_PIF20           EXTI_PR_PIF20_Msk                              /*!< Pending bit 20 */
#define EXTI_PR_PIF21_Pos       (21U)                                          
#define EXTI_PR_PIF21_Msk       (0x1U << EXTI_PR_PIF21_Pos)                    /*!< 0x00200000 */
#define EXTI_PR_PIF21           EXTI_PR_PIF21_Msk                              /*!< Pending bit 21 */
#define EXTI_PR_PIF22_Pos       (22U)                                          
#define EXTI_PR_PIF22_Msk       (0x1U << EXTI_PR_PIF22_Pos)                    /*!< 0x00400000 */
#define EXTI_PR_PIF22           EXTI_PR_PIF22_Msk                              /*!< Pending bit 22 */

/* Legacy defines */
#define EXTI_PR_PR0                         EXTI_PR_PIF0
#define EXTI_PR_PR1                         EXTI_PR_PIF1
#define EXTI_PR_PR2                         EXTI_PR_PIF2
#define EXTI_PR_PR3                         EXTI_PR_PIF3
#define EXTI_PR_PR4                         EXTI_PR_PIF4
#define EXTI_PR_PR5                         EXTI_PR_PIF5
#define EXTI_PR_PR6                         EXTI_PR_PIF6
#define EXTI_PR_PR7                         EXTI_PR_PIF7
#define EXTI_PR_PR8                         EXTI_PR_PIF8
#define EXTI_PR_PR9                         EXTI_PR_PIF9
#define EXTI_PR_PR10                        EXTI_PR_PIF10
#define EXTI_PR_PR11                        EXTI_PR_PIF11
#define EXTI_PR_PR12                        EXTI_PR_PIF12
#define EXTI_PR_PR13                        EXTI_PR_PIF13
#define EXTI_PR_PR14                        EXTI_PR_PIF14
#define EXTI_PR_PR15                        EXTI_PR_PIF15
#define EXTI_PR_PR16                        EXTI_PR_PIF16
#define EXTI_PR_PR17                        EXTI_PR_PIF17
#define EXTI_PR_PR19                        EXTI_PR_PIF19
#define EXTI_PR_PR20                        EXTI_PR_PIF20
#define EXTI_PR_PR21                        EXTI_PR_PIF21
#define EXTI_PR_PR22                        EXTI_PR_PIF22

/******************************************************************************/
/*                                                                            */
/*                      FLASH and Option Bytes Registers                      */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for FLASH_ACR register  ******************/
#define FLASH_ACR_LATENCY_Pos        (0U)                                      
#define FLASH_ACR_LATENCY_Msk        (0x1U << FLASH_ACR_LATENCY_Pos)           /*!< 0x00000001 */
#define FLASH_ACR_LATENCY            FLASH_ACR_LATENCY_Msk                     /*!< LATENCY bit (Latency) */
#define FLASH_ACR_PRFTEN_Pos         (1U)                                      
#define FLASH_ACR_PRFTEN_Msk         (0x1U << FLASH_ACR_PRFTEN_Pos)            /*!< 0x00000002 */
#define FLASH_ACR_PRFTEN             FLASH_ACR_PRFTEN_Msk                      /*!< Prefetch Buffer Enable */
#define FLASH_ACR_SLEEP_PD_Pos       (3U)                                      
#define FLASH_ACR_SLEEP_PD_Msk       (0x1U << FLASH_ACR_SLEEP_PD_Pos)          /*!< 0x00000008 */
#define FLASH_ACR_SLEEP_PD           FLASH_ACR_SLEEP_PD_Msk                    /*!< Flash mode during sleep mode */
#define FLASH_ACR_RUN_PD_Pos         (4U)                                      
#define FLASH_ACR_RUN_PD_Msk         (0x1U << FLASH_ACR_RUN_PD_Pos)            /*!< 0x00000010 */
#define FLASH_ACR_RUN_PD             FLASH_ACR_RUN_PD_Msk                      /*!< Flash mode during RUN mode */
#define FLASH_ACR_DISAB_BUF_Pos      (5U)                                      
#define FLASH_ACR_DISAB_BUF_Msk      (0x1U << FLASH_ACR_DISAB_BUF_Pos)         /*!< 0x00000020 */
#define FLASH_ACR_DISAB_BUF          FLASH_ACR_DISAB_BUF_Msk                   /*!< Disable Buffer */
#define FLASH_ACR_PRE_READ_Pos       (6U)                                      
#define FLASH_ACR_PRE_READ_Msk       (0x1U << FLASH_ACR_PRE_READ_Pos)          /*!< 0x00000040 */
#define FLASH_ACR_PRE_READ           FLASH_ACR_PRE_READ_Msk                    /*!< Pre-read data address */

/*******************  Bit definition for FLASH_PECR register  ******************/
#define FLASH_PECR_PELOCK_Pos        (0U)                                      
#define FLASH_PECR_PELOCK_Msk        (0x1U << FLASH_PECR_PELOCK_Pos)           /*!< 0x00000001 */
#define FLASH_PECR_PELOCK            FLASH_PECR_PELOCK_Msk                     /*!< FLASH_PECR and Flash data Lock */
#define FLASH_PECR_PRGLOCK_Pos       (1U)                                      
#define FLASH_PECR_PRGLOCK_Msk       (0x1U << FLASH_PECR_PRGLOCK_Pos)          /*!< 0x00000002 */
#define FLASH_PECR_PRGLOCK           FLASH_PECR_PRGLOCK_Msk                    /*!< Program matrix Lock */
#define FLASH_PECR_OPTLOCK_Pos       (2U)                                      
#define FLASH_PECR_OPTLOCK_Msk       (0x1U << FLASH_PECR_OPTLOCK_Pos)          /*!< 0x00000004 */
#define FLASH_PECR_OPTLOCK           FLASH_PECR_OPTLOCK_Msk                    /*!< Option byte matrix Lock */
#define FLASH_PECR_PROG_Pos          (3U)                                      
#define FLASH_PECR_PROG_Msk          (0x1U << FLASH_PECR_PROG_Pos)             /*!< 0x00000008 */
#define FLASH_PECR_PROG              FLASH_PECR_PROG_Msk                       /*!< Program matrix selection */
#define FLASH_PECR_DATA_Pos          (4U)                                      
#define FLASH_PECR_DATA_Msk          (0x1U << FLASH_PECR_DATA_Pos)             /*!< 0x00000010 */
#define FLASH_PECR_DATA              FLASH_PECR_DATA_Msk                       /*!< Data matrix selection */
#define FLASH_PECR_FIX_Pos           (8U)                                      
#define FLASH_PECR_FIX_Msk           (0x1U << FLASH_PECR_FIX_Pos)              /*!< 0x00000100 */
#define FLASH_PECR_FIX               FLASH_PECR_FIX_Msk                        /*!< Fixed Time Data write for Word/Half Word/Byte programming */
#define FLASH_PECR_ERASE_Pos         (9U)                                      
#define FLASH_PECR_ERASE_Msk         (0x1U << FLASH_PECR_ERASE_Pos)            /*!< 0x00000200 */
#define FLASH_PECR_ERASE             FLASH_PECR_ERASE_Msk                      /*!< Page erasing mode */
#define FLASH_PECR_FPRG_Pos          (10U)                                     
#define FLASH_PECR_FPRG_Msk          (0x1U << FLASH_PECR_FPRG_Pos)             /*!< 0x00000400 */
#define FLASH_PECR_FPRG              FLASH_PECR_FPRG_Msk                       /*!< Fast Page/Half Page programming mode */
#define FLASH_PECR_EOPIE_Pos         (16U)                                     
#define FLASH_PECR_EOPIE_Msk         (0x1U << FLASH_PECR_EOPIE_Pos)            /*!< 0x00010000 */
#define FLASH_PECR_EOPIE             FLASH_PECR_EOPIE_Msk                      /*!< End of programming interrupt */ 
#define FLASH_PECR_ERRIE_Pos         (17U)                                     
#define FLASH_PECR_ERRIE_Msk         (0x1U << FLASH_PECR_ERRIE_Pos)            /*!< 0x00020000 */
#define FLASH_PECR_ERRIE             FLASH_PECR_ERRIE_Msk                      /*!< Error interrupt */ 
#define FLASH_PECR_OBL_LAUNCH_Pos    (18U)                                     
#define FLASH_PECR_OBL_LAUNCH_Msk    (0x1U << FLASH_PECR_OBL_LAUNCH_Pos)       /*!< 0x00040000 */
#define FLASH_PECR_OBL_LAUNCH        FLASH_PECR_OBL_LAUNCH_Msk                 /*!< Launch the option byte loading */
#define FLASH_PECR_HALF_ARRAY_Pos    (19U)                                     
#define FLASH_PECR_HALF_ARRAY_Msk    (0x1U << FLASH_PECR_HALF_ARRAY_Pos)       /*!< 0x00080000 */
#define FLASH_PECR_HALF_ARRAY        FLASH_PECR_HALF_ARRAY_Msk                 /*!< Half array mode */

/******************  Bit definition for FLASH_PDKEYR register  ******************/
#define FLASH_PDKEYR_PDKEYR_Pos      (0U)                                      
#define FLASH_PDKEYR_PDKEYR_Msk      (0xFFFFFFFFU << FLASH_PDKEYR_PDKEYR_Pos)  /*!< 0xFFFFFFFF */
#define FLASH_PDKEYR_PDKEYR          FLASH_PDKEYR_PDKEYR_Msk                   /*!< FLASH_PEC and data matrix Key */

/******************  Bit definition for FLASH_PEKEYR register  ******************/
#define FLASH_PEKEYR_PEKEYR_Pos      (0U)                                      
#define FLASH_PEKEYR_PEKEYR_Msk      (0xFFFFFFFFU << FLASH_PEKEYR_PEKEYR_Pos)  /*!< 0xFFFFFFFF */
#define FLASH_PEKEYR_PEKEYR          FLASH_PEKEYR_PEKEYR_Msk                   /*!< FLASH_PEC and data matrix Key */

/******************  Bit definition for FLASH_PRGKEYR register  ******************/
#define FLASH_PRGKEYR_PRGKEYR_Pos    (0U)                                      
#define FLASH_PRGKEYR_PRGKEYR_Msk    (0xFFFFFFFFU << FLASH_PRGKEYR_PRGKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_PRGKEYR_PRGKEYR        FLASH_PRGKEYR_PRGKEYR_Msk                 /*!< Program matrix Key */

/******************  Bit definition for FLASH_OPTKEYR register  ******************/
#define FLASH_OPTKEYR_OPTKEYR_Pos    (0U)                                      
#define FLASH_OPTKEYR_OPTKEYR_Msk    (0xFFFFFFFFU << FLASH_OPTKEYR_OPTKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_OPTKEYR_OPTKEYR        FLASH_OPTKEYR_OPTKEYR_Msk                 /*!< Option bytes matrix Key */

/******************  Bit definition for FLASH_SR register  *******************/
#define FLASH_SR_BSY_Pos             (0U)                                      
#define FLASH_SR_BSY_Msk             (0x1U << FLASH_SR_BSY_Pos)                /*!< 0x00000001 */
#define FLASH_SR_BSY                 FLASH_SR_BSY_Msk                          /*!< Busy */
#define FLASH_SR_EOP_Pos             (1U)                                      
#define FLASH_SR_EOP_Msk             (0x1U << FLASH_SR_EOP_Pos)                /*!< 0x00000002 */
#define FLASH_SR_EOP                 FLASH_SR_EOP_Msk                          /*!< End Of Programming*/
#define FLASH_SR_HVOFF_Pos           (2U)                                      
#define FLASH_SR_HVOFF_Msk           (0x1U << FLASH_SR_HVOFF_Pos)              /*!< 0x00000004 */
#define FLASH_SR_HVOFF               FLASH_SR_HVOFF_Msk                        /*!< End of high voltage */
#define FLASH_SR_READY_Pos           (3U)                                      
#define FLASH_SR_READY_Msk           (0x1U << FLASH_SR_READY_Pos)              /*!< 0x00000008 */
#define FLASH_SR_READY               FLASH_SR_READY_Msk                        /*!< Flash ready after low power mode */

#define FLASH_SR_WRPERR_Pos          (8U)                                      
#define FLASH_SR_WRPERR_Msk          (0x1U << FLASH_SR_WRPERR_Pos)             /*!< 0x00000100 */
#define FLASH_SR_WRPERR              FLASH_SR_WRPERR_Msk                       /*!< Write protection error */
#define FLASH_SR_PGAERR_Pos          (9U)                                      
#define FLASH_SR_PGAERR_Msk          (0x1U << FLASH_SR_PGAERR_Pos)             /*!< 0x00000200 */
#define FLASH_SR_PGAERR              FLASH_SR_PGAERR_Msk                       /*!< Programming Alignment Error */
#define FLASH_SR_SIZERR_Pos          (10U)                                     
#define FLASH_SR_SIZERR_Msk          (0x1U << FLASH_SR_SIZERR_Pos)             /*!< 0x00000400 */
#define FLASH_SR_SIZERR              FLASH_SR_SIZERR_Msk                       /*!< Size error */
#define FLASH_SR_OPTVERR_Pos         (11U)                                     
#define FLASH_SR_OPTVERR_Msk         (0x1U << FLASH_SR_OPTVERR_Pos)            /*!< 0x00000800 */
#define FLASH_SR_OPTVERR             FLASH_SR_OPTVERR_Msk                      /*!< Option Valid error */
#define FLASH_SR_RDERR_Pos           (13U)                                     
#define FLASH_SR_RDERR_Msk           (0x1U << FLASH_SR_RDERR_Pos)              /*!< 0x00002000 */
#define FLASH_SR_RDERR               FLASH_SR_RDERR_Msk                        /*!< Read protected error */
#define FLASH_SR_NOTZEROERR_Pos      (16U)                                     
#define FLASH_SR_NOTZEROERR_Msk      (0x1U << FLASH_SR_NOTZEROERR_Pos)         /*!< 0x00010000 */
#define FLASH_SR_NOTZEROERR          FLASH_SR_NOTZEROERR_Msk                   /*!< Not Zero error */
#define FLASH_SR_FWWERR_Pos          (17U)                                     
#define FLASH_SR_FWWERR_Msk          (0x1U << FLASH_SR_FWWERR_Pos)             /*!< 0x00020000 */
#define FLASH_SR_FWWERR              FLASH_SR_FWWERR_Msk                       /*!< Write/Errase operation aborted */

/* Legacy defines */
#define FLASH_SR_FWWER                      FLASH_SR_FWWERR
#define FLASH_SR_ENHV                       FLASH_SR_HVOFF
#define FLASH_SR_ENDHV                      FLASH_SR_HVOFF

/******************  Bit definition for FLASH_OPTR register  *******************/
#define FLASH_OPTR_RDPROT_Pos        (0U)                                      
#define FLASH_OPTR_RDPROT_Msk        (0xFFU << FLASH_OPTR_RDPROT_Pos)          /*!< 0x000000FF */
#define FLASH_OPTR_RDPROT            FLASH_OPTR_RDPROT_Msk                     /*!< Read Protection */
#define FLASH_OPTR_WPRMOD_Pos        (8U)                                      
#define FLASH_OPTR_WPRMOD_Msk        (0x1U << FLASH_OPTR_WPRMOD_Pos)           /*!< 0x00000100 */
#define FLASH_OPTR_WPRMOD            FLASH_OPTR_WPRMOD_Msk                     /*!< Selection of protection mode of WPR bits */
#define FLASH_OPTR_BOR_LEV_Pos       (16U)                                     
#define FLASH_OPTR_BOR_LEV_Msk       (0xFU << FLASH_OPTR_BOR_LEV_Pos)          /*!< 0x000F0000 */
#define FLASH_OPTR_BOR_LEV           FLASH_OPTR_BOR_LEV_Msk                    /*!< BOR_LEV[3:0] Brown Out Reset Threshold Level*/
#define FLASH_OPTR_IWDG_SW_Pos       (20U)                                     
#define FLASH_OPTR_IWDG_SW_Msk       (0x1U << FLASH_OPTR_IWDG_SW_Pos)          /*!< 0x00100000 */
#define FLASH_OPTR_IWDG_SW           FLASH_OPTR_IWDG_SW_Msk                    /*!< IWDG_SW */
#define FLASH_OPTR_nRST_STOP_Pos     (21U)                                     
#define FLASH_OPTR_nRST_STOP_Msk     (0x1U << FLASH_OPTR_nRST_STOP_Pos)        /*!< 0x00200000 */
#define FLASH_OPTR_nRST_STOP         FLASH_OPTR_nRST_STOP_Msk                  /*!< nRST_STOP */
#define FLASH_OPTR_nRST_STDBY_Pos    (22U)                                     
#define FLASH_OPTR_nRST_STDBY_Msk    (0x1U << FLASH_OPTR_nRST_STDBY_Pos)       /*!< 0x00400000 */
#define FLASH_OPTR_nRST_STDBY        FLASH_OPTR_nRST_STDBY_Msk                 /*!< nRST_STDBY */
#define FLASH_OPTR_USER_Pos          (20U)                                     
#define FLASH_OPTR_USER_Msk          (0x7U << FLASH_OPTR_USER_Pos)             /*!< 0x00700000 */
#define FLASH_OPTR_USER              FLASH_OPTR_USER_Msk                       /*!< User Option Bytes */
#define FLASH_OPTR_BOOT1_Pos         (31U)                                     
#define FLASH_OPTR_BOOT1_Msk         (0x1U << FLASH_OPTR_BOOT1_Pos)            /*!< 0x80000000 */
#define FLASH_OPTR_BOOT1             FLASH_OPTR_BOOT1_Msk                      /*!< BOOT1 */

/******************  Bit definition for FLASH_WRPR register  ******************/
#define FLASH_WRPR_WRP_Pos           (0U)                                      
#define FLASH_WRPR_WRP_Msk           (0xFFFFU << FLASH_WRPR_WRP_Pos)           /*!< 0x0000FFFF */
#define FLASH_WRPR_WRP               FLASH_WRPR_WRP_Msk                        /*!< Write Protection bits */

/******************************************************************************/
/*                                                                            */
/*                       General Purpose IOs (GPIO)                           */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for GPIO_MODER register  *****************/
#define GPIO_MODER_MODE0_Pos            (0U)                                   
#define GPIO_MODER_MODE0_Msk            (0x3U << GPIO_MODER_MODE0_Pos)         /*!< 0x00000003 */
#define GPIO_MODER_MODE0                GPIO_MODER_MODE0_Msk                   
#define GPIO_MODER_MODE0_0              (0x1U << GPIO_MODER_MODE0_Pos)         /*!< 0x00000001 */
#define GPIO_MODER_MODE0_1              (0x2U << GPIO_MODER_MODE0_Pos)         /*!< 0x00000002 */
#define GPIO_MODER_MODE1_Pos            (2U)                                   
#define GPIO_MODER_MODE1_Msk            (0x3U << GPIO_MODER_MODE1_Pos)         /*!< 0x0000000C */
#define GPIO_MODER_MODE1                GPIO_MODER_MODE1_Msk                   
#define GPIO_MODER_MODE1_0              (0x1U << GPIO_MODER_MODE1_Pos)         /*!< 0x00000004 */
#define GPIO_MODER_MODE1_1              (0x2U << GPIO_MODER_MODE1_Pos)         /*!< 0x00000008 */
#define GPIO_MODER_MODE2_Pos            (4U)                                   
#define GPIO_MODER_MODE2_Msk            (0x3U << GPIO_MODER_MODE2_Pos)         /*!< 0x00000030 */
#define GPIO_MODER_MODE2                GPIO_MODER_MODE2_Msk                   
#define GPIO_MODER_MODE2_0              (0x1U << GPIO_MODER_MODE2_Pos)         /*!< 0x00000010 */
#define GPIO_MODER_MODE2_1              (0x2U << GPIO_MODER_MODE2_Pos)         /*!< 0x00000020 */
#define GPIO_MODER_MODE3_Pos            (6U)                                   
#define GPIO_MODER_MODE3_Msk            (0x3U << GPIO_MODER_MODE3_Pos)         /*!< 0x000000C0 */
#define GPIO_MODER_MODE3                GPIO_MODER_MODE3_Msk                   
#define GPIO_MODER_MODE3_0              (0x1U << GPIO_MODER_MODE3_Pos)         /*!< 0x00000040 */
#define GPIO_MODER_MODE3_1              (0x2U << GPIO_MODER_MODE3_Pos)         /*!< 0x00000080 */
#define GPIO_MODER_MODE4_Pos            (8U)                                   
#define GPIO_MODER_MODE4_Msk            (0x3U << GPIO_MODER_MODE4_Pos)         /*!< 0x00000300 */
#define GPIO_MODER_MODE4                GPIO_MODER_MODE4_Msk                   
#define GPIO_MODER_MODE4_0              (0x1U << GPIO_MODER_MODE4_Pos)         /*!< 0x00000100 */
#define GPIO_MODER_MODE4_1              (0x2U << GPIO_MODER_MODE4_Pos)         /*!< 0x00000200 */
#define GPIO_MODER_MODE5_Pos            (10U)                                  
#define GPIO_MODER_MODE5_Msk            (0x3U << GPIO_MODER_MODE5_Pos)         /*!< 0x00000C00 */
#define GPIO_MODER_MODE5                GPIO_MODER_MODE5_Msk                   
#define GPIO_MODER_MODE5_0              (0x1U << GPIO_MODER_MODE5_Pos)         /*!< 0x00000400 */
#define GPIO_MODER_MODE5_1              (0x2U << GPIO_MODER_MODE5_Pos)         /*!< 0x00000800 */
#define GPIO_MODER_MODE6_Pos            (12U)                                  
#define GPIO_MODER_MODE6_Msk            (0x3U << GPIO_MODER_MODE6_Pos)         /*!< 0x00003000 */
#define GPIO_MODER_MODE6                GPIO_MODER_MODE6_Msk                   
#define GPIO_MODER_MODE6_0              (0x1U << GPIO_MODER_MODE6_Pos)         /*!< 0x00001000 */
#define GPIO_MODER_MODE6_1              (0x2U << GPIO_MODER_MODE6_Pos)         /*!< 0x00002000 */
#define GPIO_MODER_MODE7_Pos            (14U)                                  
#define GPIO_MODER_MODE7_Msk            (0x3U << GPIO_MODER_MODE7_Pos)         /*!< 0x0000C000 */
#define GPIO_MODER_MODE7                GPIO_MODER_MODE7_Msk                   
#define GPIO_MODER_MODE7_0              (0x1U << GPIO_MODER_MODE7_Pos)         /*!< 0x00004000 */
#define GPIO_MODER_MODE7_1              (0x2U << GPIO_MODER_MODE7_Pos)         /*!< 0x00008000 */
#define GPIO_MODER_MODE8_Pos            (16U)                                  
#define GPIO_MODER_MODE8_Msk            (0x3U << GPIO_MODER_MODE8_Pos)         /*!< 0x00030000 */
#define GPIO_MODER_MODE8                GPIO_MODER_MODE8_Msk                   
#define GPIO_MODER_MODE8_0              (0x1U << GPIO_MODER_MODE8_Pos)         /*!< 0x00010000 */
#define GPIO_MODER_MODE8_1              (0x2U << GPIO_MODER_MODE8_Pos)         /*!< 0x00020000 */
#define GPIO_MODER_MODE9_Pos            (18U)                                  
#define GPIO_MODER_MODE9_Msk            (0x3U << GPIO_MODER_MODE9_Pos)         /*!< 0x000C0000 */
#define GPIO_MODER_MODE9                GPIO_MODER_MODE9_Msk                   
#define GPIO_MODER_MODE9_0              (0x1U << GPIO_MODER_MODE9_Pos)         /*!< 0x00040000 */
#define GPIO_MODER_MODE9_1              (0x2U << GPIO_MODER_MODE9_Pos)         /*!< 0x00080000 */
#define GPIO_MODER_MODE10_Pos           (20U)                                  
#define GPIO_MODER_MODE10_Msk           (0x3U << GPIO_MODER_MODE10_Pos)        /*!< 0x00300000 */
#define GPIO_MODER_MODE10               GPIO_MODER_MODE10_Msk                  
#define GPIO_MODER_MODE10_0             (0x1U << GPIO_MODER_MODE10_Pos)        /*!< 0x00100000 */
#define GPIO_MODER_MODE10_1             (0x2U << GPIO_MODER_MODE10_Pos)        /*!< 0x00200000 */
#define GPIO_MODER_MODE11_Pos           (22U)                                  
#define GPIO_MODER_MODE11_Msk           (0x3U << GPIO_MODER_MODE11_Pos)        /*!< 0x00C00000 */
#define GPIO_MODER_MODE11               GPIO_MODER_MODE11_Msk                  
#define GPIO_MODER_MODE11_0             (0x1U << GPIO_MODER_MODE11_Pos)        /*!< 0x00400000 */
#define GPIO_MODER_MODE11_1             (0x2U << GPIO_MODER_MODE11_Pos)        /*!< 0x00800000 */
#define GPIO_MODER_MODE12_Pos           (24U)                                  
#define GPIO_MODER_MODE12_Msk           (0x3U << GPIO_MODER_MODE12_Pos)        /*!< 0x03000000 */
#define GPIO_MODER_MODE12               GPIO_MODER_MODE12_Msk                  
#define GPIO_MODER_MODE12_0             (0x1U << GPIO_MODER_MODE12_Pos)        /*!< 0x01000000 */
#define GPIO_MODER_MODE12_1             (0x2U << GPIO_MODER_MODE12_Pos)        /*!< 0x02000000 */
#define GPIO_MODER_MODE13_Pos           (26U)                                  
#define GPIO_MODER_MODE13_Msk           (0x3U << GPIO_MODER_MODE13_Pos)        /*!< 0x0C000000 */
#define GPIO_MODER_MODE13               GPIO_MODER_MODE13_Msk                  
#define GPIO_MODER_MODE13_0             (0x1U << GPIO_MODER_MODE13_Pos)        /*!< 0x04000000 */
#define GPIO_MODER_MODE13_1             (0x2U << GPIO_MODER_MODE13_Pos)        /*!< 0x08000000 */
#define GPIO_MODER_MODE14_Pos           (28U)                                  
#define GPIO_MODER_MODE14_Msk           (0x3U << GPIO_MODER_MODE14_Pos)        /*!< 0x30000000 */
#define GPIO_MODER_MODE14               GPIO_MODER_MODE14_Msk                  
#define GPIO_MODER_MODE14_0             (0x1U << GPIO_MODER_MODE14_Pos)        /*!< 0x10000000 */
#define GPIO_MODER_MODE14_1             (0x2U << GPIO_MODER_MODE14_Pos)        /*!< 0x20000000 */
#define GPIO_MODER_MODE15_Pos           (30U)                                  
#define GPIO_MODER_MODE15_Msk           (0x3U << GPIO_MODER_MODE15_Pos)        /*!< 0xC0000000 */
#define GPIO_MODER_MODE15               GPIO_MODER_MODE15_Msk                  
#define GPIO_MODER_MODE15_0             (0x1U << GPIO_MODER_MODE15_Pos)        /*!< 0x40000000 */
#define GPIO_MODER_MODE15_1             (0x2U << GPIO_MODER_MODE15_Pos)        /*!< 0x80000000 */

/******************  Bit definition for GPIO_OTYPER register  *****************/
#define GPIO_OTYPER_OT_0                (0x00000001U)                          
#define GPIO_OTYPER_OT_1                (0x00000002U)                          
#define GPIO_OTYPER_OT_2                (0x00000004U)                          
#define GPIO_OTYPER_OT_3                (0x00000008U)                          
#define GPIO_OTYPER_OT_4                (0x00000010U)                          
#define GPIO_OTYPER_OT_5                (0x00000020U)                          
#define GPIO_OTYPER_OT_6                (0x00000040U)                          
#define GPIO_OTYPER_OT_7                (0x00000080U)                          
#define GPIO_OTYPER_OT_8                (0x00000100U)                          
#define GPIO_OTYPER_OT_9                (0x00000200U)                          
#define GPIO_OTYPER_OT_10               (0x00000400U)                          
#define GPIO_OTYPER_OT_11               (0x00000800U)                          
#define GPIO_OTYPER_OT_12               (0x00001000U)                          
#define GPIO_OTYPER_OT_13               (0x00002000U)                          
#define GPIO_OTYPER_OT_14               (0x00004000U)                          
#define GPIO_OTYPER_OT_15               (0x00008000U)                          

/****************  Bit definition for GPIO_OSPEEDR register  ******************/
#define GPIO_OSPEEDER_OSPEED0_Pos       (0U)                                   
#define GPIO_OSPEEDER_OSPEED0_Msk       (0x3U << GPIO_OSPEEDER_OSPEED0_Pos)    /*!< 0x00000003 */
#define GPIO_OSPEEDER_OSPEED0           GPIO_OSPEEDER_OSPEED0_Msk              
#define GPIO_OSPEEDER_OSPEED0_0         (0x1U << GPIO_OSPEEDER_OSPEED0_Pos)    /*!< 0x00000001 */
#define GPIO_OSPEEDER_OSPEED0_1         (0x2U << GPIO_OSPEEDER_OSPEED0_Pos)    /*!< 0x00000002 */
#define GPIO_OSPEEDER_OSPEED1_Pos       (2U)                                   
#define GPIO_OSPEEDER_OSPEED1_Msk       (0x3U << GPIO_OSPEEDER_OSPEED1_Pos)    /*!< 0x0000000C */
#define GPIO_OSPEEDER_OSPEED1           GPIO_OSPEEDER_OSPEED1_Msk              
#define GPIO_OSPEEDER_OSPEED1_0         (0x1U << GPIO_OSPEEDER_OSPEED1_Pos)    /*!< 0x00000004 */
#define GPIO_OSPEEDER_OSPEED1_1         (0x2U << GPIO_OSPEEDER_OSPEED1_Pos)    /*!< 0x00000008 */
#define GPIO_OSPEEDER_OSPEED2_Pos       (4U)                                   
#define GPIO_OSPEEDER_OSPEED2_Msk       (0x3U << GPIO_OSPEEDER_OSPEED2_Pos)    /*!< 0x00000030 */
#define GPIO_OSPEEDER_OSPEED2           GPIO_OSPEEDER_OSPEED2_Msk              
#define GPIO_OSPEEDER_OSPEED2_0         (0x1U << GPIO_OSPEEDER_OSPEED2_Pos)    /*!< 0x00000010 */
#define GPIO_OSPEEDER_OSPEED2_1         (0x2U << GPIO_OSPEEDER_OSPEED2_Pos)    /*!< 0x00000020 */
#define GPIO_OSPEEDER_OSPEED3_Pos       (6U)                                   
#define GPIO_OSPEEDER_OSPEED3_Msk       (0x3U << GPIO_OSPEEDER_OSPEED3_Pos)    /*!< 0x000000C0 */
#define GPIO_OSPEEDER_OSPEED3           GPIO_OSPEEDER_OSPEED3_Msk              
#define GPIO_OSPEEDER_OSPEED3_0         (0x1U << GPIO_OSPEEDER_OSPEED3_Pos)    /*!< 0x00000040 */
#define GPIO_OSPEEDER_OSPEED3_1         (0x2U << GPIO_OSPEEDER_OSPEED3_Pos)    /*!< 0x00000080 */
#define GPIO_OSPEEDER_OSPEED4_Pos       (8U)                                   
#define GPIO_OSPEEDER_OSPEED4_Msk       (0x3U << GPIO_OSPEEDER_OSPEED4_Pos)    /*!< 0x00000300 */
#define GPIO_OSPEEDER_OSPEED4           GPIO_OSPEEDER_OSPEED4_Msk              
#define GPIO_OSPEEDER_OSPEED4_0         (0x1U << GPIO_OSPEEDER_OSPEED4_Pos)    /*!< 0x00000100 */
#define GPIO_OSPEEDER_OSPEED4_1         (0x2U << GPIO_OSPEEDER_OSPEED4_Pos)    /*!< 0x00000200 */
#define GPIO_OSPEEDER_OSPEED5_Pos       (10U)                                  
#define GPIO_OSPEEDER_OSPEED5_Msk       (0x3U << GPIO_OSPEEDER_OSPEED5_Pos)    /*!< 0x00000C00 */
#define GPIO_OSPEEDER_OSPEED5           GPIO_OSPEEDER_OSPEED5_Msk              
#define GPIO_OSPEEDER_OSPEED5_0         (0x1U << GPIO_OSPEEDER_OSPEED5_Pos)    /*!< 0x00000400 */
#define GPIO_OSPEEDER_OSPEED5_1         (0x2U << GPIO_OSPEEDER_OSPEED5_Pos)    /*!< 0x00000800 */
#define GPIO_OSPEEDER_OSPEED6_Pos       (12U)                                  
#define GPIO_OSPEEDER_OSPEED6_Msk       (0x3U << GPIO_OSPEEDER_OSPEED6_Pos)    /*!< 0x00003000 */
#define GPIO_OSPEEDER_OSPEED6           GPIO_OSPEEDER_OSPEED6_Msk              
#define GPIO_OSPEEDER_OSPEED6_0         (0x1U << GPIO_OSPEEDER_OSPEED6_Pos)    /*!< 0x00001000 */
#define GPIO_OSPEEDER_OSPEED6_1         (0x2U << GPIO_OSPEEDER_OSPEED6_Pos)    /*!< 0x00002000 */
#define GPIO_OSPEEDER_OSPEED7_Pos       (14U)                                  
#define GPIO_OSPEEDER_OSPEED7_Msk       (0x3U << GPIO_OSPEEDER_OSPEED7_Pos)    /*!< 0x0000C000 */
#define GPIO_OSPEEDER_OSPEED7           GPIO_OSPEEDER_OSPEED7_Msk              
#define GPIO_OSPEEDER_OSPEED7_0         (0x1U << GPIO_OSPEEDER_OSPEED7_Pos)    /*!< 0x00004000 */
#define GPIO_OSPEEDER_OSPEED7_1         (0x2U << GPIO_OSPEEDER_OSPEED7_Pos)    /*!< 0x00008000 */
#define GPIO_OSPEEDER_OSPEED8_Pos       (16U)                                  
#define GPIO_OSPEEDER_OSPEED8_Msk       (0x3U << GPIO_OSPEEDER_OSPEED8_Pos)    /*!< 0x00030000 */
#define GPIO_OSPEEDER_OSPEED8           GPIO_OSPEEDER_OSPEED8_Msk              
#define GPIO_OSPEEDER_OSPEED8_0         (0x1U << GPIO_OSPEEDER_OSPEED8_Pos)    /*!< 0x00010000 */
#define GPIO_OSPEEDER_OSPEED8_1         (0x2U << GPIO_OSPEEDER_OSPEED8_Pos)    /*!< 0x00020000 */
#define GPIO_OSPEEDER_OSPEED9_Pos       (18U)                                  
#define GPIO_OSPEEDER_OSPEED9_Msk       (0x3U << GPIO_OSPEEDER_OSPEED9_Pos)    /*!< 0x000C0000 */
#define GPIO_OSPEEDER_OSPEED9           GPIO_OSPEEDER_OSPEED9_Msk              
#define GPIO_OSPEEDER_OSPEED9_0         (0x1U << GPIO_OSPEEDER_OSPEED9_Pos)    /*!< 0x00040000 */
#define GPIO_OSPEEDER_OSPEED9_1         (0x2U << GPIO_OSPEEDER_OSPEED9_Pos)    /*!< 0x00080000 */
#define GPIO_OSPEEDER_OSPEED10_Pos      (20U)                                  
#define GPIO_OSPEEDER_OSPEED10_Msk      (0x3U << GPIO_OSPEEDER_OSPEED10_Pos)   /*!< 0x00300000 */
#define GPIO_OSPEEDER_OSPEED10          GPIO_OSPEEDER_OSPEED10_Msk             
#define GPIO_OSPEEDER_OSPEED10_0        (0x1U << GPIO_OSPEEDER_OSPEED10_Pos)   /*!< 0x00100000 */
#define GPIO_OSPEEDER_OSPEED10_1        (0x2U << GPIO_OSPEEDER_OSPEED10_Pos)   /*!< 0x00200000 */
#define GPIO_OSPEEDER_OSPEED11_Pos      (22U)                                  
#define GPIO_OSPEEDER_OSPEED11_Msk      (0x3U << GPIO_OSPEEDER_OSPEED11_Pos)   /*!< 0x00C00000 */
#define GPIO_OSPEEDER_OSPEED11          GPIO_OSPEEDER_OSPEED11_Msk             
#define GPIO_OSPEEDER_OSPEED11_0        (0x1U << GPIO_OSPEEDER_OSPEED11_Pos)   /*!< 0x00400000 */
#define GPIO_OSPEEDER_OSPEED11_1        (0x2U << GPIO_OSPEEDER_OSPEED11_Pos)   /*!< 0x00800000 */
#define GPIO_OSPEEDER_OSPEED12_Pos      (24U)                                  
#define GPIO_OSPEEDER_OSPEED12_Msk      (0x3U << GPIO_OSPEEDER_OSPEED12_Pos)   /*!< 0x03000000 */
#define GPIO_OSPEEDER_OSPEED12          GPIO_OSPEEDER_OSPEED12_Msk             
#define GPIO_OSPEEDER_OSPEED12_0        (0x1U << GPIO_OSPEEDER_OSPEED12_Pos)   /*!< 0x01000000 */
#define GPIO_OSPEEDER_OSPEED12_1        (0x2U << GPIO_OSPEEDER_OSPEED12_Pos)   /*!< 0x02000000 */
#define GPIO_OSPEEDER_OSPEED13_Pos      (26U)                                  
#define GPIO_OSPEEDER_OSPEED13_Msk      (0x3U << GPIO_OSPEEDER_OSPEED13_Pos)   /*!< 0x0C000000 */
#define GPIO_OSPEEDER_OSPEED13          GPIO_OSPEEDER_OSPEED13_Msk             
#define GPIO_OSPEEDER_OSPEED13_0        (0x1U << GPIO_OSPEEDER_OSPEED13_Pos)   /*!< 0x04000000 */
#define GPIO_OSPEEDER_OSPEED13_1        (0x2U << GPIO_OSPEEDER_OSPEED13_Pos)   /*!< 0x08000000 */
#define GPIO_OSPEEDER_OSPEED14_Pos      (28U)                                  
#define GPIO_OSPEEDER_OSPEED14_Msk      (0x3U << GPIO_OSPEEDER_OSPEED14_Pos)   /*!< 0x30000000 */
#define GPIO_OSPEEDER_OSPEED14          GPIO_OSPEEDER_OSPEED14_Msk             
#define GPIO_OSPEEDER_OSPEED14_0        (0x1U << GPIO_OSPEEDER_OSPEED14_Pos)   /*!< 0x10000000 */
#define GPIO_OSPEEDER_OSPEED14_1        (0x2U << GPIO_OSPEEDER_OSPEED14_Pos)   /*!< 0x20000000 */
#define GPIO_OSPEEDER_OSPEED15_Pos      (30U)                                  
#define GPIO_OSPEEDER_OSPEED15_Msk      (0x3U << GPIO_OSPEEDER_OSPEED15_Pos)   /*!< 0xC0000000 */
#define GPIO_OSPEEDER_OSPEED15          GPIO_OSPEEDER_OSPEED15_Msk             
#define GPIO_OSPEEDER_OSPEED15_0        (0x1U << GPIO_OSPEEDER_OSPEED15_Pos)   /*!< 0x40000000 */
#define GPIO_OSPEEDER_OSPEED15_1        (0x2U << GPIO_OSPEEDER_OSPEED15_Pos)   /*!< 0x80000000 */

/*******************  Bit definition for GPIO_PUPDR register ******************/
#define GPIO_PUPDR_PUPD0_Pos            (0U)                                   
#define GPIO_PUPDR_PUPD0_Msk            (0x3U << GPIO_PUPDR_PUPD0_Pos)         /*!< 0x00000003 */
#define GPIO_PUPDR_PUPD0                GPIO_PUPDR_PUPD0_Msk                   
#define GPIO_PUPDR_PUPD0_0              (0x1U << GPIO_PUPDR_PUPD0_Pos)         /*!< 0x00000001 */
#define GPIO_PUPDR_PUPD0_1              (0x2U << GPIO_PUPDR_PUPD0_Pos)         /*!< 0x00000002 */
#define GPIO_PUPDR_PUPD1_Pos            (2U)                                   
#define GPIO_PUPDR_PUPD1_Msk            (0x3U << GPIO_PUPDR_PUPD1_Pos)         /*!< 0x0000000C */
#define GPIO_PUPDR_PUPD1                GPIO_PUPDR_PUPD1_Msk                   
#define GPIO_PUPDR_PUPD1_0              (0x1U << GPIO_PUPDR_PUPD1_Pos)         /*!< 0x00000004 */
#define GPIO_PUPDR_PUPD1_1              (0x2U << GPIO_PUPDR_PUPD1_Pos)         /*!< 0x00000008 */
#define GPIO_PUPDR_PUPD2_Pos            (4U)                                   
#define GPIO_PUPDR_PUPD2_Msk            (0x3U << GPIO_PUPDR_PUPD2_Pos)         /*!< 0x00000030 */
#define GPIO_PUPDR_PUPD2                GPIO_PUPDR_PUPD2_Msk                   
#define GPIO_PUPDR_PUPD2_0              (0x1U << GPIO_PUPDR_PUPD2_Pos)         /*!< 0x00000010 */
#define GPIO_PUPDR_PUPD2_1              (0x2U << GPIO_PUPDR_PUPD2_Pos)         /*!< 0x00000020 */
#define GPIO_PUPDR_PUPD3_Pos            (6U)                                   
#define GPIO_PUPDR_PUPD3_Msk            (0x3U << GPIO_PUPDR_PUPD3_Pos)         /*!< 0x000000C0 */
#define GPIO_PUPDR_PUPD3                GPIO_PUPDR_PUPD3_Msk                   
#define GPIO_PUPDR_PUPD3_0              (0x1U << GPIO_PUPDR_PUPD3_Pos)         /*!< 0x00000040 */
#define GPIO_PUPDR_PUPD3_1              (0x2U << GPIO_PUPDR_PUPD3_Pos)         /*!< 0x00000080 */
#define GPIO_PUPDR_PUPD4_Pos            (8U)                                   
#define GPIO_PUPDR_PUPD4_Msk            (0x3U << GPIO_PUPDR_PUPD4_Pos)         /*!< 0x00000300 */
#define GPIO_PUPDR_PUPD4                GPIO_PUPDR_PUPD4_Msk                   
#define GPIO_PUPDR_PUPD4_0              (0x1U << GPIO_PUPDR_PUPD4_Pos)         /*!< 0x00000100 */
#define GPIO_PUPDR_PUPD4_1              (0x2U << GPIO_PUPDR_PUPD4_Pos)         /*!< 0x00000200 */
#define GPIO_PUPDR_PUPD5_Pos            (10U)                                  
#define GPIO_PUPDR_PUPD5_Msk            (0x3U << GPIO_PUPDR_PUPD5_Pos)         /*!< 0x00000C00 */
#define GPIO_PUPDR_PUPD5                GPIO_PUPDR_PUPD5_Msk                   
#define GPIO_PUPDR_PUPD5_0              (0x1U << GPIO_PUPDR_PUPD5_Pos)         /*!< 0x00000400 */
#define GPIO_PUPDR_PUPD5_1              (0x2U << GPIO_PUPDR_PUPD5_Pos)         /*!< 0x00000800 */
#define GPIO_PUPDR_PUPD6_Pos            (12U)                                  
#define GPIO_PUPDR_PUPD6_Msk            (0x3U << GPIO_PUPDR_PUPD6_Pos)         /*!< 0x00003000 */
#define GPIO_PUPDR_PUPD6                GPIO_PUPDR_PUPD6_Msk                   
#define GPIO_PUPDR_PUPD6_0              (0x1U << GPIO_PUPDR_PUPD6_Pos)         /*!< 0x00001000 */
#define GPIO_PUPDR_PUPD6_1              (0x2U << GPIO_PUPDR_PUPD6_Pos)         /*!< 0x00002000 */
#define GPIO_PUPDR_PUPD7_Pos            (14U)                                  
#define GPIO_PUPDR_PUPD7_Msk            (0x3U << GPIO_PUPDR_PUPD7_Pos)         /*!< 0x0000C000 */
#define GPIO_PUPDR_PUPD7                GPIO_PUPDR_PUPD7_Msk                   
#define GPIO_PUPDR_PUPD7_0              (0x1U << GPIO_PUPDR_PUPD7_Pos)         /*!< 0x00004000 */
#define GPIO_PUPDR_PUPD7_1              (0x2U << GPIO_PUPDR_PUPD7_Pos)         /*!< 0x00008000 */
#define GPIO_PUPDR_PUPD8_Pos            (16U)                                  
#define GPIO_PUPDR_PUPD8_Msk            (0x3U << GPIO_PUPDR_PUPD8_Pos)         /*!< 0x00030000 */
#define GPIO_PUPDR_PUPD8                GPIO_PUPDR_PUPD8_Msk                   
#define GPIO_PUPDR_PUPD8_0              (0x1U << GPIO_PUPDR_PUPD8_Pos)         /*!< 0x00010000 */
#define GPIO_PUPDR_PUPD8_1              (0x2U << GPIO_PUPDR_PUPD8_Pos)         /*!< 0x00020000 */
#define GPIO_PUPDR_PUPD9_Pos            (18U)                                  
#define GPIO_PUPDR_PUPD9_Msk            (0x3U << GPIO_PUPDR_PUPD9_Pos)         /*!< 0x000C0000 */
#define GPIO_PUPDR_PUPD9                GPIO_PUPDR_PUPD9_Msk                   
#define GPIO_PUPDR_PUPD9_0              (0x1U << GPIO_PUPDR_PUPD9_Pos)         /*!< 0x00040000 */
#define GPIO_PUPDR_PUPD9_1              (0x2U << GPIO_PUPDR_PUPD9_Pos)         /*!< 0x00080000 */
#define GPIO_PUPDR_PUPD10_Pos           (20U)                                  
#define GPIO_PUPDR_PUPD10_Msk           (0x3U << GPIO_PUPDR_PUPD10_Pos)        /*!< 0x00300000 */
#define GPIO_PUPDR_PUPD10               GPIO_PUPDR_PUPD10_Msk                  
#define GPIO_PUPDR_PUPD10_0             (0x1U << GPIO_PUPDR_PUPD10_Pos)        /*!< 0x00100000 */
#define GPIO_PUPDR_PUPD10_1             (0x2U << GPIO_PUPDR_PUPD10_Pos)        /*!< 0x00200000 */
#define GPIO_PUPDR_PUPD11_Pos           (22U)                                  
#define GPIO_PUPDR_PUPD11_Msk           (0x3U << GPIO_PUPDR_PUPD11_Pos)        /*!< 0x00C00000 */
#define GPIO_PUPDR_PUPD11               GPIO_PUPDR_PUPD11_Msk                  
#define GPIO_PUPDR_PUPD11_0             (0x1U << GPIO_PUPDR_PUPD11_Pos)        /*!< 0x00400000 */
#define GPIO_PUPDR_PUPD11_1             (0x2U << GPIO_PUPDR_PUPD11_Pos)        /*!< 0x00800000 */
#define GPIO_PUPDR_PUPD12_Pos           (24U)                                  
#define GPIO_PUPDR_PUPD12_Msk           (0x3U << GPIO_PUPDR_PUPD12_Pos)        /*!< 0x03000000 */
#define GPIO_PUPDR_PUPD12               GPIO_PUPDR_PUPD12_Msk                  
#define GPIO_PUPDR_PUPD12_0             (0x1U << GPIO_PUPDR_PUPD12_Pos)        /*!< 0x01000000 */
#define GPIO_PUPDR_PUPD12_1             (0x2U << GPIO_PUPDR_PUPD12_Pos)        /*!< 0x02000000 */
#define GPIO_PUPDR_PUPD13_Pos           (26U)                                  
#define GPIO_PUPDR_PUPD13_Msk           (0x3U << GPIO_PUPDR_PUPD13_Pos)        /*!< 0x0C000000 */
#define GPIO_PUPDR_PUPD13               GPIO_PUPDR_PUPD13_Msk                  
#define GPIO_PUPDR_PUPD13_0             (0x1U << GPIO_PUPDR_PUPD13_Pos)        /*!< 0x04000000 */
#define GPIO_PUPDR_PUPD13_1             (0x2U << GPIO_PUPDR_PUPD13_Pos)        /*!< 0x08000000 */
#define GPIO_PUPDR_PUPD14_Pos           (28U)                                  
#define GPIO_PUPDR_PUPD14_Msk           (0x3U << GPIO_PUPDR_PUPD14_Pos)        /*!< 0x30000000 */
#define GPIO_PUPDR_PUPD14               GPIO_PUPDR_PUPD14_Msk                  
#define GPIO_PUPDR_PUPD14_0             (0x1U << GPIO_PUPDR_PUPD14_Pos)        /*!< 0x10000000 */
#define GPIO_PUPDR_PUPD14_1             (0x2U << GPIO_PUPDR_PUPD14_Pos)        /*!< 0x20000000 */
#define GPIO_PUPDR_PUPD15_Pos           (30U)                                  
#define GPIO_PUPDR_PUPD15_Msk           (0x3U << GPIO_PUPDR_PUPD15_Pos)        /*!< 0xC0000000 */
#define GPIO_PUPDR_PUPD15               GPIO_PUPDR_PUPD15_Msk                  
#define GPIO_PUPDR_PUPD15_0             (0x1U << GPIO_PUPDR_PUPD15_Pos)        /*!< 0x40000000 */
#define GPIO_PUPDR_PUPD15_1             (0x2U << GPIO_PUPDR_PUPD15_Pos)        /*!< 0x80000000 */

/*******************  Bit definition for GPIO_IDR register  *******************/
#define GPIO_IDR_ID0_Pos                (0U)                                   
#define GPIO_IDR_ID0_Msk                (0x1U << GPIO_IDR_ID0_Pos)             /*!< 0x00000001 */
#define GPIO_IDR_ID0                    GPIO_IDR_ID0_Msk                       
#define GPIO_IDR_ID1_Pos                (1U)                                   
#define GPIO_IDR_ID1_Msk                (0x1U << GPIO_IDR_ID1_Pos)             /*!< 0x00000002 */
#define GPIO_IDR_ID1                    GPIO_IDR_ID1_Msk                       
#define GPIO_IDR_ID2_Pos                (2U)                                   
#define GPIO_IDR_ID2_Msk                (0x1U << GPIO_IDR_ID2_Pos)             /*!< 0x00000004 */
#define GPIO_IDR_ID2                    GPIO_IDR_ID2_Msk                       
#define GPIO_IDR_ID3_Pos                (3U)                                   
#define GPIO_IDR_ID3_Msk                (0x1U << GPIO_IDR_ID3_Pos)             /*!< 0x00000008 */
#define GPIO_IDR_ID3                    GPIO_IDR_ID3_Msk                       
#define GPIO_IDR_ID4_Pos                (4U)                                   
#define GPIO_IDR_ID4_Msk                (0x1U << GPIO_IDR_ID4_Pos)             /*!< 0x00000010 */
#define GPIO_IDR_ID4                    GPIO_IDR_ID4_Msk                       
#define GPIO_IDR_ID5_Pos                (5U)                                   
#define GPIO_IDR_ID5_Msk                (0x1U << GPIO_IDR_ID5_Pos)             /*!< 0x00000020 */
#define GPIO_IDR_ID5                    GPIO_IDR_ID5_Msk                       
#define GPIO_IDR_ID6_Pos                (6U)                                   
#define GPIO_IDR_ID6_Msk                (0x1U << GPIO_IDR_ID6_Pos)             /*!< 0x00000040 */
#define GPIO_IDR_ID6                    GPIO_IDR_ID6_Msk                       
#define GPIO_IDR_ID7_Pos                (7U)                                   
#define GPIO_IDR_ID7_Msk                (0x1U << GPIO_IDR_ID7_Pos)             /*!< 0x00000080 */
#define GPIO_IDR_ID7                    GPIO_IDR_ID7_Msk                       
#define GPIO_IDR_ID8_Pos                (8U)                                   
#define GPIO_IDR_ID8_Msk                (0x1U << GPIO_IDR_ID8_Pos)             /*!< 0x00000100 */
#define GPIO_IDR_ID8                    GPIO_IDR_ID8_Msk                       
#define GPIO_IDR_ID9_Pos                (9U)                                   
#define GPIO_IDR_ID9_Msk                (0x1U << GPIO_IDR_ID9_Pos)             /*!< 0x00000200 */
#define GPIO_IDR_ID9                    GPIO_IDR_ID9_Msk                       
#define GPIO_IDR_ID10_Pos               (10U)                                  
#define GPIO_IDR_ID10_Msk               (0x1U << GPIO_IDR_ID10_Pos)            /*!< 0x00000400 */
#define GPIO_IDR_ID10                   GPIO_IDR_ID10_Msk                      
#define GPIO_IDR_ID11_Pos               (11U)                                  
#define GPIO_IDR_ID11_Msk               (0x1U << GPIO_IDR_ID11_Pos)            /*!< 0x00000800 */
#define GPIO_IDR_ID11                   GPIO_IDR_ID11_Msk                      
#define GPIO_IDR_ID12_Pos               (12U)                                  
#define GPIO_IDR_ID12_Msk               (0x1U << GPIO_IDR_ID12_Pos)            /*!< 0x00001000 */
#define GPIO_IDR_ID12                   GPIO_IDR_ID12_Msk                      
#define GPIO_IDR_ID13_Pos               (13U)                                  
#define GPIO_IDR_ID13_Msk               (0x1U << GPIO_IDR_ID13_Pos)            /*!< 0x00002000 */
#define GPIO_IDR_ID13                   GPIO_IDR_ID13_Msk                      
#define GPIO_IDR_ID14_Pos               (14U)                                  
#define GPIO_IDR_ID14_Msk               (0x1U << GPIO_IDR_ID14_Pos)            /*!< 0x00004000 */
#define GPIO_IDR_ID14                   GPIO_IDR_ID14_Msk                      
#define GPIO_IDR_ID15_Pos               (15U)                                  
#define GPIO_IDR_ID15_Msk               (0x1U << GPIO_IDR_ID15_Pos)            /*!< 0x00008000 */
#define GPIO_IDR_ID15                   GPIO_IDR_ID15_Msk                      

/******************  Bit definition for GPIO_ODR register  ********************/
#define GPIO_ODR_OD0_Pos                (0U)                                   
#define GPIO_ODR_OD0_Msk                (0x1U << GPIO_ODR_OD0_Pos)             /*!< 0x00000001 */
#define GPIO_ODR_OD0                    GPIO_ODR_OD0_Msk                       
#define GPIO_ODR_OD1_Pos                (1U)                                   
#define GPIO_ODR_OD1_Msk                (0x1U << GPIO_ODR_OD1_Pos)             /*!< 0x00000002 */
#define GPIO_ODR_OD1                    GPIO_ODR_OD1_Msk                       
#define GPIO_ODR_OD2_Pos                (2U)                                   
#define GPIO_ODR_OD2_Msk                (0x1U << GPIO_ODR_OD2_Pos)             /*!< 0x00000004 */
#define GPIO_ODR_OD2                    GPIO_ODR_OD2_Msk                       
#define GPIO_ODR_OD3_Pos                (3U)                                   
#define GPIO_ODR_OD3_Msk                (0x1U << GPIO_ODR_OD3_Pos)             /*!< 0x00000008 */
#define GPIO_ODR_OD3                    GPIO_ODR_OD3_Msk                       
#define GPIO_ODR_OD4_Pos                (4U)                                   
#define GPIO_ODR_OD4_Msk                (0x1U << GPIO_ODR_OD4_Pos)             /*!< 0x00000010 */
#define GPIO_ODR_OD4                    GPIO_ODR_OD4_Msk                       
#define GPIO_ODR_OD5_Pos                (5U)                                   
#define GPIO_ODR_OD5_Msk                (0x1U << GPIO_ODR_OD5_Pos)             /*!< 0x00000020 */
#define GPIO_ODR_OD5                    GPIO_ODR_OD5_Msk                       
#define GPIO_ODR_OD6_Pos                (6U)                                   
#define GPIO_ODR_OD6_Msk                (0x1U << GPIO_ODR_OD6_Pos)             /*!< 0x00000040 */
#define GPIO_ODR_OD6                    GPIO_ODR_OD6_Msk                       
#define GPIO_ODR_OD7_Pos                (7U)                                   
#define GPIO_ODR_OD7_Msk                (0x1U << GPIO_ODR_OD7_Pos)             /*!< 0x00000080 */
#define GPIO_ODR_OD7                    GPIO_ODR_OD7_Msk                       
#define GPIO_ODR_OD8_Pos                (8U)                                   
#define GPIO_ODR_OD8_Msk                (0x1U << GPIO_ODR_OD8_Pos)             /*!< 0x00000100 */
#define GPIO_ODR_OD8                    GPIO_ODR_OD8_Msk                       
#define GPIO_ODR_OD9_Pos                (9U)                                   
#define GPIO_ODR_OD9_Msk                (0x1U << GPIO_ODR_OD9_Pos)             /*!< 0x00000200 */
#define GPIO_ODR_OD9                    GPIO_ODR_OD9_Msk                       
#define GPIO_ODR_OD10_Pos               (10U)                                  
#define GPIO_ODR_OD10_Msk               (0x1U << GPIO_ODR_OD10_Pos)            /*!< 0x00000400 */
#define GPIO_ODR_OD10                   GPIO_ODR_OD10_Msk                      
#define GPIO_ODR_OD11_Pos               (11U)                                  
#define GPIO_ODR_OD11_Msk               (0x1U << GPIO_ODR_OD11_Pos)            /*!< 0x00000800 */
#define GPIO_ODR_OD11                   GPIO_ODR_OD11_Msk                      
#define GPIO_ODR_OD12_Pos               (12U)                                  
#define GPIO_ODR_OD12_Msk               (0x1U << GPIO_ODR_OD12_Pos)            /*!< 0x00001000 */
#define GPIO_ODR_OD12                   GPIO_ODR_OD12_Msk                      
#define GPIO_ODR_OD13_Pos               (13U)                                  
#define GPIO_ODR_OD13_Msk               (0x1U << GPIO_ODR_OD13_Pos)            /*!< 0x00002000 */
#define GPIO_ODR_OD13                   GPIO_ODR_OD13_Msk                      
#define GPIO_ODR_OD14_Pos               (14U)                                  
#define GPIO_ODR_OD14_Msk               (0x1U << GPIO_ODR_OD14_Pos)            /*!< 0x00004000 */
#define GPIO_ODR_OD14                   GPIO_ODR_OD14_Msk                      
#define GPIO_ODR_OD15_Pos               (15U)                                  
#define GPIO_ODR_OD15_Msk               (0x1U << GPIO_ODR_OD15_Pos)            /*!< 0x00008000 */
#define GPIO_ODR_OD15                   GPIO_ODR_OD15_Msk                      

/****************** Bit definition for GPIO_BSRR register  ********************/
#define GPIO_BSRR_BS_0                  (0x00000001U)                          
#define GPIO_BSRR_BS_1                  (0x00000002U)                          
#define GPIO_BSRR_BS_2                  (0x00000004U)                          
#define GPIO_BSRR_BS_3                  (0x00000008U)                          
#define GPIO_BSRR_BS_4                  (0x00000010U)                          
#define GPIO_BSRR_BS_5                  (0x00000020U)                          
#define GPIO_BSRR_BS_6                  (0x00000040U)                          
#define GPIO_BSRR_BS_7                  (0x00000080U)                          
#define GPIO_BSRR_BS_8                  (0x00000100U)                          
#define GPIO_BSRR_BS_9                  (0x00000200U)                          
#define GPIO_BSRR_BS_10                 (0x00000400U)                          
#define GPIO_BSRR_BS_11                 (0x00000800U)                          
#define GPIO_BSRR_BS_12                 (0x00001000U)                          
#define GPIO_BSRR_BS_13                 (0x00002000U)                          
#define GPIO_BSRR_BS_14                 (0x00004000U)                          
#define GPIO_BSRR_BS_15                 (0x00008000U)                          
#define GPIO_BSRR_BR_0                  (0x00010000U)                          
#define GPIO_BSRR_BR_1                  (0x00020000U)                          
#define GPIO_BSRR_BR_2                  (0x00040000U)                          
#define GPIO_BSRR_BR_3                  (0x00080000U)                          
#define GPIO_BSRR_BR_4                  (0x00100000U)                          
#define GPIO_BSRR_BR_5                  (0x00200000U)                          
#define GPIO_BSRR_BR_6                  (0x00400000U)                          
#define GPIO_BSRR_BR_7                  (0x00800000U)                          
#define GPIO_BSRR_BR_8                  (0x01000000U)                          
#define GPIO_BSRR_BR_9                  (0x02000000U)                          
#define GPIO_BSRR_BR_10                 (0x04000000U)                          
#define GPIO_BSRR_BR_11                 (0x08000000U)                          
#define GPIO_BSRR_BR_12                 (0x10000000U)                          
#define GPIO_BSRR_BR_13                 (0x20000000U)                          
#define GPIO_BSRR_BR_14                 (0x40000000U)                          
#define GPIO_BSRR_BR_15                 (0x80000000U)                          

/****************** Bit definition for GPIO_LCKR register  ********************/
#define GPIO_LCKR_LCK0_Pos              (0U)                                   
#define GPIO_LCKR_LCK0_Msk              (0x1U << GPIO_LCKR_LCK0_Pos)           /*!< 0x00000001 */
#define GPIO_LCKR_LCK0                  GPIO_LCKR_LCK0_Msk                     
#define GPIO_LCKR_LCK1_Pos              (1U)                                   
#define GPIO_LCKR_LCK1_Msk              (0x1U << GPIO_LCKR_LCK1_Pos)           /*!< 0x00000002 */
#define GPIO_LCKR_LCK1                  GPIO_LCKR_LCK1_Msk                     
#define GPIO_LCKR_LCK2_Pos              (2U)                                   
#define GPIO_LCKR_LCK2_Msk              (0x1U << GPIO_LCKR_LCK2_Pos)           /*!< 0x00000004 */
#define GPIO_LCKR_LCK2                  GPIO_LCKR_LCK2_Msk                     
#define GPIO_LCKR_LCK3_Pos              (3U)                                   
#define GPIO_LCKR_LCK3_Msk              (0x1U << GPIO_LCKR_LCK3_Pos)           /*!< 0x00000008 */
#define GPIO_LCKR_LCK3                  GPIO_LCKR_LCK3_Msk                     
#define GPIO_LCKR_LCK4_Pos              (4U)                                   
#define GPIO_LCKR_LCK4_Msk              (0x1U << GPIO_LCKR_LCK4_Pos)           /*!< 0x00000010 */
#define GPIO_LCKR_LCK4                  GPIO_LCKR_LCK4_Msk                     
#define GPIO_LCKR_LCK5_Pos              (5U)                                   
#define GPIO_LCKR_LCK5_Msk              (0x1U << GPIO_LCKR_LCK5_Pos)           /*!< 0x00000020 */
#define GPIO_LCKR_LCK5                  GPIO_LCKR_LCK5_Msk                     
#define GPIO_LCKR_LCK6_Pos              (6U)                                   
#define GPIO_LCKR_LCK6_Msk              (0x1U << GPIO_LCKR_LCK6_Pos)           /*!< 0x00000040 */
#define GPIO_LCKR_LCK6                  GPIO_LCKR_LCK6_Msk                     
#define GPIO_LCKR_LCK7_Pos              (7U)                                   
#define GPIO_LCKR_LCK7_Msk              (0x1U << GPIO_LCKR_LCK7_Pos)           /*!< 0x00000080 */
#define GPIO_LCKR_LCK7                  GPIO_LCKR_LCK7_Msk                     
#define GPIO_LCKR_LCK8_Pos              (8U)                                   
#define GPIO_LCKR_LCK8_Msk              (0x1U << GPIO_LCKR_LCK8_Pos)           /*!< 0x00000100 */
#define GPIO_LCKR_LCK8                  GPIO_LCKR_LCK8_Msk                     
#define GPIO_LCKR_LCK9_Pos              (9U)                                   
#define GPIO_LCKR_LCK9_Msk              (0x1U << GPIO_LCKR_LCK9_Pos)           /*!< 0x00000200 */
#define GPIO_LCKR_LCK9                  GPIO_LCKR_LCK9_Msk                     
#define GPIO_LCKR_LCK10_Pos             (10U)                                  
#define GPIO_LCKR_LCK10_Msk             (0x1U << GPIO_LCKR_LCK10_Pos)          /*!< 0x00000400 */
#define GPIO_LCKR_LCK10                 GPIO_LCKR_LCK10_Msk                    
#define GPIO_LCKR_LCK11_Pos             (11U)                                  
#define GPIO_LCKR_LCK11_Msk             (0x1U << GPIO_LCKR_LCK11_Pos)          /*!< 0x00000800 */
#define GPIO_LCKR_LCK11                 GPIO_LCKR_LCK11_Msk                    
#define GPIO_LCKR_LCK12_Pos             (12U)                                  
#define GPIO_LCKR_LCK12_Msk             (0x1U << GPIO_LCKR_LCK12_Pos)          /*!< 0x00001000 */
#define GPIO_LCKR_LCK12                 GPIO_LCKR_LCK12_Msk                    
#define GPIO_LCKR_LCK13_Pos             (13U)                                  
#define GPIO_LCKR_LCK13_Msk             (0x1U << GPIO_LCKR_LCK13_Pos)          /*!< 0x00002000 */
#define GPIO_LCKR_LCK13                 GPIO_LCKR_LCK13_Msk                    
#define GPIO_LCKR_LCK14_Pos             (14U)                                  
#define GPIO_LCKR_LCK14_Msk             (0x1U << GPIO_LCKR_LCK14_Pos)          /*!< 0x00004000 */
#define GPIO_LCKR_LCK14                 GPIO_LCKR_LCK14_Msk                    
#define GPIO_LCKR_LCK15_Pos             (15U)                                  
#define GPIO_LCKR_LCK15_Msk             (0x1U << GPIO_LCKR_LCK15_Pos)          /*!< 0x00008000 */
#define GPIO_LCKR_LCK15                 GPIO_LCKR_LCK15_Msk                    
#define GPIO_LCKR_LCKK_Pos              (16U)                                  
#define GPIO_LCKR_LCKK_Msk              (0x1U << GPIO_LCKR_LCKK_Pos)           /*!< 0x00010000 */
#define GPIO_LCKR_LCKK                  GPIO_LCKR_LCKK_Msk                     

/****************** Bit definition for GPIO_AFRL register ********************/
#define GPIO_AFRL_AFRL0_Pos             (0U)                                   
#define GPIO_AFRL_AFRL0_Msk             (0xFU << GPIO_AFRL_AFRL0_Pos)          /*!< 0x0000000F */
#define GPIO_AFRL_AFRL0                 GPIO_AFRL_AFRL0_Msk                    
#define GPIO_AFRL_AFRL1_Pos             (4U)                                   
#define GPIO_AFRL_AFRL1_Msk             (0xFU << GPIO_AFRL_AFRL1_Pos)          /*!< 0x000000F0 */
#define GPIO_AFRL_AFRL1                 GPIO_AFRL_AFRL1_Msk                    
#define GPIO_AFRL_AFRL2_Pos             (8U)                                   
#define GPIO_AFRL_AFRL2_Msk             (0xFU << GPIO_AFRL_AFRL2_Pos)          /*!< 0x00000F00 */
#define GPIO_AFRL_AFRL2                 GPIO_AFRL_AFRL2_Msk                    
#define GPIO_AFRL_AFRL3_Pos             (12U)                                  
#define GPIO_AFRL_AFRL3_Msk             (0xFU << GPIO_AFRL_AFRL3_Pos)          /*!< 0x0000F000 */
#define GPIO_AFRL_AFRL3                 GPIO_AFRL_AFRL3_Msk                    
#define GPIO_AFRL_AFRL4_Pos             (16U)                                  
#define GPIO_AFRL_AFRL4_Msk             (0xFU << GPIO_AFRL_AFRL4_Pos)          /*!< 0x000F0000 */
#define GPIO_AFRL_AFRL4                 GPIO_AFRL_AFRL4_Msk                    
#define GPIO_AFRL_AFRL5_Pos             (20U)                                  
#define GPIO_AFRL_AFRL5_Msk             (0xFU << GPIO_AFRL_AFRL5_Pos)          /*!< 0x00F00000 */
#define GPIO_AFRL_AFRL5                 GPIO_AFRL_AFRL5_Msk                    
#define GPIO_AFRL_AFRL6_Pos             (24U)                                  
#define GPIO_AFRL_AFRL6_Msk             (0xFU << GPIO_AFRL_AFRL6_Pos)          /*!< 0x0F000000 */
#define GPIO_AFRL_AFRL6                 GPIO_AFRL_AFRL6_Msk                    
#define GPIO_AFRL_AFRL7_Pos             (28U)                                  
#define GPIO_AFRL_AFRL7_Msk             (0xFU << GPIO_AFRL_AFRL7_Pos)          /*!< 0xF0000000 */
#define GPIO_AFRL_AFRL7                 GPIO_AFRL_AFRL7_Msk                    

/****************** Bit definition for GPIO_AFRH register ********************/
#define GPIO_AFRH_AFRH0_Pos             (0U)                                   
#define GPIO_AFRH_AFRH0_Msk             (0xFU << GPIO_AFRH_AFRH0_Pos)          /*!< 0x0000000F */
#define GPIO_AFRH_AFRH0                 GPIO_AFRH_AFRH0_Msk                    
#define GPIO_AFRH_AFRH1_Pos             (4U)                                   
#define GPIO_AFRH_AFRH1_Msk             (0xFU << GPIO_AFRH_AFRH1_Pos)          /*!< 0x000000F0 */
#define GPIO_AFRH_AFRH1                 GPIO_AFRH_AFRH1_Msk                    
#define GPIO_AFRH_AFRH2_Pos             (8U)                                   
#define GPIO_AFRH_AFRH2_Msk             (0xFU << GPIO_AFRH_AFRH2_Pos)          /*!< 0x00000F00 */
#define GPIO_AFRH_AFRH2                 GPIO_AFRH_AFRH2_Msk                    
#define GPIO_AFRH_AFRH3_Pos             (12U)                                  
#define GPIO_AFRH_AFRH3_Msk             (0xFU << GPIO_AFRH_AFRH3_Pos)          /*!< 0x0000F000 */
#define GPIO_AFRH_AFRH3                 GPIO_AFRH_AFRH3_Msk                    
#define GPIO_AFRH_AFRH4_Pos             (16U)                                  
#define GPIO_AFRH_AFRH4_Msk             (0xFU << GPIO_AFRH_AFRH4_Pos)          /*!< 0x000F0000 */
#define GPIO_AFRH_AFRH4                 GPIO_AFRH_AFRH4_Msk                    
#define GPIO_AFRH_AFRH5_Pos             (20U)                                  
#define GPIO_AFRH_AFRH5_Msk             (0xFU << GPIO_AFRH_AFRH5_Pos)          /*!< 0x00F00000 */
#define GPIO_AFRH_AFRH5                 GPIO_AFRH_AFRH5_Msk                    
#define GPIO_AFRH_AFRH6_Pos             (24U)                                  
#define GPIO_AFRH_AFRH6_Msk             (0xFU << GPIO_AFRH_AFRH6_Pos)          /*!< 0x0F000000 */
#define GPIO_AFRH_AFRH6                 GPIO_AFRH_AFRH6_Msk                    
#define GPIO_AFRH_AFRH7_Pos             (28U)                                  
#define GPIO_AFRH_AFRH7_Msk             (0xFU << GPIO_AFRH_AFRH7_Pos)          /*!< 0xF0000000 */
#define GPIO_AFRH_AFRH7                 GPIO_AFRH_AFRH7_Msk                    

/****************** Bit definition for GPIO_BRR register  *********************/
#define GPIO_BRR_BR_0                   (0x00000001U)                          
#define GPIO_BRR_BR_1                   (0x00000002U)                          
#define GPIO_BRR_BR_2                   (0x00000004U)                          
#define GPIO_BRR_BR_3                   (0x00000008U)                          
#define GPIO_BRR_BR_4                   (0x00000010U)                          
#define GPIO_BRR_BR_5                   (0x00000020U)                          
#define GPIO_BRR_BR_6                   (0x00000040U)                          
#define GPIO_BRR_BR_7                   (0x00000080U)                          
#define GPIO_BRR_BR_8                   (0x00000100U)                          
#define GPIO_BRR_BR_9                   (0x00000200U)                          
#define GPIO_BRR_BR_10                  (0x00000400U)                          
#define GPIO_BRR_BR_11                  (0x00000800U)                          
#define GPIO_BRR_BR_12                  (0x00001000U)                          
#define GPIO_BRR_BR_13                  (0x00002000U)                          
#define GPIO_BRR_BR_14                  (0x00004000U)                          
#define GPIO_BRR_BR_15                  (0x00008000U)                          

/******************************************************************************/
/*                                                                            */
/*                   Inter-integrated Circuit Interface (I2C)                 */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for I2C_CR1 register  *******************/
#define I2C_CR1_PE_Pos               (0U)                                      
#define I2C_CR1_PE_Msk               (0x1U << I2C_CR1_PE_Pos)                  /*!< 0x00000001 */
#define I2C_CR1_PE                   I2C_CR1_PE_Msk                            /*!< Peripheral enable */
#define I2C_CR1_TXIE_Pos             (1U)                                      
#define I2C_CR1_TXIE_Msk             (0x1U << I2C_CR1_TXIE_Pos)                /*!< 0x00000002 */
#define I2C_CR1_TXIE                 I2C_CR1_TXIE_Msk                          /*!< TX interrupt enable */
#define I2C_CR1_RXIE_Pos             (2U)                                      
#define I2C_CR1_RXIE_Msk             (0x1U << I2C_CR1_RXIE_Pos)                /*!< 0x00000004 */
#define I2C_CR1_RXIE                 I2C_CR1_RXIE_Msk                          /*!< RX interrupt enable */
#define I2C_CR1_ADDRIE_Pos           (3U)                                      
#define I2C_CR1_ADDRIE_Msk           (0x1U << I2C_CR1_ADDRIE_Pos)              /*!< 0x00000008 */
#define I2C_CR1_ADDRIE               I2C_CR1_ADDRIE_Msk                        /*!< Address match interrupt enable */
#define I2C_CR1_NACKIE_Pos           (4U)                                      
#define I2C_CR1_NACKIE_Msk           (0x1U << I2C_CR1_NACKIE_Pos)              /*!< 0x00000010 */
#define I2C_CR1_NACKIE               I2C_CR1_NACKIE_Msk                        /*!< NACK received interrupt enable */
#define I2C_CR1_STOPIE_Pos           (5U)                                      
#define I2C_CR1_STOPIE_Msk           (0x1U << I2C_CR1_STOPIE_Pos)              /*!< 0x00000020 */
#define I2C_CR1_STOPIE               I2C_CR1_STOPIE_Msk                        /*!< STOP detection interrupt enable */
#define I2C_CR1_TCIE_Pos             (6U)                                      
#define I2C_CR1_TCIE_Msk             (0x1U << I2C_CR1_TCIE_Pos)                /*!< 0x00000040 */
#define I2C_CR1_TCIE                 I2C_CR1_TCIE_Msk                          /*!< Transfer complete interrupt enable */
#define I2C_CR1_ERRIE_Pos            (7U)                                      
#define I2C_CR1_ERRIE_Msk            (0x1U << I2C_CR1_ERRIE_Pos)               /*!< 0x00000080 */
#define I2C_CR1_ERRIE                I2C_CR1_ERRIE_Msk                         /*!< Errors interrupt enable */
#define I2C_CR1_DNF_Pos              (8U)                                      
#define I2C_CR1_DNF_Msk              (0xFU << I2C_CR1_DNF_Pos)                 /*!< 0x00000F00 */
#define I2C_CR1_DNF                  I2C_CR1_DNF_Msk                           /*!< Digital noise filter */
#define I2C_CR1_ANFOFF_Pos           (12U)                                     
#define I2C_CR1_ANFOFF_Msk           (0x1U << I2C_CR1_ANFOFF_Pos)              /*!< 0x00001000 */
#define I2C_CR1_ANFOFF               I2C_CR1_ANFOFF_Msk                        /*!< Analog noise filter OFF */
#define I2C_CR1_TXDMAEN_Pos          (14U)                                     
#define I2C_CR1_TXDMAEN_Msk          (0x1U << I2C_CR1_TXDMAEN_Pos)             /*!< 0x00004000 */
#define I2C_CR1_TXDMAEN              I2C_CR1_TXDMAEN_Msk                       /*!< DMA transmission requests enable */
#define I2C_CR1_RXDMAEN_Pos          (15U)                                     
#define I2C_CR1_RXDMAEN_Msk          (0x1U << I2C_CR1_RXDMAEN_Pos)             /*!< 0x00008000 */
#define I2C_CR1_RXDMAEN              I2C_CR1_RXDMAEN_Msk                       /*!< DMA reception requests enable */
#define I2C_CR1_SBC_Pos              (16U)                                     
#define I2C_CR1_SBC_Msk              (0x1U << I2C_CR1_SBC_Pos)                 /*!< 0x00010000 */
#define I2C_CR1_SBC                  I2C_CR1_SBC_Msk                           /*!< Slave byte control */
#define I2C_CR1_NOSTRETCH_Pos        (17U)                                     
#define I2C_CR1_NOSTRETCH_Msk        (0x1U << I2C_CR1_NOSTRETCH_Pos)           /*!< 0x00020000 */
#define I2C_CR1_NOSTRETCH            I2C_CR1_NOSTRETCH_Msk                     /*!< Clock stretching disable */
#define I2C_CR1_WUPEN_Pos            (18U)                                     
#define I2C_CR1_WUPEN_Msk            (0x1U << I2C_CR1_WUPEN_Pos)               /*!< 0x00040000 */
#define I2C_CR1_WUPEN                I2C_CR1_WUPEN_Msk                         /*!< Wakeup from STOP enable */
#define I2C_CR1_GCEN_Pos             (19U)                                     
#define I2C_CR1_GCEN_Msk             (0x1U << I2C_CR1_GCEN_Pos)                /*!< 0x00080000 */
#define I2C_CR1_GCEN                 I2C_CR1_GCEN_Msk                          /*!< General call enable */
#define I2C_CR1_SMBHEN_Pos           (20U)                                     
#define I2C_CR1_SMBHEN_Msk           (0x1U << I2C_CR1_SMBHEN_Pos)              /*!< 0x00100000 */
#define I2C_CR1_SMBHEN               I2C_CR1_SMBHEN_Msk                        /*!< SMBus host address enable */
#define I2C_CR1_SMBDEN_Pos           (21U)                                     
#define I2C_CR1_SMBDEN_Msk           (0x1U << I2C_CR1_SMBDEN_Pos)              /*!< 0x00200000 */
#define I2C_CR1_SMBDEN               I2C_CR1_SMBDEN_Msk                        /*!< SMBus device default address enable */
#define I2C_CR1_ALERTEN_Pos          (22U)                                     
#define I2C_CR1_ALERTEN_Msk          (0x1U << I2C_CR1_ALERTEN_Pos)             /*!< 0x00400000 */
#define I2C_CR1_ALERTEN              I2C_CR1_ALERTEN_Msk                       /*!< SMBus alert enable */
#define I2C_CR1_PECEN_Pos            (23U)                                     
#define I2C_CR1_PECEN_Msk            (0x1U << I2C_CR1_PECEN_Pos)               /*!< 0x00800000 */
#define I2C_CR1_PECEN                I2C_CR1_PECEN_Msk                         /*!< PEC enable */

/******************  Bit definition for I2C_CR2 register  ********************/
#define I2C_CR2_SADD_Pos             (0U)                                      
#define I2C_CR2_SADD_Msk             (0x3FFU << I2C_CR2_SADD_Pos)              /*!< 0x000003FF */
#define I2C_CR2_SADD                 I2C_CR2_SADD_Msk                          /*!< Slave address (master mode) */
#define I2C_CR2_RD_WRN_Pos           (10U)                                     
#define I2C_CR2_RD_WRN_Msk           (0x1U << I2C_CR2_RD_WRN_Pos)              /*!< 0x00000400 */
#define I2C_CR2_RD_WRN               I2C_CR2_RD_WRN_Msk                        /*!< Transfer direction (master mode) */
#define I2C_CR2_ADD10_Pos            (11U)                                     
#define I2C_CR2_ADD10_Msk            (0x1U << I2C_CR2_ADD10_Pos)               /*!< 0x00000800 */
#define I2C_CR2_ADD10                I2C_CR2_ADD10_Msk                         /*!< 10-bit addressing mode (master mode) */
#define I2C_CR2_HEAD10R_Pos          (12U)                                     
#define I2C_CR2_HEAD10R_Msk          (0x1U << I2C_CR2_HEAD10R_Pos)             /*!< 0x00001000 */
#define I2C_CR2_HEAD10R              I2C_CR2_HEAD10R_Msk                       /*!< 10-bit address header only read direction (master mode) */
#define I2C_CR2_START_Pos            (13U)                                     
#define I2C_CR2_START_Msk            (0x1U << I2C_CR2_START_Pos)               /*!< 0x00002000 */
#define I2C_CR2_START                I2C_CR2_START_Msk                         /*!< START generation */
#define I2C_CR2_STOP_Pos             (14U)                                     
#define I2C_CR2_STOP_Msk             (0x1U << I2C_CR2_STOP_Pos)                /*!< 0x00004000 */
#define I2C_CR2_STOP                 I2C_CR2_STOP_Msk                          /*!< STOP generation (master mode) */
#define I2C_CR2_NACK_Pos             (15U)                                     
#define I2C_CR2_NACK_Msk             (0x1U << I2C_CR2_NACK_Pos)                /*!< 0x00008000 */
#define I2C_CR2_NACK                 I2C_CR2_NACK_Msk                          /*!< NACK generation (slave mode) */
#define I2C_CR2_NBYTES_Pos           (16U)                                     
#define I2C_CR2_NBYTES_Msk           (0xFFU << I2C_CR2_NBYTES_Pos)             /*!< 0x00FF0000 */
#define I2C_CR2_NBYTES               I2C_CR2_NBYTES_Msk                        /*!< Number of bytes */
#define I2C_CR2_RELOAD_Pos           (24U)                                     
#define I2C_CR2_RELOAD_Msk           (0x1U << I2C_CR2_RELOAD_Pos)              /*!< 0x01000000 */
#define I2C_CR2_RELOAD               I2C_CR2_RELOAD_Msk                        /*!< NBYTES reload mode */
#define I2C_CR2_AUTOEND_Pos          (25U)                                     
#define I2C_CR2_AUTOEND_Msk          (0x1U << I2C_CR2_AUTOEND_Pos)             /*!< 0x02000000 */
#define I2C_CR2_AUTOEND              I2C_CR2_AUTOEND_Msk                       /*!< Automatic end mode (master mode) */
#define I2C_CR2_PECBYTE_Pos          (26U)                                     
#define I2C_CR2_PECBYTE_Msk          (0x1U << I2C_CR2_PECBYTE_Pos)             /*!< 0x04000000 */
#define I2C_CR2_PECBYTE              I2C_CR2_PECBYTE_Msk                       /*!< Packet error checking byte */

/*******************  Bit definition for I2C_OAR1 register  ******************/
#define I2C_OAR1_OA1_Pos             (0U)                                      
#define I2C_OAR1_OA1_Msk             (0x3FFU << I2C_OAR1_OA1_Pos)              /*!< 0x000003FF */
#define I2C_OAR1_OA1                 I2C_OAR1_OA1_Msk                          /*!< Interface own address 1 */
#define I2C_OAR1_OA1MODE_Pos         (10U)                                     
#define I2C_OAR1_OA1MODE_Msk         (0x1U << I2C_OAR1_OA1MODE_Pos)            /*!< 0x00000400 */
#define I2C_OAR1_OA1MODE             I2C_OAR1_OA1MODE_Msk                      /*!< Own address 1 10-bit mode */
#define I2C_OAR1_OA1EN_Pos           (15U)                                     
#define I2C_OAR1_OA1EN_Msk           (0x1U << I2C_OAR1_OA1EN_Pos)              /*!< 0x00008000 */
#define I2C_OAR1_OA1EN               I2C_OAR1_OA1EN_Msk                        /*!< Own address 1 enable */

/*******************  Bit definition for I2C_OAR2 register  ******************/
#define I2C_OAR2_OA2_Pos             (1U)                                      
#define I2C_OAR2_OA2_Msk             (0x7FU << I2C_OAR2_OA2_Pos)               /*!< 0x000000FE */
#define I2C_OAR2_OA2                 I2C_OAR2_OA2_Msk                          /*!< Interface own address 2                        */
#define I2C_OAR2_OA2MSK_Pos          (8U)                                      
#define I2C_OAR2_OA2MSK_Msk          (0x7U << I2C_OAR2_OA2MSK_Pos)             /*!< 0x00000700 */
#define I2C_OAR2_OA2MSK              I2C_OAR2_OA2MSK_Msk                       /*!< Own address 2 masks                            */
#define I2C_OAR2_OA2NOMASK           (0x00000000U)                             /*!< No mask                                        */
#define I2C_OAR2_OA2MASK01_Pos       (8U)                                      
#define I2C_OAR2_OA2MASK01_Msk       (0x1U << I2C_OAR2_OA2MASK01_Pos)          /*!< 0x00000100 */
#define I2C_OAR2_OA2MASK01           I2C_OAR2_OA2MASK01_Msk                    /*!< OA2[1] is masked, Only OA2[7:2] are compared   */
#define I2C_OAR2_OA2MASK02_Pos       (9U)                                      
#define I2C_OAR2_OA2MASK02_Msk       (0x1U << I2C_OAR2_OA2MASK02_Pos)          /*!< 0x00000200 */
#define I2C_OAR2_OA2MASK02           I2C_OAR2_OA2MASK02_Msk                    /*!< OA2[2:1] is masked, Only OA2[7:3] are compared */
#define I2C_OAR2_OA2MASK03_Pos       (8U)                                      
#define I2C_OAR2_OA2MASK03_Msk       (0x3U << I2C_OAR2_OA2MASK03_Pos)          /*!< 0x00000300 */
#define I2C_OAR2_OA2MASK03           I2C_OAR2_OA2MASK03_Msk                    /*!< OA2[3:1] is masked, Only OA2[7:4] are compared */
#define I2C_OAR2_OA2MASK04_Pos       (10U)                                     
#define I2C_OAR2_OA2MASK04_Msk       (0x1U << I2C_OAR2_OA2MASK04_Pos)          /*!< 0x00000400 */
#define I2C_OAR2_OA2MASK04           I2C_OAR2_OA2MASK04_Msk                    /*!< OA2[4:1] is masked, Only OA2[7:5] are compared */
#define I2C_OAR2_OA2MASK05_Pos       (8U)                                      
#define I2C_OAR2_OA2MASK05_Msk       (0x5U << I2C_OAR2_OA2MASK05_Pos)          /*!< 0x00000500 */
#define I2C_OAR2_OA2MASK05           I2C_OAR2_OA2MASK05_Msk                    /*!< OA2[5:1] is masked, Only OA2[7:6] are compared */
#define I2C_OAR2_OA2MASK06_Pos       (9U)                                      
#define I2C_OAR2_OA2MASK06_Msk       (0x3U << I2C_OAR2_OA2MASK06_Pos)          /*!< 0x00000600 */
#define I2C_OAR2_OA2MASK06           I2C_OAR2_OA2MASK06_Msk                    /*!< OA2[6:1] is masked, Only OA2[7] are compared   */
#define I2C_OAR2_OA2MASK07_Pos       (8U)                                      
#define I2C_OAR2_OA2MASK07_Msk       (0x7U << I2C_OAR2_OA2MASK07_Pos)          /*!< 0x00000700 */
#define I2C_OAR2_OA2MASK07           I2C_OAR2_OA2MASK07_Msk                    /*!< OA2[7:1] is masked, No comparison is done      */
#define I2C_OAR2_OA2EN_Pos           (15U)                                     
#define I2C_OAR2_OA2EN_Msk           (0x1U << I2C_OAR2_OA2EN_Pos)              /*!< 0x00008000 */
#define I2C_OAR2_OA2EN               I2C_OAR2_OA2EN_Msk                        /*!< Own address 2 enable                           */

/*******************  Bit definition for I2C_TIMINGR register *******************/
#define I2C_TIMINGR_SCLL_Pos         (0U)                                      
#define I2C_TIMINGR_SCLL_Msk         (0xFFU << I2C_TIMINGR_SCLL_Pos)           /*!< 0x000000FF */
#define I2C_TIMINGR_SCLL             I2C_TIMINGR_SCLL_Msk                      /*!< SCL low period (master mode) */
#define I2C_TIMINGR_SCLH_Pos         (8U)                                      
#define I2C_TIMINGR_SCLH_Msk         (0xFFU << I2C_TIMINGR_SCLH_Pos)           /*!< 0x0000FF00 */
#define I2C_TIMINGR_SCLH             I2C_TIMINGR_SCLH_Msk                      /*!< SCL high period (master mode) */
#define I2C_TIMINGR_SDADEL_Pos       (16U)                                     
#define I2C_TIMINGR_SDADEL_Msk       (0xFU << I2C_TIMINGR_SDADEL_Pos)          /*!< 0x000F0000 */
#define I2C_TIMINGR_SDADEL           I2C_TIMINGR_SDADEL_Msk                    /*!< Data hold time */
#define I2C_TIMINGR_SCLDEL_Pos       (20U)                                     
#define I2C_TIMINGR_SCLDEL_Msk       (0xFU << I2C_TIMINGR_SCLDEL_Pos)          /*!< 0x00F00000 */
#define I2C_TIMINGR_SCLDEL           I2C_TIMINGR_SCLDEL_Msk                    /*!< Data setup time */
#define I2C_TIMINGR_PRESC_Pos        (28U)                                     
#define I2C_TIMINGR_PRESC_Msk        (0xFU << I2C_TIMINGR_PRESC_Pos)           /*!< 0xF0000000 */
#define I2C_TIMINGR_PRESC            I2C_TIMINGR_PRESC_Msk                     /*!< Timings prescaler */

/******************* Bit definition for I2C_TIMEOUTR register *******************/
#define I2C_TIMEOUTR_TIMEOUTA_Pos    (0U)                                      
#define I2C_TIMEOUTR_TIMEOUTA_Msk    (0xFFFU << I2C_TIMEOUTR_TIMEOUTA_Pos)     /*!< 0x00000FFF */
#define I2C_TIMEOUTR_TIMEOUTA        I2C_TIMEOUTR_TIMEOUTA_Msk                 /*!< Bus timeout A */
#define I2C_TIMEOUTR_TIDLE_Pos       (12U)                                     
#define I2C_TIMEOUTR_TIDLE_Msk       (0x1U << I2C_TIMEOUTR_TIDLE_Pos)          /*!< 0x00001000 */
#define I2C_TIMEOUTR_TIDLE           I2C_TIMEOUTR_TIDLE_Msk                    /*!< Idle clock timeout detection */
#define I2C_TIMEOUTR_TIMOUTEN_Pos    (15U)                                     
#define I2C_TIMEOUTR_TIMOUTEN_Msk    (0x1U << I2C_TIMEOUTR_TIMOUTEN_Pos)       /*!< 0x00008000 */
#define I2C_TIMEOUTR_TIMOUTEN        I2C_TIMEOUTR_TIMOUTEN_Msk                 /*!< Clock timeout enable */
#define I2C_TIMEOUTR_TIMEOUTB_Pos    (16U)                                     
#define I2C_TIMEOUTR_TIMEOUTB_Msk    (0xFFFU << I2C_TIMEOUTR_TIMEOUTB_Pos)     /*!< 0x0FFF0000 */
#define I2C_TIMEOUTR_TIMEOUTB        I2C_TIMEOUTR_TIMEOUTB_Msk                 /*!< Bus timeout B*/
#define I2C_TIMEOUTR_TEXTEN_Pos      (31U)                                     
#define I2C_TIMEOUTR_TEXTEN_Msk      (0x1U << I2C_TIMEOUTR_TEXTEN_Pos)         /*!< 0x80000000 */
#define I2C_TIMEOUTR_TEXTEN          I2C_TIMEOUTR_TEXTEN_Msk                   /*!< Extended clock timeout enable */

/******************  Bit definition for I2C_ISR register  *********************/
#define I2C_ISR_TXE_Pos              (0U)                                      
#define I2C_ISR_TXE_Msk              (0x1U << I2C_ISR_TXE_Pos)                 /*!< 0x00000001 */
#define I2C_ISR_TXE                  I2C_ISR_TXE_Msk                           /*!< Transmit data register empty */
#define I2C_ISR_TXIS_Pos             (1U)                                      
#define I2C_ISR_TXIS_Msk             (0x1U << I2C_ISR_TXIS_Pos)                /*!< 0x00000002 */
#define I2C_ISR_TXIS                 I2C_ISR_TXIS_Msk                          /*!< Transmit interrupt status */
#define I2C_ISR_RXNE_Pos             (2U)                                      
#define I2C_ISR_RXNE_Msk             (0x1U << I2C_ISR_RXNE_Pos)                /*!< 0x00000004 */
#define I2C_ISR_RXNE                 I2C_ISR_RXNE_Msk                          /*!< Receive data register not empty */
#define I2C_ISR_ADDR_Pos             (3U)                                      
#define I2C_ISR_ADDR_Msk             (0x1U << I2C_ISR_ADDR_Pos)                /*!< 0x00000008 */
#define I2C_ISR_ADDR                 I2C_ISR_ADDR_Msk                          /*!< Address matched (slave mode)*/
#define I2C_ISR_NACKF_Pos            (4U)                                      
#define I2C_ISR_NACKF_Msk            (0x1U << I2C_ISR_NACKF_Pos)               /*!< 0x00000010 */
#define I2C_ISR_NACKF                I2C_ISR_NACKF_Msk                         /*!< NACK received flag */
#define I2C_ISR_STOPF_Pos            (5U)                                      
#define I2C_ISR_STOPF_Msk            (0x1U << I2C_ISR_STOPF_Pos)               /*!< 0x00000020 */
#define I2C_ISR_STOPF                I2C_ISR_STOPF_Msk                         /*!< STOP detection flag */
#define I2C_ISR_TC_Pos               (6U)                                      
#define I2C_ISR_TC_Msk               (0x1U << I2C_ISR_TC_Pos)                  /*!< 0x00000040 */
#define I2C_ISR_TC                   I2C_ISR_TC_Msk                            /*!< Transfer complete (master mode) */
#define I2C_ISR_TCR_Pos              (7U)                                      
#define I2C_ISR_TCR_Msk              (0x1U << I2C_ISR_TCR_Pos)                 /*!< 0x00000080 */
#define I2C_ISR_TCR                  I2C_ISR_TCR_Msk                           /*!< Transfer complete reload */
#define I2C_ISR_BERR_Pos             (8U)                                      
#define I2C_ISR_BERR_Msk             (0x1U << I2C_ISR_BERR_Pos)                /*!< 0x00000100 */
#define I2C_ISR_BERR                 I2C_ISR_BERR_Msk                          /*!< Bus error */
#define I2C_ISR_ARLO_Pos             (9U)                                      
#define I2C_ISR_ARLO_Msk             (0x1U << I2C_ISR_ARLO_Pos)                /*!< 0x00000200 */
#define I2C_ISR_ARLO                 I2C_ISR_ARLO_Msk                          /*!< Arbitration lost */
#define I2C_ISR_OVR_Pos              (10U)                                     
#define I2C_ISR_OVR_Msk              (0x1U << I2C_ISR_OVR_Pos)                 /*!< 0x00000400 */
#define I2C_ISR_OVR                  I2C_ISR_OVR_Msk                           /*!< Overrun/Underrun */
#define I2C_ISR_PECERR_Pos           (11U)                                     
#define I2C_ISR_PECERR_Msk           (0x1U << I2C_ISR_PECERR_Pos)              /*!< 0x00000800 */
#define I2C_ISR_PECERR               I2C_ISR_PECERR_Msk                        /*!< PEC error in reception */
#define I2C_ISR_TIMEOUT_Pos          (12U)                                     
#define I2C_ISR_TIMEOUT_Msk          (0x1U << I2C_ISR_TIMEOUT_Pos)             /*!< 0x00001000 */
#define I2C_ISR_TIMEOUT              I2C_ISR_TIMEOUT_Msk                       /*!< Timeout or Tlow detection flag */
#define I2C_ISR_ALERT_Pos            (13U)                                     
#define I2C_ISR_ALERT_Msk            (0x1U << I2C_ISR_ALERT_Pos)               /*!< 0x00002000 */
#define I2C_ISR_ALERT                I2C_ISR_ALERT_Msk                         /*!< SMBus alert */
#define I2C_ISR_BUSY_Pos             (15U)                                     
#define I2C_ISR_BUSY_Msk             (0x1U << I2C_ISR_BUSY_Pos)                /*!< 0x00008000 */
#define I2C_ISR_BUSY                 I2C_ISR_BUSY_Msk                          /*!< Bus busy */
#define I2C_ISR_DIR_Pos              (16U)                                     
#define I2C_ISR_DIR_Msk              (0x1U << I2C_ISR_DIR_Pos)                 /*!< 0x00010000 */
#define I2C_ISR_DIR                  I2C_ISR_DIR_Msk                           /*!< Transfer direction (slave mode) */
#define I2C_ISR_ADDCODE_Pos          (17U)                                     
#define I2C_ISR_ADDCODE_Msk          (0x7FU << I2C_ISR_ADDCODE_Pos)            /*!< 0x00FE0000 */
#define I2C_ISR_ADDCODE              I2C_ISR_ADDCODE_Msk                       /*!< Address match code (slave mode) */

/******************  Bit definition for I2C_ICR register  *********************/
#define I2C_ICR_ADDRCF_Pos           (3U)                                      
#define I2C_ICR_ADDRCF_Msk           (0x1U << I2C_ICR_ADDRCF_Pos)              /*!< 0x00000008 */
#define I2C_ICR_ADDRCF               I2C_ICR_ADDRCF_Msk                        /*!< Address matched clear flag */
#define I2C_ICR_NACKCF_Pos           (4U)                                      
#define I2C_ICR_NACKCF_Msk           (0x1U << I2C_ICR_NACKCF_Pos)              /*!< 0x00000010 */
#define I2C_ICR_NACKCF               I2C_ICR_NACKCF_Msk                        /*!< NACK clear flag */
#define I2C_ICR_STOPCF_Pos           (5U)                                      
#define I2C_ICR_STOPCF_Msk           (0x1U << I2C_ICR_STOPCF_Pos)              /*!< 0x00000020 */
#define I2C_ICR_STOPCF               I2C_ICR_STOPCF_Msk                        /*!< STOP detection clear flag */
#define I2C_ICR_BERRCF_Pos           (8U)                                      
#define I2C_ICR_BERRCF_Msk           (0x1U << I2C_ICR_BERRCF_Pos)              /*!< 0x00000100 */
#define I2C_ICR_BERRCF               I2C_ICR_BERRCF_Msk                        /*!< Bus error clear flag */
#define I2C_ICR_ARLOCF_Pos           (9U)                                      
#define I2C_ICR_ARLOCF_Msk           (0x1U << I2C_ICR_ARLOCF_Pos)              /*!< 0x00000200 */
#define I2C_ICR_ARLOCF               I2C_ICR_ARLOCF_Msk                        /*!< Arbitration lost clear flag */
#define I2C_ICR_OVRCF_Pos            (10U)                                     
#define I2C_ICR_OVRCF_Msk            (0x1U << I2C_ICR_OVRCF_Pos)               /*!< 0x00000400 */
#define I2C_ICR_OVRCF                I2C_ICR_OVRCF_Msk                         /*!< Overrun/Underrun clear flag */
#define I2C_ICR_PECCF_Pos            (11U)                                     
#define I2C_ICR_PECCF_Msk            (0x1U << I2C_ICR_PECCF_Pos)               /*!< 0x00000800 */
#define I2C_ICR_PECCF                I2C_ICR_PECCF_Msk                         /*!< PAC error clear flag */
#define I2C_ICR_TIMOUTCF_Pos         (12U)                                     
#define I2C_ICR_TIMOUTCF_Msk         (0x1U << I2C_ICR_TIMOUTCF_Pos)            /*!< 0x00001000 */
#define I2C_ICR_TIMOUTCF             I2C_ICR_TIMOUTCF_Msk                      /*!< Timeout clear flag */
#define I2C_ICR_ALERTCF_Pos          (13U)                                     
#define I2C_ICR_ALERTCF_Msk          (0x1U << I2C_ICR_ALERTCF_Pos)             /*!< 0x00002000 */
#define I2C_ICR_ALERTCF              I2C_ICR_ALERTCF_Msk                       /*!< Alert clear flag */

/******************  Bit definition for I2C_PECR register  *********************/
#define I2C_PECR_PEC_Pos             (0U)                                      
#define I2C_PECR_PEC_Msk             (0xFFU << I2C_PECR_PEC_Pos)               /*!< 0x000000FF */
#define I2C_PECR_PEC                 I2C_PECR_PEC_Msk                          /*!< PEC register */

/******************  Bit definition for I2C_RXDR register  *********************/
#define I2C_RXDR_RXDATA_Pos          (0U)                                      
#define I2C_RXDR_RXDATA_Msk          (0xFFU << I2C_RXDR_RXDATA_Pos)            /*!< 0x000000FF */
#define I2C_RXDR_RXDATA              I2C_RXDR_RXDATA_Msk                       /*!< 8-bit receive data */

/******************  Bit definition for I2C_TXDR register  *********************/
#define I2C_TXDR_TXDATA_Pos          (0U)                                      
#define I2C_TXDR_TXDATA_Msk          (0xFFU << I2C_TXDR_TXDATA_Pos)            /*!< 0x000000FF */
#define I2C_TXDR_TXDATA              I2C_TXDR_TXDATA_Msk                       /*!< 8-bit transmit data */

/******************************************************************************/
/*                                                                            */
/*                        Independent WATCHDOG (IWDG)                         */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_KR_KEY_Pos      (0U)                                              
#define IWDG_KR_KEY_Msk      (0xFFFFU << IWDG_KR_KEY_Pos)                      /*!< 0x0000FFFF */
#define IWDG_KR_KEY          IWDG_KR_KEY_Msk                                   /*!< Key value (write only, read 0000h) */

/*******************  Bit definition for IWDG_PR register  ********************/
#define IWDG_PR_PR_Pos       (0U)                                              
#define IWDG_PR_PR_Msk       (0x7U << IWDG_PR_PR_Pos)                          /*!< 0x00000007 */
#define IWDG_PR_PR           IWDG_PR_PR_Msk                                    /*!< PR[2:0] (Prescaler divider) */
#define IWDG_PR_PR_0         (0x1U << IWDG_PR_PR_Pos)                          /*!< 0x00000001 */
#define IWDG_PR_PR_1         (0x2U << IWDG_PR_PR_Pos)                          /*!< 0x00000002 */
#define IWDG_PR_PR_2         (0x4U << IWDG_PR_PR_Pos)                          /*!< 0x00000004 */

/*******************  Bit definition for IWDG_RLR register  *******************/
#define IWDG_RLR_RL_Pos      (0U)                                              
#define IWDG_RLR_RL_Msk      (0xFFFU << IWDG_RLR_RL_Pos)                       /*!< 0x00000FFF */
#define IWDG_RLR_RL          IWDG_RLR_RL_Msk                                   /*!< Watchdog counter reload value */

/*******************  Bit definition for IWDG_SR register  ********************/
#define IWDG_SR_PVU_Pos      (0U)                                              
#define IWDG_SR_PVU_Msk      (0x1U << IWDG_SR_PVU_Pos)                         /*!< 0x00000001 */
#define IWDG_SR_PVU          IWDG_SR_PVU_Msk                                   /*!< Watchdog prescaler value update */
#define IWDG_SR_RVU_Pos      (1U)                                              
#define IWDG_SR_RVU_Msk      (0x1U << IWDG_SR_RVU_Pos)                         /*!< 0x00000002 */
#define IWDG_SR_RVU          IWDG_SR_RVU_Msk                                   /*!< Watchdog counter reload value update */
#define IWDG_SR_WVU_Pos      (2U)                                              
#define IWDG_SR_WVU_Msk      (0x1U << IWDG_SR_WVU_Pos)                         /*!< 0x00000004 */
#define IWDG_SR_WVU          IWDG_SR_WVU_Msk                                   /*!< Watchdog counter window value update */

/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_WINR_WIN_Pos    (0U)                                              
#define IWDG_WINR_WIN_Msk    (0xFFFU << IWDG_WINR_WIN_Pos)                     /*!< 0x00000FFF */
#define IWDG_WINR_WIN        IWDG_WINR_WIN_Msk                                 /*!< Watchdog counter window value */

/******************************************************************************/
/*                                                                            */
/*                         Low Power Timer (LPTTIM)                           */
/*                                                                            */
/******************************************************************************/
/******************  Bit definition for LPTIM_ISR register  *******************/
#define LPTIM_ISR_CMPM_Pos          (0U)                                       
#define LPTIM_ISR_CMPM_Msk          (0x1U << LPTIM_ISR_CMPM_Pos)               /*!< 0x00000001 */
#define LPTIM_ISR_CMPM              LPTIM_ISR_CMPM_Msk                         /*!< Compare match */
#define LPTIM_ISR_ARRM_Pos          (1U)                                       
#define LPTIM_ISR_ARRM_Msk          (0x1U << LPTIM_ISR_ARRM_Pos)               /*!< 0x00000002 */
#define LPTIM_ISR_ARRM              LPTIM_ISR_ARRM_Msk                         /*!< Autoreload match */
#define LPTIM_ISR_EXTTRIG_Pos       (2U)                                       
#define LPTIM_ISR_EXTTRIG_Msk       (0x1U << LPTIM_ISR_EXTTRIG_Pos)            /*!< 0x00000004 */
#define LPTIM_ISR_EXTTRIG           LPTIM_ISR_EXTTRIG_Msk                      /*!< External trigger edge event */
#define LPTIM_ISR_CMPOK_Pos         (3U)                                       
#define LPTIM_ISR_CMPOK_Msk         (0x1U << LPTIM_ISR_CMPOK_Pos)              /*!< 0x00000008 */
#define LPTIM_ISR_CMPOK             LPTIM_ISR_CMPOK_Msk                        /*!< Compare register update OK */
#define LPTIM_ISR_ARROK_Pos         (4U)                                       
#define LPTIM_ISR_ARROK_Msk         (0x1U << LPTIM_ISR_ARROK_Pos)              /*!< 0x00000010 */
#define LPTIM_ISR_ARROK             LPTIM_ISR_ARROK_Msk                        /*!< Autoreload register update OK */
#define LPTIM_ISR_UP_Pos            (5U)                                       
#define LPTIM_ISR_UP_Msk            (0x1U << LPTIM_ISR_UP_Pos)                 /*!< 0x00000020 */
#define LPTIM_ISR_UP                LPTIM_ISR_UP_Msk                           /*!< Counter direction change down to up */
#define LPTIM_ISR_DOWN_Pos          (6U)                                       
#define LPTIM_ISR_DOWN_Msk          (0x1U << LPTIM_ISR_DOWN_Pos)               /*!< 0x00000040 */
#define LPTIM_ISR_DOWN              LPTIM_ISR_DOWN_Msk                         /*!< Counter direction change up to down */

/******************  Bit definition for LPTIM_ICR register  *******************/
#define LPTIM_ICR_CMPMCF_Pos        (0U)                                       
#define LPTIM_ICR_CMPMCF_Msk        (0x1U << LPTIM_ICR_CMPMCF_Pos)             /*!< 0x00000001 */
#define LPTIM_ICR_CMPMCF            LPTIM_ICR_CMPMCF_Msk                       /*!< Compare match Clear Flag */
#define LPTIM_ICR_ARRMCF_Pos        (1U)                                       
#define LPTIM_ICR_ARRMCF_Msk        (0x1U << LPTIM_ICR_ARRMCF_Pos)             /*!< 0x00000002 */
#define LPTIM_ICR_ARRMCF            LPTIM_ICR_ARRMCF_Msk                       /*!< Autoreload match Clear Flag */
#define LPTIM_ICR_EXTTRIGCF_Pos     (2U)                                       
#define LPTIM_ICR_EXTTRIGCF_Msk     (0x1U << LPTIM_ICR_EXTTRIGCF_Pos)          /*!< 0x00000004 */
#define LPTIM_ICR_EXTTRIGCF         LPTIM_ICR_EXTTRIGCF_Msk                    /*!< External trigger edge event Clear Flag */
#define LPTIM_ICR_CMPOKCF_Pos       (3U)                                       
#define LPTIM_ICR_CMPOKCF_Msk       (0x1U << LPTIM_ICR_CMPOKCF_Pos)            /*!< 0x00000008 */
#define LPTIM_ICR_CMPOKCF           LPTIM_ICR_CMPOKCF_Msk                      /*!< Compare register update OK Clear Flag */
#define LPTIM_ICR_ARROKCF_Pos       (4U)                                       
#define LPTIM_ICR_ARROKCF_Msk       (0x1U << LPTIM_ICR_ARROKCF_Pos)            /*!< 0x00000010 */
#define LPTIM_ICR_ARROKCF           LPTIM_ICR_ARROKCF_Msk                      /*!< Autoreload register update OK Clear Flag */
#define LPTIM_ICR_UPCF_Pos          (5U)                                       
#define LPTIM_ICR_UPCF_Msk          (0x1U << LPTIM_ICR_UPCF_Pos)               /*!< 0x00000020 */
#define LPTIM_ICR_UPCF              LPTIM_ICR_UPCF_Msk                         /*!< Counter direction change down to up Clear Flag */
#define LPTIM_ICR_DOWNCF_Pos        (6U)                                       
#define LPTIM_ICR_DOWNCF_Msk        (0x1U << LPTIM_ICR_DOWNCF_Pos)             /*!< 0x00000040 */
#define LPTIM_ICR_DOWNCF            LPTIM_ICR_DOWNCF_Msk                       /*!< Counter direction change up to down Clear Flag */

/******************  Bit definition for LPTIM_IER register ********************/
#define LPTIM_IER_CMPMIE_Pos        (0U)                                       
#define LPTIM_IER_CMPMIE_Msk        (0x1U << LPTIM_IER_CMPMIE_Pos)             /*!< 0x00000001 */
#define LPTIM_IER_CMPMIE            LPTIM_IER_CMPMIE_Msk                       /*!< Compare match Interrupt Enable */
#define LPTIM_IER_ARRMIE_Pos        (1U)                                       
#define LPTIM_IER_ARRMIE_Msk        (0x1U << LPTIM_IER_ARRMIE_Pos)             /*!< 0x00000002 */
#define LPTIM_IER_ARRMIE            LPTIM_IER_ARRMIE_Msk                       /*!< Autoreload match Interrupt Enable */
#define LPTIM_IER_EXTTRIGIE_Pos     (2U)                                       
#define LPTIM_IER_EXTTRIGIE_Msk     (0x1U << LPTIM_IER_EXTTRIGIE_Pos)          /*!< 0x00000004 */
#define LPTIM_IER_EXTTRIGIE         LPTIM_IER_EXTTRIGIE_Msk                    /*!< External trigger edge event Interrupt Enable */
#define LPTIM_IER_CMPOKIE_Pos       (3U)                                       
#define LPTIM_IER_CMPOKIE_Msk       (0x1U << LPTIM_IER_CMPOKIE_Pos)            /*!< 0x00000008 */
#define LPTIM_IER_CMPOKIE           LPTIM_IER_CMPOKIE_Msk                      /*!< Compare register update OK Interrupt Enable */
#define LPTIM_IER_ARROKIE_Pos       (4U)                                       
#define LPTIM_IER_ARROKIE_Msk       (0x1U << LPTIM_IER_ARROKIE_Pos)            /*!< 0x00000010 */
#define LPTIM_IER_ARROKIE           LPTIM_IER_ARROKIE_Msk                      /*!< Autoreload register update OK Interrupt Enable */
#define LPTIM_IER_UPIE_Pos          (5U)                                       
#define LPTIM_IER_UPIE_Msk          (0x1U << LPTIM_IER_UPIE_Pos)               /*!< 0x00000020 */
#define LPTIM_IER_UPIE              LPTIM_IER_UPIE_Msk                         /*!< Counter direction change down to up Interrupt Enable */
#define LPTIM_IER_DOWNIE_Pos        (6U)                                       
#define LPTIM_IER_DOWNIE_Msk        (0x1U << LPTIM_IER_DOWNIE_Pos)             /*!< 0x00000040 */
#define LPTIM_IER_DOWNIE            LPTIM_IER_DOWNIE_Msk                       /*!< Counter direction change up to down Interrupt Enable */

/******************  Bit definition for LPTIM_CFGR register *******************/
#define LPTIM_CFGR_CKSEL_Pos        (0U)                                       
#define LPTIM_CFGR_CKSEL_Msk        (0x1U << LPTIM_CFGR_CKSEL_Pos)             /*!< 0x00000001 */
#define LPTIM_CFGR_CKSEL            LPTIM_CFGR_CKSEL_Msk                       /*!< Clock selector */

#define LPTIM_CFGR_CKPOL_Pos        (1U)                                       
#define LPTIM_CFGR_CKPOL_Msk        (0x3U << LPTIM_CFGR_CKPOL_Pos)             /*!< 0x00000006 */
#define LPTIM_CFGR_CKPOL            LPTIM_CFGR_CKPOL_Msk                       /*!< CKPOL[1:0] bits (Clock polarity) */
#define LPTIM_CFGR_CKPOL_0          (0x1U << LPTIM_CFGR_CKPOL_Pos)             /*!< 0x00000002 */
#define LPTIM_CFGR_CKPOL_1          (0x2U << LPTIM_CFGR_CKPOL_Pos)             /*!< 0x00000004 */

#define LPTIM_CFGR_CKFLT_Pos        (3U)                                       
#define LPTIM_CFGR_CKFLT_Msk        (0x3U << LPTIM_CFGR_CKFLT_Pos)             /*!< 0x00000018 */
#define LPTIM_CFGR_CKFLT            LPTIM_CFGR_CKFLT_Msk                       /*!< CKFLT[1:0] bits (Configurable digital filter for external clock) */
#define LPTIM_CFGR_CKFLT_0          (0x1U << LPTIM_CFGR_CKFLT_Pos)             /*!< 0x00000008 */
#define LPTIM_CFGR_CKFLT_1          (0x2U << LPTIM_CFGR_CKFLT_Pos)             /*!< 0x00000010 */

#define LPTIM_CFGR_TRGFLT_Pos       (6U)                                       
#define LPTIM_CFGR_TRGFLT_Msk       (0x3U << LPTIM_CFGR_TRGFLT_Pos)            /*!< 0x000000C0 */
#define LPTIM_CFGR_TRGFLT           LPTIM_CFGR_TRGFLT_Msk                      /*!< TRGFLT[1:0] bits (Configurable digital filter for trigger) */
#define LPTIM_CFGR_TRGFLT_0         (0x1U << LPTIM_CFGR_TRGFLT_Pos)            /*!< 0x00000040 */
#define LPTIM_CFGR_TRGFLT_1         (0x2U << LPTIM_CFGR_TRGFLT_Pos)            /*!< 0x00000080 */

#define LPTIM_CFGR_PRESC_Pos        (9U)                                       
#define LPTIM_CFGR_PRESC_Msk        (0x7U << LPTIM_CFGR_PRESC_Pos)             /*!< 0x00000E00 */
#define LPTIM_CFGR_PRESC            LPTIM_CFGR_PRESC_Msk                       /*!< PRESC[2:0] bits (Clock prescaler) */
#define LPTIM_CFGR_PRESC_0          (0x1U << LPTIM_CFGR_PRESC_Pos)             /*!< 0x00000200 */
#define LPTIM_CFGR_PRESC_1          (0x2U << LPTIM_CFGR_PRESC_Pos)             /*!< 0x00000400 */
#define LPTIM_CFGR_PRESC_2          (0x4U << LPTIM_CFGR_PRESC_Pos)             /*!< 0x00000800 */

#define LPTIM_CFGR_TRIGSEL_Pos      (13U)                                      
#define LPTIM_CFGR_TRIGSEL_Msk      (0x7U << LPTIM_CFGR_TRIGSEL_Pos)           /*!< 0x0000E000 */
#define LPTIM_CFGR_TRIGSEL          LPTIM_CFGR_TRIGSEL_Msk                     /*!< TRIGSEL[2:0]] bits (Trigger selector) */
#define LPTIM_CFGR_TRIGSEL_0        (0x1U << LPTIM_CFGR_TRIGSEL_Pos)           /*!< 0x00002000 */
#define LPTIM_CFGR_TRIGSEL_1        (0x2U << LPTIM_CFGR_TRIGSEL_Pos)           /*!< 0x00004000 */
#define LPTIM_CFGR_TRIGSEL_2        (0x4U << LPTIM_CFGR_TRIGSEL_Pos)           /*!< 0x00008000 */

#define LPTIM_CFGR_TRIGEN_Pos       (17U)                                      
#define LPTIM_CFGR_TRIGEN_Msk       (0x3U << LPTIM_CFGR_TRIGEN_Pos)            /*!< 0x00060000 */
#define LPTIM_CFGR_TRIGEN           LPTIM_CFGR_TRIGEN_Msk                      /*!< TRIGEN[1:0] bits (Trigger enable and polarity) */
#define LPTIM_CFGR_TRIGEN_0         (0x1U << LPTIM_CFGR_TRIGEN_Pos)            /*!< 0x00020000 */
#define LPTIM_CFGR_TRIGEN_1         (0x2U << LPTIM_CFGR_TRIGEN_Pos)            /*!< 0x00040000 */

#define LPTIM_CFGR_TIMOUT_Pos       (19U)                                      
#define LPTIM_CFGR_TIMOUT_Msk       (0x1U << LPTIM_CFGR_TIMOUT_Pos)            /*!< 0x00080000 */
#define LPTIM_CFGR_TIMOUT           LPTIM_CFGR_TIMOUT_Msk                      /*!< Timout enable */
#define LPTIM_CFGR_WAVE_Pos         (20U)                                      
#define LPTIM_CFGR_WAVE_Msk         (0x1U << LPTIM_CFGR_WAVE_Pos)              /*!< 0x00100000 */
#define LPTIM_CFGR_WAVE             LPTIM_CFGR_WAVE_Msk                        /*!< Waveform shape */
#define LPTIM_CFGR_WAVPOL_Pos       (21U)                                      
#define LPTIM_CFGR_WAVPOL_Msk       (0x1U << LPTIM_CFGR_WAVPOL_Pos)            /*!< 0x00200000 */
#define LPTIM_CFGR_WAVPOL           LPTIM_CFGR_WAVPOL_Msk                      /*!< Waveform shape polarity */
#define LPTIM_CFGR_PRELOAD_Pos      (22U)                                      
#define LPTIM_CFGR_PRELOAD_Msk      (0x1U << LPTIM_CFGR_PRELOAD_Pos)           /*!< 0x00400000 */
#define LPTIM_CFGR_PRELOAD          LPTIM_CFGR_PRELOAD_Msk                     /*!< Reg update mode */
#define LPTIM_CFGR_COUNTMODE_Pos    (23U)                                      
#define LPTIM_CFGR_COUNTMODE_Msk    (0x1U << LPTIM_CFGR_COUNTMODE_Pos)         /*!< 0x00800000 */
#define LPTIM_CFGR_COUNTMODE        LPTIM_CFGR_COUNTMODE_Msk                   /*!< Counter mode enable */
#define LPTIM_CFGR_ENC_Pos          (24U)                                      
#define LPTIM_CFGR_ENC_Msk          (0x1U << LPTIM_CFGR_ENC_Pos)               /*!< 0x01000000 */
#define LPTIM_CFGR_ENC              LPTIM_CFGR_ENC_Msk                         /*!< Encoder mode enable */

/******************  Bit definition for LPTIM_CR register  ********************/
#define LPTIM_CR_ENABLE_Pos         (0U)                                       
#define LPTIM_CR_ENABLE_Msk         (0x1U << LPTIM_CR_ENABLE_Pos)              /*!< 0x00000001 */
#define LPTIM_CR_ENABLE             LPTIM_CR_ENABLE_Msk                        /*!< LPTIMer enable */
#define LPTIM_CR_SNGSTRT_Pos        (1U)                                       
#define LPTIM_CR_SNGSTRT_Msk        (0x1U << LPTIM_CR_SNGSTRT_Pos)             /*!< 0x00000002 */
#define LPTIM_CR_SNGSTRT            LPTIM_CR_SNGSTRT_Msk                       /*!< Timer start in single mode */
#define LPTIM_CR_CNTSTRT_Pos        (2U)                                       
#define LPTIM_CR_CNTSTRT_Msk        (0x1U << LPTIM_CR_CNTSTRT_Pos)             /*!< 0x00000004 */
#define LPTIM_CR_CNTSTRT            LPTIM_CR_CNTSTRT_Msk                       /*!< Timer start in continuous mode */

/******************  Bit definition for LPTIM_CMP register  *******************/
#define LPTIM_CMP_CMP_Pos           (0U)                                       
#define LPTIM_CMP_CMP_Msk           (0xFFFFU << LPTIM_CMP_CMP_Pos)             /*!< 0x0000FFFF */
#define LPTIM_CMP_CMP               LPTIM_CMP_CMP_Msk                          /*!< Compare register */

/******************  Bit definition for LPTIM_ARR register  *******************/
#define LPTIM_ARR_ARR_Pos           (0U)                                       
#define LPTIM_ARR_ARR_Msk           (0xFFFFU << LPTIM_ARR_ARR_Pos)             /*!< 0x0000FFFF */
#define LPTIM_ARR_ARR               LPTIM_ARR_ARR_Msk                          /*!< Auto reload register */

/******************  Bit definition for LPTIM_CNT register  *******************/
#define LPTIM_CNT_CNT_Pos           (0U)                                       
#define LPTIM_CNT_CNT_Msk           (0xFFFFU << LPTIM_CNT_CNT_Pos)             /*!< 0x0000FFFF */
#define LPTIM_CNT_CNT               LPTIM_CNT_CNT_Msk                          /*!< Counter register */

/******************************************************************************/
/*                                                                            */
/*                            MIFARE   Firewall                               */
/*                                                                            */
/******************************************************************************/

/*******Bit definition for CSSA;CSL;NVDSSA;NVDSL;VDSSA;VDSL register */
#define FW_CSSA_ADD_Pos      (8U)                                              
#define FW_CSSA_ADD_Msk      (0xFFFFU << FW_CSSA_ADD_Pos)                      /*!< 0x00FFFF00 */
#define FW_CSSA_ADD          FW_CSSA_ADD_Msk                                   /*!< Code Segment Start Address */ 
#define FW_CSL_LENG_Pos      (8U)                                              
#define FW_CSL_LENG_Msk      (0x3FFFU << FW_CSL_LENG_Pos)                      /*!< 0x003FFF00 */
#define FW_CSL_LENG          FW_CSL_LENG_Msk                                   /*!< Code Segment Length        */  
#define FW_NVDSSA_ADD_Pos    (8U)                                              
#define FW_NVDSSA_ADD_Msk    (0xFFFFU << FW_NVDSSA_ADD_Pos)                    /*!< 0x00FFFF00 */
#define FW_NVDSSA_ADD        FW_NVDSSA_ADD_Msk                                 /*!< Non Volatile Dat Segment Start Address */ 
#define FW_NVDSL_LENG_Pos    (8U)                                              
#define FW_NVDSL_LENG_Msk    (0x3FFFU << FW_NVDSL_LENG_Pos)                    /*!< 0x003FFF00 */
#define FW_NVDSL_LENG        FW_NVDSL_LENG_Msk                                 /*!< Non Volatile Data Segment Length */ 
#define FW_VDSSA_ADD_Pos     (6U)                                              
#define FW_VDSSA_ADD_Msk     (0x3FFU << FW_VDSSA_ADD_Pos)                      /*!< 0x0000FFC0 */
#define FW_VDSSA_ADD         FW_VDSSA_ADD_Msk                                  /*!< Volatile Data Segment Start Address */ 
#define FW_VDSL_LENG_Pos     (6U)                                              
#define FW_VDSL_LENG_Msk     (0x3FFU << FW_VDSL_LENG_Pos)                      /*!< 0x0000FFC0 */
#define FW_VDSL_LENG         FW_VDSL_LENG_Msk                                  /*!< Volatile Data Segment Length */ 

/**************************Bit definition for CR register *********************/
#define FW_CR_FPA_Pos        (0U)                                              
#define FW_CR_FPA_Msk        (0x1U << FW_CR_FPA_Pos)                           /*!< 0x00000001 */
#define FW_CR_FPA            FW_CR_FPA_Msk                                     /*!< Firewall Pre Arm*/ 
#define FW_CR_VDS_Pos        (1U)                                              
#define FW_CR_VDS_Msk        (0x1U << FW_CR_VDS_Pos)                           /*!< 0x00000002 */
#define FW_CR_VDS            FW_CR_VDS_Msk                                     /*!< Volatile Data Sharing*/ 
#define FW_CR_VDE_Pos        (2U)                                              
#define FW_CR_VDE_Msk        (0x1U << FW_CR_VDE_Pos)                           /*!< 0x00000004 */
#define FW_CR_VDE            FW_CR_VDE_Msk                                     /*!< Volatile Data Execution*/ 

/******************************************************************************/
/*                                                                            */
/*                          Power Control (PWR)                               */
/*                                                                            */
/******************************************************************************/

#define PWR_PVD_SUPPORT                     /*!< PVD feature available on all devices: Power Voltage Detection feature */

/********************  Bit definition for PWR_CR register  ********************/
#define PWR_CR_LPSDSR_Pos          (0U)                                        
#define PWR_CR_LPSDSR_Msk          (0x1U << PWR_CR_LPSDSR_Pos)                 /*!< 0x00000001 */
#define PWR_CR_LPSDSR              PWR_CR_LPSDSR_Msk                           /*!< Low-power deepsleep/sleep/low power run */
#define PWR_CR_PDDS_Pos            (1U)                                        
#define PWR_CR_PDDS_Msk            (0x1U << PWR_CR_PDDS_Pos)                   /*!< 0x00000002 */
#define PWR_CR_PDDS                PWR_CR_PDDS_Msk                             /*!< Power Down Deepsleep */
#define PWR_CR_CWUF_Pos            (2U)                                        
#define PWR_CR_CWUF_Msk            (0x1U << PWR_CR_CWUF_Pos)                   /*!< 0x00000004 */
#define PWR_CR_CWUF                PWR_CR_CWUF_Msk                             /*!< Clear Wakeup Flag */
#define PWR_CR_CSBF_Pos            (3U)                                        
#define PWR_CR_CSBF_Msk            (0x1U << PWR_CR_CSBF_Pos)                   /*!< 0x00000008 */
#define PWR_CR_CSBF                PWR_CR_CSBF_Msk                             /*!< Clear Standby Flag */
#define PWR_CR_PVDE_Pos            (4U)                                        
#define PWR_CR_PVDE_Msk            (0x1U << PWR_CR_PVDE_Pos)                   /*!< 0x00000010 */
#define PWR_CR_PVDE                PWR_CR_PVDE_Msk                             /*!< Power Voltage Detector Enable */

#define PWR_CR_PLS_Pos             (5U)                                        
#define PWR_CR_PLS_Msk             (0x7U << PWR_CR_PLS_Pos)                    /*!< 0x000000E0 */
#define PWR_CR_PLS                 PWR_CR_PLS_Msk                              /*!< PLS[2:0] bits (PVD Level Selection) */
#define PWR_CR_PLS_0               (0x1U << PWR_CR_PLS_Pos)                    /*!< 0x00000020 */
#define PWR_CR_PLS_1               (0x2U << PWR_CR_PLS_Pos)                    /*!< 0x00000040 */
#define PWR_CR_PLS_2               (0x4U << PWR_CR_PLS_Pos)                    /*!< 0x00000080 */

/*!< PVD level configuration */
#define PWR_CR_PLS_LEV0            (0x00000000U)                               /*!< PVD level 0 */
#define PWR_CR_PLS_LEV1            (0x00000020U)                               /*!< PVD level 1 */
#define PWR_CR_PLS_LEV2            (0x00000040U)                               /*!< PVD level 2 */
#define PWR_CR_PLS_LEV3            (0x00000060U)                               /*!< PVD level 3 */
#define PWR_CR_PLS_LEV4            (0x00000080U)                               /*!< PVD level 4 */
#define PWR_CR_PLS_LEV5            (0x000000A0U)                               /*!< PVD level 5 */
#define PWR_CR_PLS_LEV6            (0x000000C0U)                               /*!< PVD level 6 */
#define PWR_CR_PLS_LEV7            (0x000000E0U)                               /*!< PVD level 7 */

#define PWR_CR_DBP_Pos             (8U)                                        
#define PWR_CR_DBP_Msk             (0x1U << PWR_CR_DBP_Pos)                    /*!< 0x00000100 */
#define PWR_CR_DBP                 PWR_CR_DBP_Msk                              /*!< Disable Backup Domain write protection */
#define PWR_CR_ULP_Pos             (9U)                                        
#define PWR_CR_ULP_Msk             (0x1U << PWR_CR_ULP_Pos)                    /*!< 0x00000200 */
#define PWR_CR_ULP                 PWR_CR_ULP_Msk                              /*!< Ultra Low Power mode */
#define PWR_CR_FWU_Pos             (10U)                                       
#define PWR_CR_FWU_Msk             (0x1U << PWR_CR_FWU_Pos)                    /*!< 0x00000400 */
#define PWR_CR_FWU                 PWR_CR_FWU_Msk                              /*!< Fast wakeup */

#define PWR_CR_VOS_Pos             (11U)                                       
#define PWR_CR_VOS_Msk             (0x3U << PWR_CR_VOS_Pos)                    /*!< 0x00001800 */
#define PWR_CR_VOS                 PWR_CR_VOS_Msk                              /*!< VOS[1:0] bits (Voltage scaling range selection) */
#define PWR_CR_VOS_0               (0x1U << PWR_CR_VOS_Pos)                    /*!< 0x00000800 */
#define PWR_CR_VOS_1               (0x2U << PWR_CR_VOS_Pos)                    /*!< 0x00001000 */
#define PWR_CR_DSEEKOFF_Pos        (13U)                                       
#define PWR_CR_DSEEKOFF_Msk        (0x1U << PWR_CR_DSEEKOFF_Pos)               /*!< 0x00002000 */
#define PWR_CR_DSEEKOFF            PWR_CR_DSEEKOFF_Msk                         /*!< Deep Sleep mode with EEPROM kept Off */
#define PWR_CR_LPRUN_Pos           (14U)                                       
#define PWR_CR_LPRUN_Msk           (0x1U << PWR_CR_LPRUN_Pos)                  /*!< 0x00004000 */
#define PWR_CR_LPRUN               PWR_CR_LPRUN_Msk                            /*!< Low power run mode */

/*******************  Bit definition for PWR_CSR register  ********************/
#define PWR_CSR_WUF_Pos            (0U)                                        
#define PWR_CSR_WUF_Msk            (0x1U << PWR_CSR_WUF_Pos)                   /*!< 0x00000001 */
#define PWR_CSR_WUF                PWR_CSR_WUF_Msk                             /*!< Wakeup Flag */
#define PWR_CSR_SBF_Pos            (1U)                                        
#define PWR_CSR_SBF_Msk            (0x1U << PWR_CSR_SBF_Pos)                   /*!< 0x00000002 */
#define PWR_CSR_SBF                PWR_CSR_SBF_Msk                             /*!< Standby Flag */
#define PWR_CSR_PVDO_Pos           (2U)                                        
#define PWR_CSR_PVDO_Msk           (0x1U << PWR_CSR_PVDO_Pos)                  /*!< 0x00000004 */
#define PWR_CSR_PVDO               PWR_CSR_PVDO_Msk                            /*!< PVD Output */
#define PWR_CSR_VREFINTRDYF_Pos    (3U)                                        
#define PWR_CSR_VREFINTRDYF_Msk    (0x1U << PWR_CSR_VREFINTRDYF_Pos)           /*!< 0x00000008 */
#define PWR_CSR_VREFINTRDYF        PWR_CSR_VREFINTRDYF_Msk                     /*!< Internal voltage reference (VREFINT) ready flag */
#define PWR_CSR_VOSF_Pos           (4U)                                        
#define PWR_CSR_VOSF_Msk           (0x1U << PWR_CSR_VOSF_Pos)                  /*!< 0x00000010 */
#define PWR_CSR_VOSF               PWR_CSR_VOSF_Msk                            /*!< Voltage Scaling select flag */
#define PWR_CSR_REGLPF_Pos         (5U)                                        
#define PWR_CSR_REGLPF_Msk         (0x1U << PWR_CSR_REGLPF_Pos)                /*!< 0x00000020 */
#define PWR_CSR_REGLPF             PWR_CSR_REGLPF_Msk                          /*!< Regulator LP flag */

#define PWR_CSR_EWUP1_Pos          (8U)                                        
#define PWR_CSR_EWUP1_Msk          (0x1U << PWR_CSR_EWUP1_Pos)                 /*!< 0x00000100 */
#define PWR_CSR_EWUP1              PWR_CSR_EWUP1_Msk                           /*!< Enable WKUP pin 1 */
#define PWR_CSR_EWUP2_Pos          (9U)                                        
#define PWR_CSR_EWUP2_Msk          (0x1U << PWR_CSR_EWUP2_Pos)                 /*!< 0x00000200 */
#define PWR_CSR_EWUP2              PWR_CSR_EWUP2_Msk                           /*!< Enable WKUP pin 2 */

/******************************************************************************/
/*                                                                            */
/*                         Reset and Clock Control                            */
/*                                                                            */
/******************************************************************************/

#define RCC_HSECSS_SUPPORT          /*!< HSE CSS feature activation support */

/********************  Bit definition for RCC_CR register  ********************/
#define RCC_CR_HSION_Pos                 (0U)                                  
#define RCC_CR_HSION_Msk                 (0x1U << RCC_CR_HSION_Pos)            /*!< 0x00000001 */
#define RCC_CR_HSION                     RCC_CR_HSION_Msk                      /*!< Internal High Speed clock enable */
#define RCC_CR_HSIKERON_Pos              (1U)                                  
#define RCC_CR_HSIKERON_Msk              (0x1U << RCC_CR_HSIKERON_Pos)         /*!< 0x00000002 */
#define RCC_CR_HSIKERON                  RCC_CR_HSIKERON_Msk                   /*!< Internal High Speed clock enable for some IPs Kernel */
#define RCC_CR_HSIRDY_Pos                (2U)                                  
#define RCC_CR_HSIRDY_Msk                (0x1U << RCC_CR_HSIRDY_Pos)           /*!< 0x00000004 */
#define RCC_CR_HSIRDY                    RCC_CR_HSIRDY_Msk                     /*!< Internal High Speed clock ready flag */
#define RCC_CR_HSIDIVEN_Pos              (3U)                                  
#define RCC_CR_HSIDIVEN_Msk              (0x1U << RCC_CR_HSIDIVEN_Pos)         /*!< 0x00000008 */
#define RCC_CR_HSIDIVEN                  RCC_CR_HSIDIVEN_Msk                   /*!< Internal High Speed clock divider enable */
#define RCC_CR_HSIDIVF_Pos               (4U)                                  
#define RCC_CR_HSIDIVF_Msk               (0x1U << RCC_CR_HSIDIVF_Pos)          /*!< 0x00000010 */
#define RCC_CR_HSIDIVF                   RCC_CR_HSIDIVF_Msk                    /*!< Internal High Speed clock divider flag */
#define RCC_CR_MSION_Pos                 (8U)                                  
#define RCC_CR_MSION_Msk                 (0x1U << RCC_CR_MSION_Pos)            /*!< 0x00000100 */
#define RCC_CR_MSION                     RCC_CR_MSION_Msk                      /*!< Internal Multi Speed clock enable */
#define RCC_CR_MSIRDY_Pos                (9U)                                  
#define RCC_CR_MSIRDY_Msk                (0x1U << RCC_CR_MSIRDY_Pos)           /*!< 0x00000200 */
#define RCC_CR_MSIRDY                    RCC_CR_MSIRDY_Msk                     /*!< Internal Multi Speed clock ready flag */
#define RCC_CR_HSEON_Pos                 (16U)                                 
#define RCC_CR_HSEON_Msk                 (0x1U << RCC_CR_HSEON_Pos)            /*!< 0x00010000 */
#define RCC_CR_HSEON                     RCC_CR_HSEON_Msk                      /*!< External High Speed clock enable */
#define RCC_CR_HSERDY_Pos                (17U)                                 
#define RCC_CR_HSERDY_Msk                (0x1U << RCC_CR_HSERDY_Pos)           /*!< 0x00020000 */
#define RCC_CR_HSERDY                    RCC_CR_HSERDY_Msk                     /*!< External High Speed clock ready flag */
#define RCC_CR_HSEBYP_Pos                (18U)                                 
#define RCC_CR_HSEBYP_Msk                (0x1U << RCC_CR_HSEBYP_Pos)           /*!< 0x00040000 */
#define RCC_CR_HSEBYP                    RCC_CR_HSEBYP_Msk                     /*!< External High Speed clock Bypass */
#define RCC_CR_CSSHSEON_Pos              (19U)                                 
#define RCC_CR_CSSHSEON_Msk              (0x1U << RCC_CR_CSSHSEON_Pos)         /*!< 0x00080000 */
#define RCC_CR_CSSHSEON                  RCC_CR_CSSHSEON_Msk                   /*!< HSE Clock Security System enable */
#define RCC_CR_RTCPRE_Pos                (20U)                                 
#define RCC_CR_RTCPRE_Msk                (0x3U << RCC_CR_RTCPRE_Pos)           /*!< 0x00300000 */
#define RCC_CR_RTCPRE                    RCC_CR_RTCPRE_Msk                     /*!< RTC prescaler [1:0] bits */
#define RCC_CR_RTCPRE_0                  (0x1U << RCC_CR_RTCPRE_Pos)           /*!< 0x00100000 */
#define RCC_CR_RTCPRE_1                  (0x2U << RCC_CR_RTCPRE_Pos)           /*!< 0x00200000 */
#define RCC_CR_PLLON_Pos                 (24U)                                 
#define RCC_CR_PLLON_Msk                 (0x1U << RCC_CR_PLLON_Pos)            /*!< 0x01000000 */
#define RCC_CR_PLLON                     RCC_CR_PLLON_Msk                      /*!< PLL enable */
#define RCC_CR_PLLRDY_Pos                (25U)                                 
#define RCC_CR_PLLRDY_Msk                (0x1U << RCC_CR_PLLRDY_Pos)           /*!< 0x02000000 */
#define RCC_CR_PLLRDY                    RCC_CR_PLLRDY_Msk                     /*!< PLL clock ready flag */

/* Reference defines */
#define RCC_CR_CSSON     RCC_CR_CSSHSEON

/********************  Bit definition for RCC_ICSCR register  *****************/
#define RCC_ICSCR_HSICAL_Pos             (0U)                                  
#define RCC_ICSCR_HSICAL_Msk             (0xFFU << RCC_ICSCR_HSICAL_Pos)       /*!< 0x000000FF */
#define RCC_ICSCR_HSICAL                 RCC_ICSCR_HSICAL_Msk                  /*!< Internal High Speed clock Calibration */
#define RCC_ICSCR_HSITRIM_Pos            (8U)                                  
#define RCC_ICSCR_HSITRIM_Msk            (0x1FU << RCC_ICSCR_HSITRIM_Pos)      /*!< 0x00001F00 */
#define RCC_ICSCR_HSITRIM                RCC_ICSCR_HSITRIM_Msk                 /*!< Internal High Speed clock trimming */

#define RCC_ICSCR_MSIRANGE_Pos           (13U)                                 
#define RCC_ICSCR_MSIRANGE_Msk           (0x7U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x0000E000 */
#define RCC_ICSCR_MSIRANGE               RCC_ICSCR_MSIRANGE_Msk                /*!< Internal Multi Speed clock Range */
#define RCC_ICSCR_MSIRANGE_0             (0x0U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x00000000 */
#define RCC_ICSCR_MSIRANGE_1             (0x1U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x00002000 */
#define RCC_ICSCR_MSIRANGE_2             (0x2U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x00004000 */
#define RCC_ICSCR_MSIRANGE_3             (0x3U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x00006000 */
#define RCC_ICSCR_MSIRANGE_4             (0x4U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x00008000 */
#define RCC_ICSCR_MSIRANGE_5             (0x5U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x0000A000 */
#define RCC_ICSCR_MSIRANGE_6             (0x6U << RCC_ICSCR_MSIRANGE_Pos)      /*!< 0x0000C000 */
#define RCC_ICSCR_MSICAL_Pos             (16U)                                 
#define RCC_ICSCR_MSICAL_Msk             (0xFFU << RCC_ICSCR_MSICAL_Pos)       /*!< 0x00FF0000 */
#define RCC_ICSCR_MSICAL                 RCC_ICSCR_MSICAL_Msk                  /*!< Internal Multi Speed clock Calibration */
#define RCC_ICSCR_MSITRIM_Pos            (24U)                                 
#define RCC_ICSCR_MSITRIM_Msk            (0xFFU << RCC_ICSCR_MSITRIM_Pos)      /*!< 0xFF000000 */
#define RCC_ICSCR_MSITRIM                RCC_ICSCR_MSITRIM_Msk                 /*!< Internal Multi Speed clock trimming */


/*******************  Bit definition for RCC_CFGR register  *******************/
/*!< SW configuration */
#define RCC_CFGR_SW_Pos                      (0U)                              
#define RCC_CFGR_SW_Msk                      (0x3U << RCC_CFGR_SW_Pos)         /*!< 0x00000003 */
#define RCC_CFGR_SW                          RCC_CFGR_SW_Msk                   /*!< SW[1:0] bits (System clock Switch) */
#define RCC_CFGR_SW_0                        (0x1U << RCC_CFGR_SW_Pos)         /*!< 0x00000001 */
#define RCC_CFGR_SW_1                        (0x2U << RCC_CFGR_SW_Pos)         /*!< 0x00000002 */

#define RCC_CFGR_SW_MSI                      (0x00000000U)                     /*!< MSI selected as system clock */
#define RCC_CFGR_SW_HSI                      (0x00000001U)                     /*!< HSI selected as system clock */
#define RCC_CFGR_SW_HSE                      (0x00000002U)                     /*!< HSE selected as system clock */
#define RCC_CFGR_SW_PLL                      (0x00000003U)                     /*!< PLL selected as system clock */

/*!< SWS configuration */
#define RCC_CFGR_SWS_Pos                     (2U)                              
#define RCC_CFGR_SWS_Msk                     (0x3U << RCC_CFGR_SWS_Pos)        /*!< 0x0000000C */
#define RCC_CFGR_SWS                         RCC_CFGR_SWS_Msk                  /*!< SWS[1:0] bits (System Clock Switch Status) */
#define RCC_CFGR_SWS_0                       (0x1U << RCC_CFGR_SWS_Pos)        /*!< 0x00000004 */
#define RCC_CFGR_SWS_1                       (0x2U << RCC_CFGR_SWS_Pos)        /*!< 0x00000008 */

#define RCC_CFGR_SWS_MSI                     (0x00000000U)                     /*!< MSI oscillator used as system clock */
#define RCC_CFGR_SWS_HSI                     (0x00000004U)                     /*!< HSI oscillator used as system clock */
#define RCC_CFGR_SWS_HSE                     (0x00000008U)                     /*!< HSE oscillator used as system clock */
#define RCC_CFGR_SWS_PLL                     (0x0000000CU)                     /*!< PLL used as system clock */

/*!< HPRE configuration */
#define RCC_CFGR_HPRE_Pos                    (4U)                              
#define RCC_CFGR_HPRE_Msk                    (0xFU << RCC_CFGR_HPRE_Pos)       /*!< 0x000000F0 */
#define RCC_CFGR_HPRE                        RCC_CFGR_HPRE_Msk                 /*!< HPRE[3:0] bits (AHB prescaler) */
#define RCC_CFGR_HPRE_0                      (0x1U << RCC_CFGR_HPRE_Pos)       /*!< 0x00000010 */
#define RCC_CFGR_HPRE_1                      (0x2U << RCC_CFGR_HPRE_Pos)       /*!< 0x00000020 */
#define RCC_CFGR_HPRE_2                      (0x4U << RCC_CFGR_HPRE_Pos)       /*!< 0x00000040 */
#define RCC_CFGR_HPRE_3                      (0x8U << RCC_CFGR_HPRE_Pos)       /*!< 0x00000080 */

#define RCC_CFGR_HPRE_DIV1                   (0x00000000U)                     /*!< SYSCLK not divided */
#define RCC_CFGR_HPRE_DIV2                   (0x00000080U)                     /*!< SYSCLK divided by 2 */
#define RCC_CFGR_HPRE_DIV4                   (0x00000090U)                     /*!< SYSCLK divided by 4 */
#define RCC_CFGR_HPRE_DIV8                   (0x000000A0U)                     /*!< SYSCLK divided by 8 */
#define RCC_CFGR_HPRE_DIV16                  (0x000000B0U)                     /*!< SYSCLK divided by 16 */
#define RCC_CFGR_HPRE_DIV64                  (0x000000C0U)                     /*!< SYSCLK divided by 64 */
#define RCC_CFGR_HPRE_DIV128                 (0x000000D0U)                     /*!< SYSCLK divided by 128 */
#define RCC_CFGR_HPRE_DIV256                 (0x000000E0U)                     /*!< SYSCLK divided by 256 */
#define RCC_CFGR_HPRE_DIV512                 (0x000000F0U)                     /*!< SYSCLK divided by 512 */

/*!< PPRE1 configuration */
#define RCC_CFGR_PPRE1_Pos                   (8U)                              
#define RCC_CFGR_PPRE1_Msk                   (0x7U << RCC_CFGR_PPRE1_Pos)      /*!< 0x00000700 */
#define RCC_CFGR_PPRE1                       RCC_CFGR_PPRE1_Msk                /*!< PRE1[2:0] bits (APB1 prescaler) */
#define RCC_CFGR_PPRE1_0                     (0x1U << RCC_CFGR_PPRE1_Pos)      /*!< 0x00000100 */
#define RCC_CFGR_PPRE1_1                     (0x2U << RCC_CFGR_PPRE1_Pos)      /*!< 0x00000200 */
#define RCC_CFGR_PPRE1_2                     (0x4U << RCC_CFGR_PPRE1_Pos)      /*!< 0x00000400 */

#define RCC_CFGR_PPRE1_DIV1                  (0x00000000U)                     /*!< HCLK not divided */
#define RCC_CFGR_PPRE1_DIV2                  (0x00000400U)                     /*!< HCLK divided by 2 */
#define RCC_CFGR_PPRE1_DIV4                  (0x00000500U)                     /*!< HCLK divided by 4 */
#define RCC_CFGR_PPRE1_DIV8                  (0x00000600U)                     /*!< HCLK divided by 8 */
#define RCC_CFGR_PPRE1_DIV16                 (0x00000700U)                     /*!< HCLK divided by 16 */

/*!< PPRE2 configuration */
#define RCC_CFGR_PPRE2_Pos                   (11U)                             
#define RCC_CFGR_PPRE2_Msk                   (0x7U << RCC_CFGR_PPRE2_Pos)      /*!< 0x00003800 */
#define RCC_CFGR_PPRE2                       RCC_CFGR_PPRE2_Msk                /*!< PRE2[2:0] bits (APB2 prescaler) */
#define RCC_CFGR_PPRE2_0                     (0x1U << RCC_CFGR_PPRE2_Pos)      /*!< 0x00000800 */
#define RCC_CFGR_PPRE2_1                     (0x2U << RCC_CFGR_PPRE2_Pos)      /*!< 0x00001000 */
#define RCC_CFGR_PPRE2_2                     (0x4U << RCC_CFGR_PPRE2_Pos)      /*!< 0x00002000 */

#define RCC_CFGR_PPRE2_DIV1                  (0x00000000U)                     /*!< HCLK not divided */
#define RCC_CFGR_PPRE2_DIV2                  (0x00002000U)                     /*!< HCLK divided by 2 */
#define RCC_CFGR_PPRE2_DIV4                  (0x00002800U)                     /*!< HCLK divided by 4 */
#define RCC_CFGR_PPRE2_DIV8                  (0x00003000U)                     /*!< HCLK divided by 8 */
#define RCC_CFGR_PPRE2_DIV16                 (0x00003800U)                     /*!< HCLK divided by 16 */

#define RCC_CFGR_STOPWUCK_Pos                (15U)                             
#define RCC_CFGR_STOPWUCK_Msk                (0x1U << RCC_CFGR_STOPWUCK_Pos)   /*!< 0x00008000 */
#define RCC_CFGR_STOPWUCK                    RCC_CFGR_STOPWUCK_Msk             /*!< Wake Up from Stop Clock selection */

/*!< PLL entry clock source*/
#define RCC_CFGR_PLLSRC_Pos                  (16U)                             
#define RCC_CFGR_PLLSRC_Msk                  (0x1U << RCC_CFGR_PLLSRC_Pos)     /*!< 0x00010000 */
#define RCC_CFGR_PLLSRC                      RCC_CFGR_PLLSRC_Msk               /*!< PLL entry clock source */

#define RCC_CFGR_PLLSRC_HSI                  (0x00000000U)                     /*!< HSI as PLL entry clock source */
#define RCC_CFGR_PLLSRC_HSE                  (0x00010000U)                     /*!< HSE as PLL entry clock source */


/*!< PLLMUL configuration */
#define RCC_CFGR_PLLMUL_Pos                  (18U)                             
#define RCC_CFGR_PLLMUL_Msk                  (0xFU << RCC_CFGR_PLLMUL_Pos)     /*!< 0x003C0000 */
#define RCC_CFGR_PLLMUL                      RCC_CFGR_PLLMUL_Msk               /*!< PLLMUL[3:0] bits (PLL multiplication factor) */
#define RCC_CFGR_PLLMUL_0                    (0x1U << RCC_CFGR_PLLMUL_Pos)     /*!< 0x00040000 */
#define RCC_CFGR_PLLMUL_1                    (0x2U << RCC_CFGR_PLLMUL_Pos)     /*!< 0x00080000 */
#define RCC_CFGR_PLLMUL_2                    (0x4U << RCC_CFGR_PLLMUL_Pos)     /*!< 0x00100000 */
#define RCC_CFGR_PLLMUL_3                    (0x8U << RCC_CFGR_PLLMUL_Pos)     /*!< 0x00200000 */

#define RCC_CFGR_PLLMUL3                     (0x00000000U)                     /*!< PLL input clock * 3 */
#define RCC_CFGR_PLLMUL4                     (0x00040000U)                     /*!< PLL input clock * 4 */
#define RCC_CFGR_PLLMUL6                     (0x00080000U)                     /*!< PLL input clock * 6 */
#define RCC_CFGR_PLLMUL8                     (0x000C0000U)                     /*!< PLL input clock * 8 */
#define RCC_CFGR_PLLMUL12                    (0x00100000U)                     /*!< PLL input clock * 12 */
#define RCC_CFGR_PLLMUL16                    (0x00140000U)                     /*!< PLL input clock * 16 */
#define RCC_CFGR_PLLMUL24                    (0x00180000U)                     /*!< PLL input clock * 24 */
#define RCC_CFGR_PLLMUL32                    (0x001C0000U)                     /*!< PLL input clock * 32 */
#define RCC_CFGR_PLLMUL48                    (0x00200000U)                     /*!< PLL input clock * 48 */

/*!< PLLDIV configuration */
#define RCC_CFGR_PLLDIV_Pos                  (22U)                             
#define RCC_CFGR_PLLDIV_Msk                  (0x3U << RCC_CFGR_PLLDIV_Pos)     /*!< 0x00C00000 */
#define RCC_CFGR_PLLDIV                      RCC_CFGR_PLLDIV_Msk               /*!< PLLDIV[1:0] bits (PLL Output Division) */
#define RCC_CFGR_PLLDIV_0                    (0x1U << RCC_CFGR_PLLDIV_Pos)     /*!< 0x00400000 */
#define RCC_CFGR_PLLDIV_1                    (0x2U << RCC_CFGR_PLLDIV_Pos)     /*!< 0x00800000 */

#define RCC_CFGR_PLLDIV2_Pos                 (22U)                             
#define RCC_CFGR_PLLDIV2_Msk                 (0x1U << RCC_CFGR_PLLDIV2_Pos)    /*!< 0x00400000 */
#define RCC_CFGR_PLLDIV2                     RCC_CFGR_PLLDIV2_Msk              /*!< PLL clock output = CKVCO / 2 */
#define RCC_CFGR_PLLDIV3_Pos                 (23U)                             
#define RCC_CFGR_PLLDIV3_Msk                 (0x1U << RCC_CFGR_PLLDIV3_Pos)    /*!< 0x00800000 */
#define RCC_CFGR_PLLDIV3                     RCC_CFGR_PLLDIV3_Msk              /*!< PLL clock output = CKVCO / 3 */
#define RCC_CFGR_PLLDIV4_Pos                 (22U)                             
#define RCC_CFGR_PLLDIV4_Msk                 (0x3U << RCC_CFGR_PLLDIV4_Pos)    /*!< 0x00C00000 */
#define RCC_CFGR_PLLDIV4                     RCC_CFGR_PLLDIV4_Msk              /*!< PLL clock output = CKVCO / 4 */

/*!< MCO configuration */
#define RCC_CFGR_MCOSEL_Pos                  (24U)                             
#define RCC_CFGR_MCOSEL_Msk                  (0xFU << RCC_CFGR_MCOSEL_Pos)     /*!< 0x0F000000 */
#define RCC_CFGR_MCOSEL                      RCC_CFGR_MCOSEL_Msk               /*!< MCO[3:0] bits (Microcontroller Clock Output) */
#define RCC_CFGR_MCOSEL_0                    (0x1U << RCC_CFGR_MCOSEL_Pos)     /*!< 0x01000000 */
#define RCC_CFGR_MCOSEL_1                    (0x2U << RCC_CFGR_MCOSEL_Pos)     /*!< 0x02000000 */
#define RCC_CFGR_MCOSEL_2                    (0x4U << RCC_CFGR_MCOSEL_Pos)     /*!< 0x04000000 */
#define RCC_CFGR_MCOSEL_3                    (0x8U << RCC_CFGR_MCOSEL_Pos)     /*!< 0x08000000 */

#define RCC_CFGR_MCOSEL_NOCLOCK              (0x00000000U)                     /*!< No clock */
#define RCC_CFGR_MCOSEL_SYSCLK_Pos           (24U)                             
#define RCC_CFGR_MCOSEL_SYSCLK_Msk           (0x1U << RCC_CFGR_MCOSEL_SYSCLK_Pos) /*!< 0x01000000 */
#define RCC_CFGR_MCOSEL_SYSCLK               RCC_CFGR_MCOSEL_SYSCLK_Msk        /*!< System clock selected as MCO source */
#define RCC_CFGR_MCOSEL_HSI_Pos              (25U)                             
#define RCC_CFGR_MCOSEL_HSI_Msk              (0x1U << RCC_CFGR_MCOSEL_HSI_Pos) /*!< 0x02000000 */
#define RCC_CFGR_MCOSEL_HSI                  RCC_CFGR_MCOSEL_HSI_Msk           /*!< Internal 16 MHz RC oscillator clock selected */
#define RCC_CFGR_MCOSEL_MSI_Pos              (24U)                             
#define RCC_CFGR_MCOSEL_MSI_Msk              (0x3U << RCC_CFGR_MCOSEL_MSI_Pos) /*!< 0x03000000 */
#define RCC_CFGR_MCOSEL_MSI                  RCC_CFGR_MCOSEL_MSI_Msk           /*!< Internal Medium Speed RC oscillator clock selected */
#define RCC_CFGR_MCOSEL_HSE_Pos              (26U)                             
#define RCC_CFGR_MCOSEL_HSE_Msk              (0x1U << RCC_CFGR_MCOSEL_HSE_Pos) /*!< 0x04000000 */
#define RCC_CFGR_MCOSEL_HSE                  RCC_CFGR_MCOSEL_HSE_Msk           /*!< External 1-25 MHz oscillator clock selected */
#define RCC_CFGR_MCOSEL_PLL_Pos              (24U)                             
#define RCC_CFGR_MCOSEL_PLL_Msk              (0x5U << RCC_CFGR_MCOSEL_PLL_Pos) /*!< 0x05000000 */
#define RCC_CFGR_MCOSEL_PLL                  RCC_CFGR_MCOSEL_PLL_Msk           /*!< PLL clock divided */
#define RCC_CFGR_MCOSEL_LSI_Pos              (25U)                             
#define RCC_CFGR_MCOSEL_LSI_Msk              (0x3U << RCC_CFGR_MCOSEL_LSI_Pos) /*!< 0x06000000 */
#define RCC_CFGR_MCOSEL_LSI                  RCC_CFGR_MCOSEL_LSI_Msk           /*!< LSI selected */
#define RCC_CFGR_MCOSEL_LSE_Pos              (24U)                             
#define RCC_CFGR_MCOSEL_LSE_Msk              (0x7U << RCC_CFGR_MCOSEL_LSE_Pos) /*!< 0x07000000 */
#define RCC_CFGR_MCOSEL_LSE                  RCC_CFGR_MCOSEL_LSE_Msk           /*!< LSE selected */

#define RCC_CFGR_MCOPRE_Pos                  (28U)                             
#define RCC_CFGR_MCOPRE_Msk                  (0x7U << RCC_CFGR_MCOPRE_Pos)     /*!< 0x70000000 */
#define RCC_CFGR_MCOPRE                      RCC_CFGR_MCOPRE_Msk               /*!< MCO prescaler */
#define RCC_CFGR_MCOPRE_0                    (0x1U << RCC_CFGR_MCOPRE_Pos)     /*!< 0x10000000 */
#define RCC_CFGR_MCOPRE_1                    (0x2U << RCC_CFGR_MCOPRE_Pos)     /*!< 0x20000000 */
#define RCC_CFGR_MCOPRE_2                    (0x4U << RCC_CFGR_MCOPRE_Pos)     /*!< 0x40000000 */

#define RCC_CFGR_MCOPRE_DIV1                 (0x00000000U)                     /*!< MCO is divided by 1 */
#define RCC_CFGR_MCOPRE_DIV2                 (0x10000000U)                     /*!< MCO is divided by 2 */
#define RCC_CFGR_MCOPRE_DIV4                 (0x20000000U)                     /*!< MCO is divided by 4 */
#define RCC_CFGR_MCOPRE_DIV8                 (0x30000000U)                     /*!< MCO is divided by 8 */
#define RCC_CFGR_MCOPRE_DIV16                (0x40000000U)                     /*!< MCO is divided by 16 */

/* Legacy defines */
#define RCC_CFGR_MCO_NOCLOCK   RCC_CFGR_MCOSEL_NOCLOCK   
#define RCC_CFGR_MCO_SYSCLK    RCC_CFGR_MCOSEL_SYSCLK    
#define RCC_CFGR_MCO_HSI       RCC_CFGR_MCOSEL_HSI       
#define RCC_CFGR_MCO_MSI       RCC_CFGR_MCOSEL_MSI       
#define RCC_CFGR_MCO_HSE       RCC_CFGR_MCOSEL_HSE       
#define RCC_CFGR_MCO_PLL       RCC_CFGR_MCOSEL_PLL       
#define RCC_CFGR_MCO_LSI       RCC_CFGR_MCOSEL_LSI       
#define RCC_CFGR_MCO_LSE       RCC_CFGR_MCOSEL_LSE       
#ifdef RCC_CFGR_MCOSEL_HSI48
#define RCC_CFGR_MCO_HSI48     RCC_CFGR_MCOSEL_HSI48   
#endif

#define RCC_CFGR_MCO_PRE                    RCC_CFGR_MCOPRE          /*!< MCO prescaler */
#define RCC_CFGR_MCO_PRE_1                  RCC_CFGR_MCOPRE_DIV1        /*!< MCO is divided by 1 */
#define RCC_CFGR_MCO_PRE_2                  RCC_CFGR_MCOPRE_DIV2        /*!< MCO is divided by 1 */
#define RCC_CFGR_MCO_PRE_4                  RCC_CFGR_MCOPRE_DIV4        /*!< MCO is divided by 1 */
#define RCC_CFGR_MCO_PRE_8                  RCC_CFGR_MCOPRE_DIV8        /*!< MCO is divided by 1 */
#define RCC_CFGR_MCO_PRE_16                 RCC_CFGR_MCOPRE_DIV16       /*!< MCO is divided by 1 */

/*!<******************  Bit definition for RCC_CIER register  ********************/
#define RCC_CIER_LSIRDYIE_Pos            (0U)                                  
#define RCC_CIER_LSIRDYIE_Msk            (0x1U << RCC_CIER_LSIRDYIE_Pos)       /*!< 0x00000001 */
#define RCC_CIER_LSIRDYIE                RCC_CIER_LSIRDYIE_Msk                 /*!< LSI Ready Interrupt Enable */
#define RCC_CIER_LSERDYIE_Pos            (1U)                                  
#define RCC_CIER_LSERDYIE_Msk            (0x1U << RCC_CIER_LSERDYIE_Pos)       /*!< 0x00000002 */
#define RCC_CIER_LSERDYIE                RCC_CIER_LSERDYIE_Msk                 /*!< LSE Ready Interrupt Enable */
#define RCC_CIER_HSIRDYIE_Pos            (2U)                                  
#define RCC_CIER_HSIRDYIE_Msk            (0x1U << RCC_CIER_HSIRDYIE_Pos)       /*!< 0x00000004 */
#define RCC_CIER_HSIRDYIE                RCC_CIER_HSIRDYIE_Msk                 /*!< HSI Ready Interrupt Enable */
#define RCC_CIER_HSERDYIE_Pos            (3U)                                  
#define RCC_CIER_HSERDYIE_Msk            (0x1U << RCC_CIER_HSERDYIE_Pos)       /*!< 0x00000008 */
#define RCC_CIER_HSERDYIE                RCC_CIER_HSERDYIE_Msk                 /*!< HSE Ready Interrupt Enable */
#define RCC_CIER_PLLRDYIE_Pos            (4U)                                  
#define RCC_CIER_PLLRDYIE_Msk            (0x1U << RCC_CIER_PLLRDYIE_Pos)       /*!< 0x00000010 */
#define RCC_CIER_PLLRDYIE                RCC_CIER_PLLRDYIE_Msk                 /*!< PLL Ready Interrupt Enable */
#define RCC_CIER_MSIRDYIE_Pos            (5U)                                  
#define RCC_CIER_MSIRDYIE_Msk            (0x1U << RCC_CIER_MSIRDYIE_Pos)       /*!< 0x00000020 */
#define RCC_CIER_MSIRDYIE                RCC_CIER_MSIRDYIE_Msk                 /*!< MSI Ready Interrupt Enable */
#define RCC_CIER_CSSLSE_Pos              (7U)                                  
#define RCC_CIER_CSSLSE_Msk              (0x1U << RCC_CIER_CSSLSE_Pos)         /*!< 0x00000080 */
#define RCC_CIER_CSSLSE                  RCC_CIER_CSSLSE_Msk                   /*!< LSE CSS Interrupt Enable */

/* Reference defines */
#define RCC_CIER_LSECSSIE                    RCC_CIER_CSSLSE

/*!<******************  Bit definition for RCC_CIFR register  ********************/
#define RCC_CIFR_LSIRDYF_Pos             (0U)                                  
#define RCC_CIFR_LSIRDYF_Msk             (0x1U << RCC_CIFR_LSIRDYF_Pos)        /*!< 0x00000001 */
#define RCC_CIFR_LSIRDYF                 RCC_CIFR_LSIRDYF_Msk                  /*!< LSI Ready Interrupt flag */
#define RCC_CIFR_LSERDYF_Pos             (1U)                                  
#define RCC_CIFR_LSERDYF_Msk             (0x1U << RCC_CIFR_LSERDYF_Pos)        /*!< 0x00000002 */
#define RCC_CIFR_LSERDYF                 RCC_CIFR_LSERDYF_Msk                  /*!< LSE Ready Interrupt flag */
#define RCC_CIFR_HSIRDYF_Pos             (2U)                                  
#define RCC_CIFR_HSIRDYF_Msk             (0x1U << RCC_CIFR_HSIRDYF_Pos)        /*!< 0x00000004 */
#define RCC_CIFR_HSIRDYF                 RCC_CIFR_HSIRDYF_Msk                  /*!< HSI Ready Interrupt flag */
#define RCC_CIFR_HSERDYF_Pos             (3U)                                  
#define RCC_CIFR_HSERDYF_Msk             (0x1U << RCC_CIFR_HSERDYF_Pos)        /*!< 0x00000008 */
#define RCC_CIFR_HSERDYF                 RCC_CIFR_HSERDYF_Msk                  /*!< HSE Ready Interrupt flag */
#define RCC_CIFR_PLLRDYF_Pos             (4U)                                  
#define RCC_CIFR_PLLRDYF_Msk             (0x1U << RCC_CIFR_PLLRDYF_Pos)        /*!< 0x00000010 */
#define RCC_CIFR_PLLRDYF                 RCC_CIFR_PLLRDYF_Msk                  /*!< PLL Ready Interrupt flag */
#define RCC_CIFR_MSIRDYF_Pos             (5U)                                  
#define RCC_CIFR_MSIRDYF_Msk             (0x1U << RCC_CIFR_MSIRDYF_Pos)        /*!< 0x00000020 */
#define RCC_CIFR_MSIRDYF                 RCC_CIFR_MSIRDYF_Msk                  /*!< MSI Ready Interrupt flag */
#define RCC_CIFR_CSSLSEF_Pos             (7U)                                  
#define RCC_CIFR_CSSLSEF_Msk             (0x1U << RCC_CIFR_CSSLSEF_Pos)        /*!< 0x00000080 */
#define RCC_CIFR_CSSLSEF                 RCC_CIFR_CSSLSEF_Msk                  /*!< LSE Clock Security System Interrupt flag */
#define RCC_CIFR_CSSHSEF_Pos             (8U)                                  
#define RCC_CIFR_CSSHSEF_Msk             (0x1U << RCC_CIFR_CSSHSEF_Pos)        /*!< 0x00000100 */
#define RCC_CIFR_CSSHSEF                 RCC_CIFR_CSSHSEF_Msk                  /*!< HSE Clock Security System Interrupt flag */

/* Reference defines */
#define RCC_CIFR_LSECSSF                    RCC_CIFR_CSSLSEF
#define RCC_CIFR_CSSF                       RCC_CIFR_CSSHSEF

/*!<******************  Bit definition for RCC_CICR register  ********************/
#define RCC_CICR_LSIRDYC_Pos             (0U)                                  
#define RCC_CICR_LSIRDYC_Msk             (0x1U << RCC_CICR_LSIRDYC_Pos)        /*!< 0x00000001 */
#define RCC_CICR_LSIRDYC                 RCC_CICR_LSIRDYC_Msk                  /*!< LSI Ready Interrupt Clear */
#define RCC_CICR_LSERDYC_Pos             (1U)                                  
#define RCC_CICR_LSERDYC_Msk             (0x1U << RCC_CICR_LSERDYC_Pos)        /*!< 0x00000002 */
#define RCC_CICR_LSERDYC                 RCC_CICR_LSERDYC_Msk                  /*!< LSE Ready Interrupt Clear */
#define RCC_CICR_HSIRDYC_Pos             (2U)                                  
#define RCC_CICR_HSIRDYC_Msk             (0x1U << RCC_CICR_HSIRDYC_Pos)        /*!< 0x00000004 */
#define RCC_CICR_HSIRDYC                 RCC_CICR_HSIRDYC_Msk                  /*!< HSI Ready Interrupt Clear */
#define RCC_CICR_HSERDYC_Pos             (3U)                                  
#define RCC_CICR_HSERDYC_Msk             (0x1U << RCC_CICR_HSERDYC_Pos)        /*!< 0x00000008 */
#define RCC_CICR_HSERDYC                 RCC_CICR_HSERDYC_Msk                  /*!< HSE Ready Interrupt Clear */
#define RCC_CICR_PLLRDYC_Pos             (4U)                                  
#define RCC_CICR_PLLRDYC_Msk             (0x1U << RCC_CICR_PLLRDYC_Pos)        /*!< 0x00000010 */
#define RCC_CICR_PLLRDYC                 RCC_CICR_PLLRDYC_Msk                  /*!< PLL Ready Interrupt Clear */
#define RCC_CICR_MSIRDYC_Pos             (5U)                                  
#define RCC_CICR_MSIRDYC_Msk             (0x1U << RCC_CICR_MSIRDYC_Pos)        /*!< 0x00000020 */
#define RCC_CICR_MSIRDYC                 RCC_CICR_MSIRDYC_Msk                  /*!< MSI Ready Interrupt Clear */
#define RCC_CICR_CSSLSEC_Pos             (7U)                                  
#define RCC_CICR_CSSLSEC_Msk             (0x1U << RCC_CICR_CSSLSEC_Pos)        /*!< 0x00000080 */
#define RCC_CICR_CSSLSEC                 RCC_CICR_CSSLSEC_Msk                  /*!< LSE Clock Security System Interrupt Clear */
#define RCC_CICR_CSSHSEC_Pos             (8U)                                  
#define RCC_CICR_CSSHSEC_Msk             (0x1U << RCC_CICR_CSSHSEC_Pos)        /*!< 0x00000100 */
#define RCC_CICR_CSSHSEC                 RCC_CICR_CSSHSEC_Msk                  /*!< HSE Clock Security System Interrupt Clear */

/* Reference defines */
#define RCC_CICR_LSECSSC                    RCC_CICR_CSSLSEC
#define RCC_CICR_CSSC                       RCC_CICR_CSSHSEC
/*****************  Bit definition for RCC_IOPRSTR register  ******************/
#define RCC_IOPRSTR_IOPARST_Pos          (0U)                                  
#define RCC_IOPRSTR_IOPARST_Msk          (0x1U << RCC_IOPRSTR_IOPARST_Pos)     /*!< 0x00000001 */
#define RCC_IOPRSTR_IOPARST              RCC_IOPRSTR_IOPARST_Msk               /*!< GPIO port A reset */
#define RCC_IOPRSTR_IOPBRST_Pos          (1U)                                  
#define RCC_IOPRSTR_IOPBRST_Msk          (0x1U << RCC_IOPRSTR_IOPBRST_Pos)     /*!< 0x00000002 */
#define RCC_IOPRSTR_IOPBRST              RCC_IOPRSTR_IOPBRST_Msk               /*!< GPIO port B reset */
#define RCC_IOPRSTR_IOPCRST_Pos          (2U)                                  
#define RCC_IOPRSTR_IOPCRST_Msk          (0x1U << RCC_IOPRSTR_IOPCRST_Pos)     /*!< 0x00000004 */
#define RCC_IOPRSTR_IOPCRST              RCC_IOPRSTR_IOPCRST_Msk               /*!< GPIO port C reset */
#define RCC_IOPRSTR_IOPDRST_Pos          (3U)                                  
#define RCC_IOPRSTR_IOPDRST_Msk          (0x1U << RCC_IOPRSTR_IOPDRST_Pos)     /*!< 0x00000008 */
#define RCC_IOPRSTR_IOPDRST              RCC_IOPRSTR_IOPDRST_Msk               /*!< GPIO port D reset */
#define RCC_IOPRSTR_IOPHRST_Pos          (7U)                                  
#define RCC_IOPRSTR_IOPHRST_Msk          (0x1U << RCC_IOPRSTR_IOPHRST_Pos)     /*!< 0x00000080 */
#define RCC_IOPRSTR_IOPHRST              RCC_IOPRSTR_IOPHRST_Msk               /*!< GPIO port H reset */

/* Reference defines */
#define RCC_IOPRSTR_GPIOARST                RCC_IOPRSTR_IOPARST        /*!< GPIO port A reset */
#define RCC_IOPRSTR_GPIOBRST                RCC_IOPRSTR_IOPBRST        /*!< GPIO port B reset */
#define RCC_IOPRSTR_GPIOCRST                RCC_IOPRSTR_IOPCRST        /*!< GPIO port C reset */
#define RCC_IOPRSTR_GPIODRST                RCC_IOPRSTR_IOPDRST        /*!< GPIO port D reset */
#define RCC_IOPRSTR_GPIOHRST                RCC_IOPRSTR_IOPHRST        /*!< GPIO port H reset */


/******************  Bit definition for RCC_AHBRST register  ******************/
#define RCC_AHBRSTR_DMARST_Pos           (0U)                                  
#define RCC_AHBRSTR_DMARST_Msk           (0x1U << RCC_AHBRSTR_DMARST_Pos)      /*!< 0x00000001 */
#define RCC_AHBRSTR_DMARST               RCC_AHBRSTR_DMARST_Msk                /*!< DMA1 reset */
#define RCC_AHBRSTR_MIFRST_Pos           (8U)                                  
#define RCC_AHBRSTR_MIFRST_Msk           (0x1U << RCC_AHBRSTR_MIFRST_Pos)      /*!< 0x00000100 */
#define RCC_AHBRSTR_MIFRST               RCC_AHBRSTR_MIFRST_Msk                /*!< Memory interface reset reset */
#define RCC_AHBRSTR_CRCRST_Pos           (12U)                                 
#define RCC_AHBRSTR_CRCRST_Msk           (0x1U << RCC_AHBRSTR_CRCRST_Pos)      /*!< 0x00001000 */
#define RCC_AHBRSTR_CRCRST               RCC_AHBRSTR_CRCRST_Msk                /*!< CRC reset */

/* Reference defines */
#define RCC_AHBRSTR_DMA1RST                 RCC_AHBRSTR_DMARST            /*!< DMA1 reset */

/*****************  Bit definition for RCC_APB2RSTR register  *****************/
#define RCC_APB2RSTR_SYSCFGRST_Pos       (0U)                                  
#define RCC_APB2RSTR_SYSCFGRST_Msk       (0x1U << RCC_APB2RSTR_SYSCFGRST_Pos)  /*!< 0x00000001 */
#define RCC_APB2RSTR_SYSCFGRST           RCC_APB2RSTR_SYSCFGRST_Msk            /*!< SYSCFG clock reset */
#define RCC_APB2RSTR_TIM21RST_Pos        (2U)                                  
#define RCC_APB2RSTR_TIM21RST_Msk        (0x1U << RCC_APB2RSTR_TIM21RST_Pos)   /*!< 0x00000004 */
#define RCC_APB2RSTR_TIM21RST            RCC_APB2RSTR_TIM21RST_Msk             /*!< TIM21 clock reset */
#define RCC_APB2RSTR_TIM22RST_Pos        (5U)                                  
#define RCC_APB2RSTR_TIM22RST_Msk        (0x1U << RCC_APB2RSTR_TIM22RST_Pos)   /*!< 0x00000020 */
#define RCC_APB2RSTR_TIM22RST            RCC_APB2RSTR_TIM22RST_Msk             /*!< TIM22 clock reset */
#define RCC_APB2RSTR_ADCRST_Pos          (9U)                                  
#define RCC_APB2RSTR_ADCRST_Msk          (0x1U << RCC_APB2RSTR_ADCRST_Pos)     /*!< 0x00000200 */
#define RCC_APB2RSTR_ADCRST              RCC_APB2RSTR_ADCRST_Msk               /*!< ADC1 clock reset */
#define RCC_APB2RSTR_SPI1RST_Pos         (12U)                                 
#define RCC_APB2RSTR_SPI1RST_Msk         (0x1U << RCC_APB2RSTR_SPI1RST_Pos)    /*!< 0x00001000 */
#define RCC_APB2RSTR_SPI1RST             RCC_APB2RSTR_SPI1RST_Msk              /*!< SPI1 clock reset */
#define RCC_APB2RSTR_USART1RST_Pos       (14U)                                 
#define RCC_APB2RSTR_USART1RST_Msk       (0x1U << RCC_APB2RSTR_USART1RST_Pos)  /*!< 0x00004000 */
#define RCC_APB2RSTR_USART1RST           RCC_APB2RSTR_USART1RST_Msk            /*!< USART1 clock reset */
#define RCC_APB2RSTR_DBGRST_Pos          (22U)                                 
#define RCC_APB2RSTR_DBGRST_Msk          (0x1U << RCC_APB2RSTR_DBGRST_Pos)     /*!< 0x00400000 */
#define RCC_APB2RSTR_DBGRST              RCC_APB2RSTR_DBGRST_Msk               /*!< DBGMCU clock reset */

/* Reference defines */
#define RCC_APB2RSTR_ADC1RST                RCC_APB2RSTR_ADCRST           /*!< ADC1 clock reset */
#define RCC_APB2RSTR_DBGMCURST              RCC_APB2RSTR_DBGRST           /*!< DBGMCU clock reset */

/*****************  Bit definition for RCC_APB1RSTR register  *****************/
#define RCC_APB1RSTR_TIM2RST_Pos         (0U)                                  
#define RCC_APB1RSTR_TIM2RST_Msk         (0x1U << RCC_APB1RSTR_TIM2RST_Pos)    /*!< 0x00000001 */
#define RCC_APB1RSTR_TIM2RST             RCC_APB1RSTR_TIM2RST_Msk              /*!< Timer 2 clock reset */
#define RCC_APB1RSTR_TIM6RST_Pos         (4U)                                  
#define RCC_APB1RSTR_TIM6RST_Msk         (0x1U << RCC_APB1RSTR_TIM6RST_Pos)    /*!< 0x00000010 */
#define RCC_APB1RSTR_TIM6RST             RCC_APB1RSTR_TIM6RST_Msk              /*!< Timer 6 clock reset */
#define RCC_APB1RSTR_WWDGRST_Pos         (11U)                                 
#define RCC_APB1RSTR_WWDGRST_Msk         (0x1U << RCC_APB1RSTR_WWDGRST_Pos)    /*!< 0x00000800 */
#define RCC_APB1RSTR_WWDGRST             RCC_APB1RSTR_WWDGRST_Msk              /*!< Window Watchdog clock reset */
#define RCC_APB1RSTR_SPI2RST_Pos         (14U)                                 
#define RCC_APB1RSTR_SPI2RST_Msk         (0x1U << RCC_APB1RSTR_SPI2RST_Pos)    /*!< 0x00004000 */
#define RCC_APB1RSTR_SPI2RST             RCC_APB1RSTR_SPI2RST_Msk              /*!< SPI2 clock reset */
#define RCC_APB1RSTR_USART2RST_Pos       (17U)                                 
#define RCC_APB1RSTR_USART2RST_Msk       (0x1U << RCC_APB1RSTR_USART2RST_Pos)  /*!< 0x00020000 */
#define RCC_APB1RSTR_USART2RST           RCC_APB1RSTR_USART2RST_Msk            /*!< USART 2 clock reset */
#define RCC_APB1RSTR_LPUART1RST_Pos      (18U)                                 
#define RCC_APB1RSTR_LPUART1RST_Msk      (0x1U << RCC_APB1RSTR_LPUART1RST_Pos) /*!< 0x00040000 */
#define RCC_APB1RSTR_LPUART1RST          RCC_APB1RSTR_LPUART1RST_Msk           /*!< LPUART1 clock reset */
#define RCC_APB1RSTR_I2C1RST_Pos         (21U)                                 
#define RCC_APB1RSTR_I2C1RST_Msk         (0x1U << RCC_APB1RSTR_I2C1RST_Pos)    /*!< 0x00200000 */
#define RCC_APB1RSTR_I2C1RST             RCC_APB1RSTR_I2C1RST_Msk              /*!< I2C 1 clock reset */
#define RCC_APB1RSTR_I2C2RST_Pos         (22U)                                 
#define RCC_APB1RSTR_I2C2RST_Msk         (0x1U << RCC_APB1RSTR_I2C2RST_Pos)    /*!< 0x00400000 */
#define RCC_APB1RSTR_I2C2RST             RCC_APB1RSTR_I2C2RST_Msk              /*!< I2C 2 clock reset */
#define RCC_APB1RSTR_PWRRST_Pos          (28U)                                 
#define RCC_APB1RSTR_PWRRST_Msk          (0x1U << RCC_APB1RSTR_PWRRST_Pos)     /*!< 0x10000000 */
#define RCC_APB1RSTR_PWRRST              RCC_APB1RSTR_PWRRST_Msk               /*!< PWR clock reset */
#define RCC_APB1RSTR_LPTIM1RST_Pos       (31U)                                 
#define RCC_APB1RSTR_LPTIM1RST_Msk       (0x1U << RCC_APB1RSTR_LPTIM1RST_Pos)  /*!< 0x80000000 */
#define RCC_APB1RSTR_LPTIM1RST           RCC_APB1RSTR_LPTIM1RST_Msk            /*!< LPTIM1 clock reset */

/*****************  Bit definition for RCC_IOPENR register  ******************/
#define RCC_IOPENR_IOPAEN_Pos            (0U)                                  
#define RCC_IOPENR_IOPAEN_Msk            (0x1U << RCC_IOPENR_IOPAEN_Pos)       /*!< 0x00000001 */
#define RCC_IOPENR_IOPAEN                RCC_IOPENR_IOPAEN_Msk                 /*!< GPIO port A clock enable */
#define RCC_IOPENR_IOPBEN_Pos            (1U)                                  
#define RCC_IOPENR_IOPBEN_Msk            (0x1U << RCC_IOPENR_IOPBEN_Pos)       /*!< 0x00000002 */
#define RCC_IOPENR_IOPBEN                RCC_IOPENR_IOPBEN_Msk                 /*!< GPIO port B clock enable */
#define RCC_IOPENR_IOPCEN_Pos            (2U)                                  
#define RCC_IOPENR_IOPCEN_Msk            (0x1U << RCC_IOPENR_IOPCEN_Pos)       /*!< 0x00000004 */
#define RCC_IOPENR_IOPCEN                RCC_IOPENR_IOPCEN_Msk                 /*!< GPIO port C clock enable */
#define RCC_IOPENR_IOPDEN_Pos            (3U)                                  
#define RCC_IOPENR_IOPDEN_Msk            (0x1U << RCC_IOPENR_IOPDEN_Pos)       /*!< 0x00000008 */
#define RCC_IOPENR_IOPDEN                RCC_IOPENR_IOPDEN_Msk                 /*!< GPIO port D clock enable */
#define RCC_IOPENR_IOPHEN_Pos            (7U)                                  
#define RCC_IOPENR_IOPHEN_Msk            (0x1U << RCC_IOPENR_IOPHEN_Pos)       /*!< 0x00000080 */
#define RCC_IOPENR_IOPHEN                RCC_IOPENR_IOPHEN_Msk                 /*!< GPIO port H clock enable */

/* Reference defines */
#define RCC_IOPENR_GPIOAEN                  RCC_IOPENR_IOPAEN        /*!< GPIO port A clock enable */
#define RCC_IOPENR_GPIOBEN                  RCC_IOPENR_IOPBEN        /*!< GPIO port B clock enable */
#define RCC_IOPENR_GPIOCEN                  RCC_IOPENR_IOPCEN        /*!< GPIO port C clock enable */
#define RCC_IOPENR_GPIODEN                  RCC_IOPENR_IOPDEN        /*!< GPIO port D clock enable */
#define RCC_IOPENR_GPIOHEN                  RCC_IOPENR_IOPHEN        /*!< GPIO port H clock enable */

/*****************  Bit definition for RCC_AHBENR register  ******************/
#define RCC_AHBENR_DMAEN_Pos             (0U)                                  
#define RCC_AHBENR_DMAEN_Msk             (0x1U << RCC_AHBENR_DMAEN_Pos)        /*!< 0x00000001 */
#define RCC_AHBENR_DMAEN                 RCC_AHBENR_DMAEN_Msk                  /*!< DMA1 clock enable */
#define RCC_AHBENR_MIFEN_Pos             (8U)                                  
#define RCC_AHBENR_MIFEN_Msk             (0x1U << RCC_AHBENR_MIFEN_Pos)        /*!< 0x00000100 */
#define RCC_AHBENR_MIFEN                 RCC_AHBENR_MIFEN_Msk                  /*!< NVM interface clock enable bit */
#define RCC_AHBENR_CRCEN_Pos             (12U)                                 
#define RCC_AHBENR_CRCEN_Msk             (0x1U << RCC_AHBENR_CRCEN_Pos)        /*!< 0x00001000 */
#define RCC_AHBENR_CRCEN                 RCC_AHBENR_CRCEN_Msk                  /*!< CRC clock enable */

/* Reference defines */
#define RCC_AHBENR_DMA1EN                   RCC_AHBENR_DMAEN              /*!< DMA1 clock enable */

/*****************  Bit definition for RCC_APB2ENR register  ******************/
#define RCC_APB2ENR_SYSCFGEN_Pos         (0U)                                  
#define RCC_APB2ENR_SYSCFGEN_Msk         (0x1U << RCC_APB2ENR_SYSCFGEN_Pos)    /*!< 0x00000001 */
#define RCC_APB2ENR_SYSCFGEN             RCC_APB2ENR_SYSCFGEN_Msk              /*!< SYSCFG clock enable */
#define RCC_APB2ENR_TIM21EN_Pos          (2U)                                  
#define RCC_APB2ENR_TIM21EN_Msk          (0x1U << RCC_APB2ENR_TIM21EN_Pos)     /*!< 0x00000004 */
#define RCC_APB2ENR_TIM21EN              RCC_APB2ENR_TIM21EN_Msk               /*!< TIM21 clock enable */
#define RCC_APB2ENR_TIM22EN_Pos          (5U)                                  
#define RCC_APB2ENR_TIM22EN_Msk          (0x1U << RCC_APB2ENR_TIM22EN_Pos)     /*!< 0x00000020 */
#define RCC_APB2ENR_TIM22EN              RCC_APB2ENR_TIM22EN_Msk               /*!< TIM22 clock enable */
#define RCC_APB2ENR_FWEN_Pos             (7U)                                  
#define RCC_APB2ENR_FWEN_Msk             (0x1U << RCC_APB2ENR_FWEN_Pos)        /*!< 0x00000080 */
#define RCC_APB2ENR_FWEN                 RCC_APB2ENR_FWEN_Msk                  /*!< MiFare Firewall clock enable */
#define RCC_APB2ENR_ADCEN_Pos            (9U)                                  
#define RCC_APB2ENR_ADCEN_Msk            (0x1U << RCC_APB2ENR_ADCEN_Pos)       /*!< 0x00000200 */
#define RCC_APB2ENR_ADCEN                RCC_APB2ENR_ADCEN_Msk                 /*!< ADC1 clock enable */
#define RCC_APB2ENR_SPI1EN_Pos           (12U)                                 
#define RCC_APB2ENR_SPI1EN_Msk           (0x1U << RCC_APB2ENR_SPI1EN_Pos)      /*!< 0x00001000 */
#define RCC_APB2ENR_SPI1EN               RCC_APB2ENR_SPI1EN_Msk                /*!< SPI1 clock enable */
#define RCC_APB2ENR_USART1EN_Pos         (14U)                                 
#define RCC_APB2ENR_USART1EN_Msk         (0x1U << RCC_APB2ENR_USART1EN_Pos)    /*!< 0x00004000 */
#define RCC_APB2ENR_USART1EN             RCC_APB2ENR_USART1EN_Msk              /*!< USART1 clock enable */
#define RCC_APB2ENR_DBGEN_Pos            (22U)                                 
#define RCC_APB2ENR_DBGEN_Msk            (0x1U << RCC_APB2ENR_DBGEN_Pos)       /*!< 0x00400000 */
#define RCC_APB2ENR_DBGEN                RCC_APB2ENR_DBGEN_Msk                 /*!< DBGMCU clock enable */

/* Reference defines */

#define RCC_APB2ENR_MIFIEN                  RCC_APB2ENR_FWEN              /*!< MiFare Firewall clock enable */
#define RCC_APB2ENR_ADC1EN                  RCC_APB2ENR_ADCEN             /*!< ADC1 clock enable */
#define RCC_APB2ENR_DBGMCUEN                RCC_APB2ENR_DBGEN             /*!< DBGMCU clock enable */

/*****************  Bit definition for RCC_APB1ENR register  ******************/
#define RCC_APB1ENR_TIM2EN_Pos           (0U)                                  
#define RCC_APB1ENR_TIM2EN_Msk           (0x1U << RCC_APB1ENR_TIM2EN_Pos)      /*!< 0x00000001 */
#define RCC_APB1ENR_TIM2EN               RCC_APB1ENR_TIM2EN_Msk                /*!< Timer 2 clock enable */
#define RCC_APB1ENR_TIM6EN_Pos           (4U)                                  
#define RCC_APB1ENR_TIM6EN_Msk           (0x1U << RCC_APB1ENR_TIM6EN_Pos)      /*!< 0x00000010 */
#define RCC_APB1ENR_TIM6EN               RCC_APB1ENR_TIM6EN_Msk                /*!< Timer 6 clock enable */
#define RCC_APB1ENR_WWDGEN_Pos           (11U)                                 
#define RCC_APB1ENR_WWDGEN_Msk           (0x1U << RCC_APB1ENR_WWDGEN_Pos)      /*!< 0x00000800 */
#define RCC_APB1ENR_WWDGEN               RCC_APB1ENR_WWDGEN_Msk                /*!< Window Watchdog clock enable */
#define RCC_APB1ENR_SPI2EN_Pos           (14U)                                 
#define RCC_APB1ENR_SPI2EN_Msk           (0x1U << RCC_APB1ENR_SPI2EN_Pos)      /*!< 0x00004000 */
#define RCC_APB1ENR_SPI2EN               RCC_APB1ENR_SPI2EN_Msk                /*!< SPI2 clock enable */
#define RCC_APB1ENR_USART2EN_Pos         (17U)                                 
#define RCC_APB1ENR_USART2EN_Msk         (0x1U << RCC_APB1ENR_USART2EN_Pos)    /*!< 0x00020000 */
#define RCC_APB1ENR_USART2EN             RCC_APB1ENR_USART2EN_Msk              /*!< USART2 clock enable */
#define RCC_APB1ENR_LPUART1EN_Pos        (18U)                                 
#define RCC_APB1ENR_LPUART1EN_Msk        (0x1U << RCC_APB1ENR_LPUART1EN_Pos)   /*!< 0x00040000 */
#define RCC_APB1ENR_LPUART1EN            RCC_APB1ENR_LPUART1EN_Msk             /*!< LPUART1 clock enable */
#define RCC_APB1ENR_I2C1EN_Pos           (21U)                                 
#define RCC_APB1ENR_I2C1EN_Msk           (0x1U << RCC_APB1ENR_I2C1EN_Pos)      /*!< 0x00200000 */
#define RCC_APB1ENR_I2C1EN               RCC_APB1ENR_I2C1EN_Msk                /*!< I2C1 clock enable */
#define RCC_APB1ENR_I2C2EN_Pos           (22U)                                 
#define RCC_APB1ENR_I2C2EN_Msk           (0x1U << RCC_APB1ENR_I2C2EN_Pos)      /*!< 0x00400000 */
#define RCC_APB1ENR_I2C2EN               RCC_APB1ENR_I2C2EN_Msk                /*!< I2C2 clock enable */
#define RCC_APB1ENR_PWREN_Pos            (28U)                                 
#define RCC_APB1ENR_PWREN_Msk            (0x1U << RCC_APB1ENR_PWREN_Pos)       /*!< 0x10000000 */
#define RCC_APB1ENR_PWREN                RCC_APB1ENR_PWREN_Msk                 /*!< PWR clock enable */
#define RCC_APB1ENR_LPTIM1EN_Pos         (31U)                                 
#define RCC_APB1ENR_LPTIM1EN_Msk         (0x1U << RCC_APB1ENR_LPTIM1EN_Pos)    /*!< 0x80000000 */
#define RCC_APB1ENR_LPTIM1EN             RCC_APB1ENR_LPTIM1EN_Msk              /*!< LPTIM1 clock enable */

/******************  Bit definition for RCC_IOPSMENR register  ****************/
#define RCC_IOPSMENR_IOPASMEN_Pos        (0U)                                  
#define RCC_IOPSMENR_IOPASMEN_Msk        (0x1U << RCC_IOPSMENR_IOPASMEN_Pos)   /*!< 0x00000001 */
#define RCC_IOPSMENR_IOPASMEN            RCC_IOPSMENR_IOPASMEN_Msk             /*!< GPIO port A clock enabled in sleep mode */
#define RCC_IOPSMENR_IOPBSMEN_Pos        (1U)                                  
#define RCC_IOPSMENR_IOPBSMEN_Msk        (0x1U << RCC_IOPSMENR_IOPBSMEN_Pos)   /*!< 0x00000002 */
#define RCC_IOPSMENR_IOPBSMEN            RCC_IOPSMENR_IOPBSMEN_Msk             /*!< GPIO port B clock enabled in sleep mode */
#define RCC_IOPSMENR_IOPCSMEN_Pos        (2U)                                  
#define RCC_IOPSMENR_IOPCSMEN_Msk        (0x1U << RCC_IOPSMENR_IOPCSMEN_Pos)   /*!< 0x00000004 */
#define RCC_IOPSMENR_IOPCSMEN            RCC_IOPSMENR_IOPCSMEN_Msk             /*!< GPIO port C clock enabled in sleep mode */
#define RCC_IOPSMENR_IOPDSMEN_Pos        (3U)                                  
#define RCC_IOPSMENR_IOPDSMEN_Msk        (0x1U << RCC_IOPSMENR_IOPDSMEN_Pos)   /*!< 0x00000008 */
#define RCC_IOPSMENR_IOPDSMEN            RCC_IOPSMENR_IOPDSMEN_Msk             /*!< GPIO port D clock enabled in sleep mode */
#define RCC_IOPSMENR_IOPHSMEN_Pos        (7U)                                  
#define RCC_IOPSMENR_IOPHSMEN_Msk        (0x1U << RCC_IOPSMENR_IOPHSMEN_Pos)   /*!< 0x00000080 */
#define RCC_IOPSMENR_IOPHSMEN            RCC_IOPSMENR_IOPHSMEN_Msk             /*!< GPIO port H clock enabled in sleep mode */

/* Reference defines */
#define RCC_IOPSMENR_GPIOASMEN              RCC_IOPSMENR_IOPASMEN        /*!< GPIO port A clock enabled in sleep mode */
#define RCC_IOPSMENR_GPIOBSMEN              RCC_IOPSMENR_IOPBSMEN        /*!< GPIO port B clock enabled in sleep mode */
#define RCC_IOPSMENR_GPIOCSMEN              RCC_IOPSMENR_IOPCSMEN        /*!< GPIO port C clock enabled in sleep mode */
#define RCC_IOPSMENR_GPIODSMEN              RCC_IOPSMENR_IOPDSMEN        /*!< GPIO port D clock enabled in sleep mode */
#define RCC_IOPSMENR_GPIOHSMEN              RCC_IOPSMENR_IOPHSMEN        /*!< GPIO port H clock enabled in sleep mode */

/*****************  Bit definition for RCC_AHBSMENR register  ******************/
#define RCC_AHBSMENR_DMASMEN_Pos         (0U)                                  
#define RCC_AHBSMENR_DMASMEN_Msk         (0x1U << RCC_AHBSMENR_DMASMEN_Pos)    /*!< 0x00000001 */
#define RCC_AHBSMENR_DMASMEN             RCC_AHBSMENR_DMASMEN_Msk              /*!< DMA1 clock enabled in sleep mode */
#define RCC_AHBSMENR_MIFSMEN_Pos         (8U)                                  
#define RCC_AHBSMENR_MIFSMEN_Msk         (0x1U << RCC_AHBSMENR_MIFSMEN_Pos)    /*!< 0x00000100 */
#define RCC_AHBSMENR_MIFSMEN             RCC_AHBSMENR_MIFSMEN_Msk              /*!< NVM interface clock enable during sleep mode */
#define RCC_AHBSMENR_SRAMSMEN_Pos        (9U)                                  
#define RCC_AHBSMENR_SRAMSMEN_Msk        (0x1U << RCC_AHBSMENR_SRAMSMEN_Pos)   /*!< 0x00000200 */
#define RCC_AHBSMENR_SRAMSMEN            RCC_AHBSMENR_SRAMSMEN_Msk             /*!< SRAM clock enabled in sleep mode */
#define RCC_AHBSMENR_CRCSMEN_Pos         (12U)                                 
#define RCC_AHBSMENR_CRCSMEN_Msk         (0x1U << RCC_AHBSMENR_CRCSMEN_Pos)    /*!< 0x00001000 */
#define RCC_AHBSMENR_CRCSMEN             RCC_AHBSMENR_CRCSMEN_Msk              /*!< CRC clock enabled in sleep mode */

/* Reference defines */
#define RCC_AHBSMENR_DMA1SMEN               RCC_AHBSMENR_DMASMEN          /*!< DMA1 clock enabled in sleep mode */

/*****************  Bit definition for RCC_APB2SMENR register  ******************/
#define RCC_APB2SMENR_SYSCFGSMEN_Pos     (0U)                                  
#define RCC_APB2SMENR_SYSCFGSMEN_Msk     (0x1U << RCC_APB2SMENR_SYSCFGSMEN_Pos) /*!< 0x00000001 */
#define RCC_APB2SMENR_SYSCFGSMEN         RCC_APB2SMENR_SYSCFGSMEN_Msk          /*!< SYSCFG clock enabled in sleep mode */
#define RCC_APB2SMENR_TIM21SMEN_Pos      (2U)                                  
#define RCC_APB2SMENR_TIM21SMEN_Msk      (0x1U << RCC_APB2SMENR_TIM21SMEN_Pos) /*!< 0x00000004 */
#define RCC_APB2SMENR_TIM21SMEN          RCC_APB2SMENR_TIM21SMEN_Msk           /*!< TIM21 clock enabled in sleep mode */
#define RCC_APB2SMENR_TIM22SMEN_Pos      (5U)                                  
#define RCC_APB2SMENR_TIM22SMEN_Msk      (0x1U << RCC_APB2SMENR_TIM22SMEN_Pos) /*!< 0x00000020 */
#define RCC_APB2SMENR_TIM22SMEN          RCC_APB2SMENR_TIM22SMEN_Msk           /*!< TIM22 clock enabled in sleep mode */
#define RCC_APB2SMENR_ADCSMEN_Pos        (9U)                                  
#define RCC_APB2SMENR_ADCSMEN_Msk        (0x1U << RCC_APB2SMENR_ADCSMEN_Pos)   /*!< 0x00000200 */
#define RCC_APB2SMENR_ADCSMEN            RCC_APB2SMENR_ADCSMEN_Msk             /*!< ADC1 clock enabled in sleep mode */
#define RCC_APB2SMENR_SPI1SMEN_Pos       (12U)                                 
#define RCC_APB2SMENR_SPI1SMEN_Msk       (0x1U << RCC_APB2SMENR_SPI1SMEN_Pos)  /*!< 0x00001000 */
#define RCC_APB2SMENR_SPI1SMEN           RCC_APB2SMENR_SPI1SMEN_Msk            /*!< SPI1 clock enabled in sleep mode */
#define RCC_APB2SMENR_USART1SMEN_Pos     (14U)                                 
#define RCC_APB2SMENR_USART1SMEN_Msk     (0x1U << RCC_APB2SMENR_USART1SMEN_Pos) /*!< 0x00004000 */
#define RCC_APB2SMENR_USART1SMEN         RCC_APB2SMENR_USART1SMEN_Msk          /*!< USART1 clock enabled in sleep mode */
#define RCC_APB2SMENR_DBGSMEN_Pos        (22U)                                 
#define RCC_APB2SMENR_DBGSMEN_Msk        (0x1U << RCC_APB2SMENR_DBGSMEN_Pos)   /*!< 0x00400000 */
#define RCC_APB2SMENR_DBGSMEN            RCC_APB2SMENR_DBGSMEN_Msk             /*!< DBGMCU clock enabled in sleep mode */

/* Reference defines */
#define RCC_APB2SMENR_ADC1SMEN              RCC_APB2SMENR_ADCSMEN         /*!< ADC1 clock enabled in sleep mode */
#define RCC_APB2SMENR_DBGMCUSMEN            RCC_APB2SMENR_DBGSMEN         /*!< DBGMCU clock enabled in sleep mode */

/*****************  Bit definition for RCC_APB1SMENR register  ******************/
#define RCC_APB1SMENR_TIM2SMEN_Pos       (0U)                                  
#define RCC_APB1SMENR_TIM2SMEN_Msk       (0x1U << RCC_APB1SMENR_TIM2SMEN_Pos)  /*!< 0x00000001 */
#define RCC_APB1SMENR_TIM2SMEN           RCC_APB1SMENR_TIM2SMEN_Msk            /*!< Timer 2 clock enabled in sleep mode */
#define RCC_APB1SMENR_TIM6SMEN_Pos       (4U)                                  
#define RCC_APB1SMENR_TIM6SMEN_Msk       (0x1U << RCC_APB1SMENR_TIM6SMEN_Pos)  /*!< 0x00000010 */
#define RCC_APB1SMENR_TIM6SMEN           RCC_APB1SMENR_TIM6SMEN_Msk            /*!< Timer 6 clock enabled in sleep mode */
#define RCC_APB1SMENR_WWDGSMEN_Pos       (11U)                                 
#define RCC_APB1SMENR_WWDGSMEN_Msk       (0x1U << RCC_APB1SMENR_WWDGSMEN_Pos)  /*!< 0x00000800 */
#define RCC_APB1SMENR_WWDGSMEN           RCC_APB1SMENR_WWDGSMEN_Msk            /*!< Window Watchdog clock enabled in sleep mode */
#define RCC_APB1SMENR_SPI2SMEN_Pos       (14U)                                 
#define RCC_APB1SMENR_SPI2SMEN_Msk       (0x1U << RCC_APB1SMENR_SPI2SMEN_Pos)  /*!< 0x00004000 */
#define RCC_APB1SMENR_SPI2SMEN           RCC_APB1SMENR_SPI2SMEN_Msk            /*!< SPI2 clock enabled in sleep mode */
#define RCC_APB1SMENR_USART2SMEN_Pos     (17U)                                 
#define RCC_APB1SMENR_USART2SMEN_Msk     (0x1U << RCC_APB1SMENR_USART2SMEN_Pos) /*!< 0x00020000 */
#define RCC_APB1SMENR_USART2SMEN         RCC_APB1SMENR_USART2SMEN_Msk          /*!< USART2 clock enabled in sleep mode */
#define RCC_APB1SMENR_LPUART1SMEN_Pos    (18U)                                 
#define RCC_APB1SMENR_LPUART1SMEN_Msk    (0x1U << RCC_APB1SMENR_LPUART1SMEN_Pos) /*!< 0x00040000 */
#define RCC_APB1SMENR_LPUART1SMEN        RCC_APB1SMENR_LPUART1SMEN_Msk         /*!< LPUART1 clock enabled in sleep mode */
#define RCC_APB1SMENR_I2C1SMEN_Pos       (21U)                                 
#define RCC_APB1SMENR_I2C1SMEN_Msk       (0x1U << RCC_APB1SMENR_I2C1SMEN_Pos)  /*!< 0x00200000 */
#define RCC_APB1SMENR_I2C1SMEN           RCC_APB1SMENR_I2C1SMEN_Msk            /*!< I2C1 clock enabled in sleep mode */
#define RCC_APB1SMENR_I2C2SMEN_Pos       (22U)                                 
#define RCC_APB1SMENR_I2C2SMEN_Msk       (0x1U << RCC_APB1SMENR_I2C2SMEN_Pos)  /*!< 0x00400000 */
#define RCC_APB1SMENR_I2C2SMEN           RCC_APB1SMENR_I2C2SMEN_Msk            /*!< I2C2 clock enabled in sleep mode */
#define RCC_APB1SMENR_PWRSMEN_Pos        (28U)                                 
#define RCC_APB1SMENR_PWRSMEN_Msk        (0x1U << RCC_APB1SMENR_PWRSMEN_Pos)   /*!< 0x10000000 */
#define RCC_APB1SMENR_PWRSMEN            RCC_APB1SMENR_PWRSMEN_Msk             /*!< PWR clock enabled in sleep mode */
#define RCC_APB1SMENR_LPTIM1SMEN_Pos     (31U)                                 
#define RCC_APB1SMENR_LPTIM1SMEN_Msk     (0x1U << RCC_APB1SMENR_LPTIM1SMEN_Pos) /*!< 0x80000000 */
#define RCC_APB1SMENR_LPTIM1SMEN         RCC_APB1SMENR_LPTIM1SMEN_Msk          /*!< LPTIM1 clock enabled in sleep mode */

/*******************  Bit definition for RCC_CCIPR register  *******************/
/*!< USART1 Clock source selection */
#define RCC_CCIPR_USART1SEL_Pos          (0U)                                  
#define RCC_CCIPR_USART1SEL_Msk          (0x3U << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000003 */
#define RCC_CCIPR_USART1SEL              RCC_CCIPR_USART1SEL_Msk               /*!< USART1SEL[1:0] bits */
#define RCC_CCIPR_USART1SEL_0            (0x1U << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000001 */
#define RCC_CCIPR_USART1SEL_1            (0x2U << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000002 */

/*!< USART2 Clock source selection */
#define RCC_CCIPR_USART2SEL_Pos          (2U)                                  
#define RCC_CCIPR_USART2SEL_Msk          (0x3U << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x0000000C */
#define RCC_CCIPR_USART2SEL              RCC_CCIPR_USART2SEL_Msk               /*!< USART2SEL[1:0] bits */
#define RCC_CCIPR_USART2SEL_0            (0x1U << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x00000004 */
#define RCC_CCIPR_USART2SEL_1            (0x2U << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x00000008 */

/*!< LPUART1 Clock source selection */ 
#define RCC_CCIPR_LPUART1SEL_Pos         (10U)                                 
#define RCC_CCIPR_LPUART1SEL_Msk         (0x3U << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x00000C00 */
#define RCC_CCIPR_LPUART1SEL             RCC_CCIPR_LPUART1SEL_Msk              /*!< LPUART1SEL[1:0] bits */
#define RCC_CCIPR_LPUART1SEL_0           (0x1U << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x0000400 */
#define RCC_CCIPR_LPUART1SEL_1           (0x2U << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x0000800 */

/*!< I2C1 Clock source selection */
#define RCC_CCIPR_I2C1SEL_Pos            (12U)                                 
#define RCC_CCIPR_I2C1SEL_Msk            (0x3U << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00003000 */
#define RCC_CCIPR_I2C1SEL                RCC_CCIPR_I2C1SEL_Msk                 /*!< I2C1SEL [1:0] bits */
#define RCC_CCIPR_I2C1SEL_0              (0x1U << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00001000 */
#define RCC_CCIPR_I2C1SEL_1              (0x2U << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00002000 */


/*!< LPTIM1 Clock source selection */ 
#define RCC_CCIPR_LPTIM1SEL_Pos          (18U)                                 
#define RCC_CCIPR_LPTIM1SEL_Msk          (0x3U << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x000C0000 */
#define RCC_CCIPR_LPTIM1SEL              RCC_CCIPR_LPTIM1SEL_Msk               /*!< LPTIM1SEL [1:0] bits */
#define RCC_CCIPR_LPTIM1SEL_0            (0x1U << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x00040000 */
#define RCC_CCIPR_LPTIM1SEL_1            (0x2U << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x00080000 */

/*******************  Bit definition for RCC_CSR register  *******************/
#define RCC_CSR_LSION_Pos                (0U)                                  
#define RCC_CSR_LSION_Msk                (0x1U << RCC_CSR_LSION_Pos)           /*!< 0x00000001 */
#define RCC_CSR_LSION                    RCC_CSR_LSION_Msk                     /*!< Internal Low Speed oscillator enable */
#define RCC_CSR_LSIRDY_Pos               (1U)                                  
#define RCC_CSR_LSIRDY_Msk               (0x1U << RCC_CSR_LSIRDY_Pos)          /*!< 0x00000002 */
#define RCC_CSR_LSIRDY                   RCC_CSR_LSIRDY_Msk                    /*!< Internal Low Speed oscillator Ready */

#define RCC_CSR_LSEON_Pos                (8U)                                  
#define RCC_CSR_LSEON_Msk                (0x1U << RCC_CSR_LSEON_Pos)           /*!< 0x00000100 */
#define RCC_CSR_LSEON                    RCC_CSR_LSEON_Msk                     /*!< External Low Speed oscillator enable */
#define RCC_CSR_LSERDY_Pos               (9U)                                  
#define RCC_CSR_LSERDY_Msk               (0x1U << RCC_CSR_LSERDY_Pos)          /*!< 0x00000200 */
#define RCC_CSR_LSERDY                   RCC_CSR_LSERDY_Msk                    /*!< External Low Speed oscillator Ready */
#define RCC_CSR_LSEBYP_Pos               (10U)                                 
#define RCC_CSR_LSEBYP_Msk               (0x1U << RCC_CSR_LSEBYP_Pos)          /*!< 0x00000400 */
#define RCC_CSR_LSEBYP                   RCC_CSR_LSEBYP_Msk                    /*!< External Low Speed oscillator Bypass */
                                             
#define RCC_CSR_LSEDRV_Pos               (11U)                                 
#define RCC_CSR_LSEDRV_Msk               (0x3U << RCC_CSR_LSEDRV_Pos)          /*!< 0x00001800 */
#define RCC_CSR_LSEDRV                   RCC_CSR_LSEDRV_Msk                    /*!< LSEDRV[1:0] bits (LSE Osc. drive capability) */
#define RCC_CSR_LSEDRV_0                 (0x1U << RCC_CSR_LSEDRV_Pos)          /*!< 0x00000800 */
#define RCC_CSR_LSEDRV_1                 (0x2U << RCC_CSR_LSEDRV_Pos)          /*!< 0x00001000 */
                                             
#define RCC_CSR_LSECSSON_Pos             (13U)                                 
#define RCC_CSR_LSECSSON_Msk             (0x1U << RCC_CSR_LSECSSON_Pos)        /*!< 0x00002000 */
#define RCC_CSR_LSECSSON                 RCC_CSR_LSECSSON_Msk                  /*!< External Low Speed oscillator CSS Enable */
#define RCC_CSR_LSECSSD_Pos              (14U)                                 
#define RCC_CSR_LSECSSD_Msk              (0x1U << RCC_CSR_LSECSSD_Pos)         /*!< 0x00004000 */
#define RCC_CSR_LSECSSD                  RCC_CSR_LSECSSD_Msk                   /*!< External Low Speed oscillator CSS Detected */
                                             
/*!< RTC congiguration */                    
#define RCC_CSR_RTCSEL_Pos               (16U)                                 
#define RCC_CSR_RTCSEL_Msk               (0x3U << RCC_CSR_RTCSEL_Pos)          /*!< 0x00030000 */
#define RCC_CSR_RTCSEL                   RCC_CSR_RTCSEL_Msk                    /*!< RTCSEL[1:0] bits (RTC clock source selection) */
#define RCC_CSR_RTCSEL_0                 (0x1U << RCC_CSR_RTCSEL_Pos)          /*!< 0x00010000 */
#define RCC_CSR_RTCSEL_1                 (0x2U << RCC_CSR_RTCSEL_Pos)          /*!< 0x00020000 */
                                             
#define RCC_CSR_RTCSEL_NOCLOCK               (0x00000000U)                     /*!< No clock */
#define RCC_CSR_RTCSEL_LSE_Pos           (16U)                                 
#define RCC_CSR_RTCSEL_LSE_Msk           (0x1U << RCC_CSR_RTCSEL_LSE_Pos)      /*!< 0x00010000 */
#define RCC_CSR_RTCSEL_LSE               RCC_CSR_RTCSEL_LSE_Msk                /*!< LSE oscillator clock used as RTC clock */
#define RCC_CSR_RTCSEL_LSI_Pos           (17U)                                 
#define RCC_CSR_RTCSEL_LSI_Msk           (0x1U << RCC_CSR_RTCSEL_LSI_Pos)      /*!< 0x00020000 */
#define RCC_CSR_RTCSEL_LSI               RCC_CSR_RTCSEL_LSI_Msk                /*!< LSI oscillator clock used as RTC clock */
#define RCC_CSR_RTCSEL_HSE_Pos           (16U)                                 
#define RCC_CSR_RTCSEL_HSE_Msk           (0x3U << RCC_CSR_RTCSEL_HSE_Pos)      /*!< 0x00030000 */
#define RCC_CSR_RTCSEL_HSE               RCC_CSR_RTCSEL_HSE_Msk                /*!< HSE oscillator clock used as RTC clock */
                                             
#define RCC_CSR_RTCEN_Pos                (18U)                                 
#define RCC_CSR_RTCEN_Msk                (0x1U << RCC_CSR_RTCEN_Pos)           /*!< 0x00040000 */
#define RCC_CSR_RTCEN                    RCC_CSR_RTCEN_Msk                     /*!< RTC clock enable */
#define RCC_CSR_RTCRST_Pos               (19U)                                 
#define RCC_CSR_RTCRST_Msk               (0x1U << RCC_CSR_RTCRST_Pos)          /*!< 0x00080000 */
#define RCC_CSR_RTCRST                   RCC_CSR_RTCRST_Msk                    /*!< RTC software reset  */

#define RCC_CSR_RMVF_Pos                 (23U)                                 
#define RCC_CSR_RMVF_Msk                 (0x1U << RCC_CSR_RMVF_Pos)            /*!< 0x00800000 */
#define RCC_CSR_RMVF                     RCC_CSR_RMVF_Msk                      /*!< Remove reset flag */
#define RCC_CSR_FWRSTF_Pos               (24U)                                 
#define RCC_CSR_FWRSTF_Msk               (0x1U << RCC_CSR_FWRSTF_Pos)          /*!< 0x01000000 */
#define RCC_CSR_FWRSTF                   RCC_CSR_FWRSTF_Msk                    /*!< Mifare Firewall reset flag */
#define RCC_CSR_OBLRSTF_Pos              (25U)                                 
#define RCC_CSR_OBLRSTF_Msk              (0x1U << RCC_CSR_OBLRSTF_Pos)         /*!< 0x02000000 */
#define RCC_CSR_OBLRSTF                  RCC_CSR_OBLRSTF_Msk                   /*!< OBL reset flag */
#define RCC_CSR_PINRSTF_Pos              (26U)                                 
#define RCC_CSR_PINRSTF_Msk              (0x1U << RCC_CSR_PINRSTF_Pos)         /*!< 0x04000000 */
#define RCC_CSR_PINRSTF                  RCC_CSR_PINRSTF_Msk                   /*!< PIN reset flag */
#define RCC_CSR_PORRSTF_Pos              (27U)                                 
#define RCC_CSR_PORRSTF_Msk              (0x1U << RCC_CSR_PORRSTF_Pos)         /*!< 0x08000000 */
#define RCC_CSR_PORRSTF                  RCC_CSR_PORRSTF_Msk                   /*!< POR/PDR reset flag */
#define RCC_CSR_SFTRSTF_Pos              (28U)                                 
#define RCC_CSR_SFTRSTF_Msk              (0x1U << RCC_CSR_SFTRSTF_Pos)         /*!< 0x10000000 */
#define RCC_CSR_SFTRSTF                  RCC_CSR_SFTRSTF_Msk                   /*!< Software Reset flag */
#define RCC_CSR_IWDGRSTF_Pos             (29U)                                 
#define RCC_CSR_IWDGRSTF_Msk             (0x1U << RCC_CSR_IWDGRSTF_Pos)        /*!< 0x20000000 */
#define RCC_CSR_IWDGRSTF                 RCC_CSR_IWDGRSTF_Msk                  /*!< Independent Watchdog reset flag */
#define RCC_CSR_WWDGRSTF_Pos             (30U)                                 
#define RCC_CSR_WWDGRSTF_Msk             (0x1U << RCC_CSR_WWDGRSTF_Pos)        /*!< 0x40000000 */
#define RCC_CSR_WWDGRSTF                 RCC_CSR_WWDGRSTF_Msk                  /*!< Window watchdog reset flag */
#define RCC_CSR_LPWRRSTF_Pos             (31U)                                 
#define RCC_CSR_LPWRRSTF_Msk             (0x1U << RCC_CSR_LPWRRSTF_Pos)        /*!< 0x80000000 */
#define RCC_CSR_LPWRRSTF                 RCC_CSR_LPWRRSTF_Msk                  /*!< Low-Power reset flag */

/* Reference defines */
#define RCC_CSR_OBL                         RCC_CSR_OBLRSTF               /*!< OBL reset flag */


/******************************************************************************/
/*                                                                            */
/*                           Real-Time Clock (RTC)                            */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions
*/
#define RTC_TAMPER1_SUPPORT
#define RTC_TAMPER2_SUPPORT
#define RTC_WAKEUP_SUPPORT
#define RTC_BACKUP_SUPPORT

/********************  Bits definition for RTC_TR register  *******************/
#define RTC_TR_PM_Pos                  (22U)                                   
#define RTC_TR_PM_Msk                  (0x1U << RTC_TR_PM_Pos)                 /*!< 0x00400000 */
#define RTC_TR_PM                      RTC_TR_PM_Msk                           /*!<  */
#define RTC_TR_HT_Pos                  (20U)                                   
#define RTC_TR_HT_Msk                  (0x3U << RTC_TR_HT_Pos)                 /*!< 0x00300000 */
#define RTC_TR_HT                      RTC_TR_HT_Msk                           /*!<  */
#define RTC_TR_HT_0                    (0x1U << RTC_TR_HT_Pos)                 /*!< 0x00100000 */
#define RTC_TR_HT_1                    (0x2U << RTC_TR_HT_Pos)                 /*!< 0x00200000 */
#define RTC_TR_HU_Pos                  (16U)                                   
#define RTC_TR_HU_Msk                  (0xFU << RTC_TR_HU_Pos)                 /*!< 0x000F0000 */
#define RTC_TR_HU                      RTC_TR_HU_Msk                           /*!<  */
#define RTC_TR_HU_0                    (0x1U << RTC_TR_HU_Pos)                 /*!< 0x00010000 */
#define RTC_TR_HU_1                    (0x2U << RTC_TR_HU_Pos)                 /*!< 0x00020000 */
#define RTC_TR_HU_2                    (0x4U << RTC_TR_HU_Pos)                 /*!< 0x00040000 */
#define RTC_TR_HU_3                    (0x8U << RTC_TR_HU_Pos)                 /*!< 0x00080000 */
#define RTC_TR_MNT_Pos                 (12U)                                   
#define RTC_TR_MNT_Msk                 (0x7U << RTC_TR_MNT_Pos)                /*!< 0x00007000 */
#define RTC_TR_MNT                     RTC_TR_MNT_Msk                          /*!<  */
#define RTC_TR_MNT_0                   (0x1U << RTC_TR_MNT_Pos)                /*!< 0x00001000 */
#define RTC_TR_MNT_1                   (0x2U << RTC_TR_MNT_Pos)                /*!< 0x00002000 */
#define RTC_TR_MNT_2                   (0x4U << RTC_TR_MNT_Pos)                /*!< 0x00004000 */
#define RTC_TR_MNU_Pos                 (8U)                                    
#define RTC_TR_MNU_Msk                 (0xFU << RTC_TR_MNU_Pos)                /*!< 0x00000F00 */
#define RTC_TR_MNU                     RTC_TR_MNU_Msk                          /*!<  */
#define RTC_TR_MNU_0                   (0x1U << RTC_TR_MNU_Pos)                /*!< 0x00000100 */
#define RTC_TR_MNU_1                   (0x2U << RTC_TR_MNU_Pos)                /*!< 0x00000200 */
#define RTC_TR_MNU_2                   (0x4U << RTC_TR_MNU_Pos)                /*!< 0x00000400 */
#define RTC_TR_MNU_3                   (0x8U << RTC_TR_MNU_Pos)                /*!< 0x00000800 */
#define RTC_TR_ST_Pos                  (4U)                                    
#define RTC_TR_ST_Msk                  (0x7U << RTC_TR_ST_Pos)                 /*!< 0x00000070 */
#define RTC_TR_ST                      RTC_TR_ST_Msk                           /*!<  */
#define RTC_TR_ST_0                    (0x1U << RTC_TR_ST_Pos)                 /*!< 0x00000010 */
#define RTC_TR_ST_1                    (0x2U << RTC_TR_ST_Pos)                 /*!< 0x00000020 */
#define RTC_TR_ST_2                    (0x4U << RTC_TR_ST_Pos)                 /*!< 0x00000040 */
#define RTC_TR_SU_Pos                  (0U)                                    
#define RTC_TR_SU_Msk                  (0xFU << RTC_TR_SU_Pos)                 /*!< 0x0000000F */
#define RTC_TR_SU                      RTC_TR_SU_Msk                           /*!<  */
#define RTC_TR_SU_0                    (0x1U << RTC_TR_SU_Pos)                 /*!< 0x00000001 */
#define RTC_TR_SU_1                    (0x2U << RTC_TR_SU_Pos)                 /*!< 0x00000002 */
#define RTC_TR_SU_2                    (0x4U << RTC_TR_SU_Pos)                 /*!< 0x00000004 */
#define RTC_TR_SU_3                    (0x8U << RTC_TR_SU_Pos)                 /*!< 0x00000008 */

/********************  Bits definition for RTC_DR register  *******************/
#define RTC_DR_YT_Pos                  (20U)                                   
#define RTC_DR_YT_Msk                  (0xFU << RTC_DR_YT_Pos)                 /*!< 0x00F00000 */
#define RTC_DR_YT                      RTC_DR_YT_Msk                           /*!<  */
#define RTC_DR_YT_0                    (0x1U << RTC_DR_YT_Pos)                 /*!< 0x00100000 */
#define RTC_DR_YT_1                    (0x2U << RTC_DR_YT_Pos)                 /*!< 0x00200000 */
#define RTC_DR_YT_2                    (0x4U << RTC_DR_YT_Pos)                 /*!< 0x00400000 */
#define RTC_DR_YT_3                    (0x8U << RTC_DR_YT_Pos)                 /*!< 0x00800000 */
#define RTC_DR_YU_Pos                  (16U)                                   
#define RTC_DR_YU_Msk                  (0xFU << RTC_DR_YU_Pos)                 /*!< 0x000F0000 */
#define RTC_DR_YU                      RTC_DR_YU_Msk                           /*!<  */
#define RTC_DR_YU_0                    (0x1U << RTC_DR_YU_Pos)                 /*!< 0x00010000 */
#define RTC_DR_YU_1                    (0x2U << RTC_DR_YU_Pos)                 /*!< 0x00020000 */
#define RTC_DR_YU_2                    (0x4U << RTC_DR_YU_Pos)                 /*!< 0x00040000 */
#define RTC_DR_YU_3                    (0x8U << RTC_DR_YU_Pos)                 /*!< 0x00080000 */
#define RTC_DR_WDU_Pos                 (13U)                                   
#define RTC_DR_WDU_Msk                 (0x7U << RTC_DR_WDU_Pos)                /*!< 0x0000E000 */
#define RTC_DR_WDU                     RTC_DR_WDU_Msk                          /*!<  */
#define RTC_DR_WDU_0                   (0x1U << RTC_DR_WDU_Pos)                /*!< 0x00002000 */
#define RTC_DR_WDU_1                   (0x2U << RTC_DR_WDU_Pos)                /*!< 0x00004000 */
#define RTC_DR_WDU_2                   (0x4U << RTC_DR_WDU_Pos)                /*!< 0x00008000 */
#define RTC_DR_MT_Pos                  (12U)                                   
#define RTC_DR_MT_Msk                  (0x1U << RTC_DR_MT_Pos)                 /*!< 0x00001000 */
#define RTC_DR_MT                      RTC_DR_MT_Msk                           /*!<  */
#define RTC_DR_MU_Pos                  (8U)                                    
#define RTC_DR_MU_Msk                  (0xFU << RTC_DR_MU_Pos)                 /*!< 0x00000F00 */
#define RTC_DR_MU                      RTC_DR_MU_Msk                           /*!<  */
#define RTC_DR_MU_0                    (0x1U << RTC_DR_MU_Pos)                 /*!< 0x00000100 */
#define RTC_DR_MU_1                    (0x2U << RTC_DR_MU_Pos)                 /*!< 0x00000200 */
#define RTC_DR_MU_2                    (0x4U << RTC_DR_MU_Pos)                 /*!< 0x00000400 */
#define RTC_DR_MU_3                    (0x8U << RTC_DR_MU_Pos)                 /*!< 0x00000800 */
#define RTC_DR_DT_Pos                  (4U)                                    
#define RTC_DR_DT_Msk                  (0x3U << RTC_DR_DT_Pos)                 /*!< 0x00000030 */
#define RTC_DR_DT                      RTC_DR_DT_Msk                           /*!<  */
#define RTC_DR_DT_0                    (0x1U << RTC_DR_DT_Pos)                 /*!< 0x00000010 */
#define RTC_DR_DT_1                    (0x2U << RTC_DR_DT_Pos)                 /*!< 0x00000020 */
#define RTC_DR_DU_Pos                  (0U)                                    
#define RTC_DR_DU_Msk                  (0xFU << RTC_DR_DU_Pos)                 /*!< 0x0000000F */
#define RTC_DR_DU                      RTC_DR_DU_Msk                           /*!<  */
#define RTC_DR_DU_0                    (0x1U << RTC_DR_DU_Pos)                 /*!< 0x00000001 */
#define RTC_DR_DU_1                    (0x2U << RTC_DR_DU_Pos)                 /*!< 0x00000002 */
#define RTC_DR_DU_2                    (0x4U << RTC_DR_DU_Pos)                 /*!< 0x00000004 */
#define RTC_DR_DU_3                    (0x8U << RTC_DR_DU_Pos)                 /*!< 0x00000008 */

/********************  Bits definition for RTC_CR register  *******************/
#define RTC_CR_COE_Pos                 (23U)                                   
#define RTC_CR_COE_Msk                 (0x1U << RTC_CR_COE_Pos)                /*!< 0x00800000 */
#define RTC_CR_COE                     RTC_CR_COE_Msk                          /*!<  */
#define RTC_CR_OSEL_Pos                (21U)                                   
#define RTC_CR_OSEL_Msk                (0x3U << RTC_CR_OSEL_Pos)               /*!< 0x00600000 */
#define RTC_CR_OSEL                    RTC_CR_OSEL_Msk                         /*!<  */
#define RTC_CR_OSEL_0                  (0x1U << RTC_CR_OSEL_Pos)               /*!< 0x00200000 */
#define RTC_CR_OSEL_1                  (0x2U << RTC_CR_OSEL_Pos)               /*!< 0x00400000 */
#define RTC_CR_POL_Pos                 (20U)                                   
#define RTC_CR_POL_Msk                 (0x1U << RTC_CR_POL_Pos)                /*!< 0x00100000 */
#define RTC_CR_POL                     RTC_CR_POL_Msk                          /*!<  */
#define RTC_CR_COSEL_Pos               (19U)                                   
#define RTC_CR_COSEL_Msk               (0x1U << RTC_CR_COSEL_Pos)              /*!< 0x00080000 */
#define RTC_CR_COSEL                   RTC_CR_COSEL_Msk                        /*!<  */
#define RTC_CR_BCK_Pos                 (18U)                                   
#define RTC_CR_BCK_Msk                 (0x1U << RTC_CR_BCK_Pos)                /*!< 0x00040000 */
#define RTC_CR_BCK                     RTC_CR_BCK_Msk                          /*!<  */
#define RTC_CR_SUB1H_Pos               (17U)                                   
#define RTC_CR_SUB1H_Msk               (0x1U << RTC_CR_SUB1H_Pos)              /*!< 0x00020000 */
#define RTC_CR_SUB1H                   RTC_CR_SUB1H_Msk                        /*!<  */
#define RTC_CR_ADD1H_Pos               (16U)                                   
#define RTC_CR_ADD1H_Msk               (0x1U << RTC_CR_ADD1H_Pos)              /*!< 0x00010000 */
#define RTC_CR_ADD1H                   RTC_CR_ADD1H_Msk                        /*!<  */
#define RTC_CR_TSIE_Pos                (15U)                                   
#define RTC_CR_TSIE_Msk                (0x1U << RTC_CR_TSIE_Pos)               /*!< 0x00008000 */
#define RTC_CR_TSIE                    RTC_CR_TSIE_Msk                         /*!<  */
#define RTC_CR_WUTIE_Pos               (14U)                                   
#define RTC_CR_WUTIE_Msk               (0x1U << RTC_CR_WUTIE_Pos)              /*!< 0x00004000 */
#define RTC_CR_WUTIE                   RTC_CR_WUTIE_Msk                        /*!<  */
#define RTC_CR_ALRBIE_Pos              (13U)                                   
#define RTC_CR_ALRBIE_Msk              (0x1U << RTC_CR_ALRBIE_Pos)             /*!< 0x00002000 */
#define RTC_CR_ALRBIE                  RTC_CR_ALRBIE_Msk                       /*!<  */
#define RTC_CR_ALRAIE_Pos              (12U)                                   
#define RTC_CR_ALRAIE_Msk              (0x1U << RTC_CR_ALRAIE_Pos)             /*!< 0x00001000 */
#define RTC_CR_ALRAIE                  RTC_CR_ALRAIE_Msk                       /*!<  */
#define RTC_CR_TSE_Pos                 (11U)                                   
#define RTC_CR_TSE_Msk                 (0x1U << RTC_CR_TSE_Pos)                /*!< 0x00000800 */
#define RTC_CR_TSE                     RTC_CR_TSE_Msk                          /*!<  */
#define RTC_CR_WUTE_Pos                (10U)                                   
#define RTC_CR_WUTE_Msk                (0x1U << RTC_CR_WUTE_Pos)               /*!< 0x00000400 */
#define RTC_CR_WUTE                    RTC_CR_WUTE_Msk                         /*!<  */
#define RTC_CR_ALRBE_Pos               (9U)                                    
#define RTC_CR_ALRBE_Msk               (0x1U << RTC_CR_ALRBE_Pos)              /*!< 0x00000200 */
#define RTC_CR_ALRBE                   RTC_CR_ALRBE_Msk                        /*!<  */
#define RTC_CR_ALRAE_Pos               (8U)                                    
#define RTC_CR_ALRAE_Msk               (0x1U << RTC_CR_ALRAE_Pos)              /*!< 0x00000100 */
#define RTC_CR_ALRAE                   RTC_CR_ALRAE_Msk                        /*!<  */
#define RTC_CR_FMT_Pos                 (6U)                                    
#define RTC_CR_FMT_Msk                 (0x1U << RTC_CR_FMT_Pos)                /*!< 0x00000040 */
#define RTC_CR_FMT                     RTC_CR_FMT_Msk                          /*!<  */
#define RTC_CR_BYPSHAD_Pos             (5U)                                    
#define RTC_CR_BYPSHAD_Msk             (0x1U << RTC_CR_BYPSHAD_Pos)            /*!< 0x00000020 */
#define RTC_CR_BYPSHAD                 RTC_CR_BYPSHAD_Msk                      /*!<  */
#define RTC_CR_REFCKON_Pos             (4U)                                    
#define RTC_CR_REFCKON_Msk             (0x1U << RTC_CR_REFCKON_Pos)            /*!< 0x00000010 */
#define RTC_CR_REFCKON                 RTC_CR_REFCKON_Msk                      /*!<  */
#define RTC_CR_TSEDGE_Pos              (3U)                                    
#define RTC_CR_TSEDGE_Msk              (0x1U << RTC_CR_TSEDGE_Pos)             /*!< 0x00000008 */
#define RTC_CR_TSEDGE                  RTC_CR_TSEDGE_Msk                       /*!<  */
#define RTC_CR_WUCKSEL_Pos             (0U)                                    
#define RTC_CR_WUCKSEL_Msk             (0x7U << RTC_CR_WUCKSEL_Pos)            /*!< 0x00000007 */
#define RTC_CR_WUCKSEL                 RTC_CR_WUCKSEL_Msk                      /*!<  */
#define RTC_CR_WUCKSEL_0               (0x1U << RTC_CR_WUCKSEL_Pos)            /*!< 0x00000001 */
#define RTC_CR_WUCKSEL_1               (0x2U << RTC_CR_WUCKSEL_Pos)            /*!< 0x00000002 */
#define RTC_CR_WUCKSEL_2               (0x4U << RTC_CR_WUCKSEL_Pos)            /*!< 0x00000004 */

/********************  Bits definition for RTC_ISR register  ******************/
#define RTC_ISR_RECALPF_Pos            (16U)                                   
#define RTC_ISR_RECALPF_Msk            (0x1U << RTC_ISR_RECALPF_Pos)           /*!< 0x00010000 */
#define RTC_ISR_RECALPF                RTC_ISR_RECALPF_Msk                     /*!<  */
#define RTC_ISR_TAMP2F_Pos             (14U)                                   
#define RTC_ISR_TAMP2F_Msk             (0x1U << RTC_ISR_TAMP2F_Pos)            /*!< 0x00004000 */
#define RTC_ISR_TAMP2F                 RTC_ISR_TAMP2F_Msk                      /*!<  */
#define RTC_ISR_TAMP1F_Pos             (13U)                                   
#define RTC_ISR_TAMP1F_Msk             (0x1U << RTC_ISR_TAMP1F_Pos)            /*!< 0x00002000 */
#define RTC_ISR_TAMP1F                 RTC_ISR_TAMP1F_Msk                      /*!<  */
#define RTC_ISR_TSOVF_Pos              (12U)                                   
#define RTC_ISR_TSOVF_Msk              (0x1U << RTC_ISR_TSOVF_Pos)             /*!< 0x00001000 */
#define RTC_ISR_TSOVF                  RTC_ISR_TSOVF_Msk                       /*!<  */
#define RTC_ISR_TSF_Pos                (11U)                                   
#define RTC_ISR_TSF_Msk                (0x1U << RTC_ISR_TSF_Pos)               /*!< 0x00000800 */
#define RTC_ISR_TSF                    RTC_ISR_TSF_Msk                         /*!<  */
#define RTC_ISR_WUTF_Pos               (10U)                                   
#define RTC_ISR_WUTF_Msk               (0x1U << RTC_ISR_WUTF_Pos)              /*!< 0x00000400 */
#define RTC_ISR_WUTF                   RTC_ISR_WUTF_Msk                        /*!<  */
#define RTC_ISR_ALRBF_Pos              (9U)                                    
#define RTC_ISR_ALRBF_Msk              (0x1U << RTC_ISR_ALRBF_Pos)             /*!< 0x00000200 */
#define RTC_ISR_ALRBF                  RTC_ISR_ALRBF_Msk                       /*!<  */
#define RTC_ISR_ALRAF_Pos              (8U)                                    
#define RTC_ISR_ALRAF_Msk              (0x1U << RTC_ISR_ALRAF_Pos)             /*!< 0x00000100 */
#define RTC_ISR_ALRAF                  RTC_ISR_ALRAF_Msk                       /*!<  */
#define RTC_ISR_INIT_Pos               (7U)                                    
#define RTC_ISR_INIT_Msk               (0x1U << RTC_ISR_INIT_Pos)              /*!< 0x00000080 */
#define RTC_ISR_INIT                   RTC_ISR_INIT_Msk                        /*!<  */
#define RTC_ISR_INITF_Pos              (6U)                                    
#define RTC_ISR_INITF_Msk              (0x1U << RTC_ISR_INITF_Pos)             /*!< 0x00000040 */
#define RTC_ISR_INITF                  RTC_ISR_INITF_Msk                       /*!<  */
#define RTC_ISR_RSF_Pos                (5U)                                    
#define RTC_ISR_RSF_Msk                (0x1U << RTC_ISR_RSF_Pos)               /*!< 0x00000020 */
#define RTC_ISR_RSF                    RTC_ISR_RSF_Msk                         /*!<  */
#define RTC_ISR_INITS_Pos              (4U)                                    
#define RTC_ISR_INITS_Msk              (0x1U << RTC_ISR_INITS_Pos)             /*!< 0x00000010 */
#define RTC_ISR_INITS                  RTC_ISR_INITS_Msk                       /*!<  */
#define RTC_ISR_SHPF_Pos               (3U)                                    
#define RTC_ISR_SHPF_Msk               (0x1U << RTC_ISR_SHPF_Pos)              /*!< 0x00000008 */
#define RTC_ISR_SHPF                   RTC_ISR_SHPF_Msk                        /*!<  */
#define RTC_ISR_WUTWF_Pos              (2U)                                    
#define RTC_ISR_WUTWF_Msk              (0x1U << RTC_ISR_WUTWF_Pos)             /*!< 0x00000004 */
#define RTC_ISR_WUTWF                  RTC_ISR_WUTWF_Msk                       /*!<  */
#define RTC_ISR_ALRBWF_Pos             (1U)                                    
#define RTC_ISR_ALRBWF_Msk             (0x1U << RTC_ISR_ALRBWF_Pos)            /*!< 0x00000002 */
#define RTC_ISR_ALRBWF                 RTC_ISR_ALRBWF_Msk                      /*!<  */
#define RTC_ISR_ALRAWF_Pos             (0U)                                    
#define RTC_ISR_ALRAWF_Msk             (0x1U << RTC_ISR_ALRAWF_Pos)            /*!< 0x00000001 */
#define RTC_ISR_ALRAWF                 RTC_ISR_ALRAWF_Msk                      /*!<  */

/********************  Bits definition for RTC_PRER register  *****************/
#define RTC_PRER_PREDIV_A_Pos          (16U)                                   
#define RTC_PRER_PREDIV_A_Msk          (0x7FU << RTC_PRER_PREDIV_A_Pos)        /*!< 0x007F0000 */
#define RTC_PRER_PREDIV_A              RTC_PRER_PREDIV_A_Msk                   /*!<  */
#define RTC_PRER_PREDIV_S_Pos          (0U)                                    
#define RTC_PRER_PREDIV_S_Msk          (0x7FFFU << RTC_PRER_PREDIV_S_Pos)      /*!< 0x00007FFF */
#define RTC_PRER_PREDIV_S              RTC_PRER_PREDIV_S_Msk                   /*!<  */

/********************  Bits definition for RTC_WUTR register  *****************/
#define RTC_WUTR_WUT_Pos               (0U)                                    
#define RTC_WUTR_WUT_Msk               (0xFFFFU << RTC_WUTR_WUT_Pos)           /*!< 0x0000FFFF */
#define RTC_WUTR_WUT                   RTC_WUTR_WUT_Msk                        

/********************  Bits definition for RTC_ALRMAR register  ***************/
#define RTC_ALRMAR_MSK4_Pos            (31U)                                   
#define RTC_ALRMAR_MSK4_Msk            (0x1U << RTC_ALRMAR_MSK4_Pos)           /*!< 0x80000000 */
#define RTC_ALRMAR_MSK4                RTC_ALRMAR_MSK4_Msk                     /*!<  */
#define RTC_ALRMAR_WDSEL_Pos           (30U)                                   
#define RTC_ALRMAR_WDSEL_Msk           (0x1U << RTC_ALRMAR_WDSEL_Pos)          /*!< 0x40000000 */
#define RTC_ALRMAR_WDSEL               RTC_ALRMAR_WDSEL_Msk                    /*!<  */
#define RTC_ALRMAR_DT_Pos              (28U)                                   
#define RTC_ALRMAR_DT_Msk              (0x3U << RTC_ALRMAR_DT_Pos)             /*!< 0x30000000 */
#define RTC_ALRMAR_DT                  RTC_ALRMAR_DT_Msk                       /*!<  */
#define RTC_ALRMAR_DT_0                (0x1U << RTC_ALRMAR_DT_Pos)             /*!< 0x10000000 */
#define RTC_ALRMAR_DT_1                (0x2U << RTC_ALRMAR_DT_Pos)             /*!< 0x20000000 */
#define RTC_ALRMAR_DU_Pos              (24U)                                   
#define RTC_ALRMAR_DU_Msk              (0xFU << RTC_ALRMAR_DU_Pos)             /*!< 0x0F000000 */
#define RTC_ALRMAR_DU                  RTC_ALRMAR_DU_Msk                       /*!<  */
#define RTC_ALRMAR_DU_0                (0x1U << RTC_ALRMAR_DU_Pos)             /*!< 0x01000000 */
#define RTC_ALRMAR_DU_1                (0x2U << RTC_ALRMAR_DU_Pos)             /*!< 0x02000000 */
#define RTC_ALRMAR_DU_2                (0x4U << RTC_ALRMAR_DU_Pos)             /*!< 0x04000000 */
#define RTC_ALRMAR_DU_3                (0x8U << RTC_ALRMAR_DU_Pos)             /*!< 0x08000000 */
#define RTC_ALRMAR_MSK3_Pos            (23U)                                   
#define RTC_ALRMAR_MSK3_Msk            (0x1U << RTC_ALRMAR_MSK3_Pos)           /*!< 0x00800000 */
#define RTC_ALRMAR_MSK3                RTC_ALRMAR_MSK3_Msk                     /*!<  */
#define RTC_ALRMAR_PM_Pos              (22U)                                   
#define RTC_ALRMAR_PM_Msk              (0x1U << RTC_ALRMAR_PM_Pos)             /*!< 0x00400000 */
#define RTC_ALRMAR_PM                  RTC_ALRMAR_PM_Msk                       /*!<  */
#define RTC_ALRMAR_HT_Pos              (20U)                                   
#define RTC_ALRMAR_HT_Msk              (0x3U << RTC_ALRMAR_HT_Pos)             /*!< 0x00300000 */
#define RTC_ALRMAR_HT                  RTC_ALRMAR_HT_Msk                       /*!<  */
#define RTC_ALRMAR_HT_0                (0x1U << RTC_ALRMAR_HT_Pos)             /*!< 0x00100000 */
#define RTC_ALRMAR_HT_1                (0x2U << RTC_ALRMAR_HT_Pos)             /*!< 0x00200000 */
#define RTC_ALRMAR_HU_Pos              (16U)                                   
#define RTC_ALRMAR_HU_Msk              (0xFU << RTC_ALRMAR_HU_Pos)             /*!< 0x000F0000 */
#define RTC_ALRMAR_HU                  RTC_ALRMAR_HU_Msk                       /*!<  */
#define RTC_ALRMAR_HU_0                (0x1U << RTC_ALRMAR_HU_Pos)             /*!< 0x00010000 */
#define RTC_ALRMAR_HU_1                (0x2U << RTC_ALRMAR_HU_Pos)             /*!< 0x00020000 */
#define RTC_ALRMAR_HU_2                (0x4U << RTC_ALRMAR_HU_Pos)             /*!< 0x00040000 */
#define RTC_ALRMAR_HU_3                (0x8U << RTC_ALRMAR_HU_Pos)             /*!< 0x00080000 */
#define RTC_ALRMAR_MSK2_Pos            (15U)                                   
#define RTC_ALRMAR_MSK2_Msk            (0x1U << RTC_ALRMAR_MSK2_Pos)           /*!< 0x00008000 */
#define RTC_ALRMAR_MSK2                RTC_ALRMAR_MSK2_Msk                     /*!<  */
#define RTC_ALRMAR_MNT_Pos             (12U)                                   
#define RTC_ALRMAR_MNT_Msk             (0x7U << RTC_ALRMAR_MNT_Pos)            /*!< 0x00007000 */
#define RTC_ALRMAR_MNT                 RTC_ALRMAR_MNT_Msk                      /*!<  */
#define RTC_ALRMAR_MNT_0               (0x1U << RTC_ALRMAR_MNT_Pos)            /*!< 0x00001000 */
#define RTC_ALRMAR_MNT_1               (0x2U << RTC_ALRMAR_MNT_Pos)            /*!< 0x00002000 */
#define RTC_ALRMAR_MNT_2               (0x4U << RTC_ALRMAR_MNT_Pos)            /*!< 0x00004000 */
#define RTC_ALRMAR_MNU_Pos             (8U)                                    
#define RTC_ALRMAR_MNU_Msk             (0xFU << RTC_ALRMAR_MNU_Pos)            /*!< 0x00000F00 */
#define RTC_ALRMAR_MNU                 RTC_ALRMAR_MNU_Msk                      /*!<  */
#define RTC_ALRMAR_MNU_0               (0x1U << RTC_ALRMAR_MNU_Pos)            /*!< 0x00000100 */
#define RTC_ALRMAR_MNU_1               (0x2U << RTC_ALRMAR_MNU_Pos)            /*!< 0x00000200 */
#define RTC_ALRMAR_MNU_2               (0x4U << RTC_ALRMAR_MNU_Pos)            /*!< 0x00000400 */
#define RTC_ALRMAR_MNU_3               (0x8U << RTC_ALRMAR_MNU_Pos)            /*!< 0x00000800 */
#define RTC_ALRMAR_MSK1_Pos            (7U)                                    
#define RTC_ALRMAR_MSK1_Msk            (0x1U << RTC_ALRMAR_MSK1_Pos)           /*!< 0x00000080 */
#define RTC_ALRMAR_MSK1                RTC_ALRMAR_MSK1_Msk                     /*!<  */
#define RTC_ALRMAR_ST_Pos              (4U)                                    
#define RTC_ALRMAR_ST_Msk              (0x7U << RTC_ALRMAR_ST_Pos)             /*!< 0x00000070 */
#define RTC_ALRMAR_ST                  RTC_ALRMAR_ST_Msk                       /*!<  */
#define RTC_ALRMAR_ST_0                (0x1U << RTC_ALRMAR_ST_Pos)             /*!< 0x00000010 */
#define RTC_ALRMAR_ST_1                (0x2U << RTC_ALRMAR_ST_Pos)             /*!< 0x00000020 */
#define RTC_ALRMAR_ST_2                (0x4U << RTC_ALRMAR_ST_Pos)             /*!< 0x00000040 */
#define RTC_ALRMAR_SU_Pos              (0U)                                    
#define RTC_ALRMAR_SU_Msk              (0xFU << RTC_ALRMAR_SU_Pos)             /*!< 0x0000000F */
#define RTC_ALRMAR_SU                  RTC_ALRMAR_SU_Msk                       /*!<  */
#define RTC_ALRMAR_SU_0                (0x1U << RTC_ALRMAR_SU_Pos)             /*!< 0x00000001 */
#define RTC_ALRMAR_SU_1                (0x2U << RTC_ALRMAR_SU_Pos)             /*!< 0x00000002 */
#define RTC_ALRMAR_SU_2                (0x4U << RTC_ALRMAR_SU_Pos)             /*!< 0x00000004 */
#define RTC_ALRMAR_SU_3                (0x8U << RTC_ALRMAR_SU_Pos)             /*!< 0x00000008 */

/********************  Bits definition for RTC_ALRMBR register  ***************/
#define RTC_ALRMBR_MSK4_Pos            (31U)                                   
#define RTC_ALRMBR_MSK4_Msk            (0x1U << RTC_ALRMBR_MSK4_Pos)           /*!< 0x80000000 */
#define RTC_ALRMBR_MSK4                RTC_ALRMBR_MSK4_Msk                     /*!<  */
#define RTC_ALRMBR_WDSEL_Pos           (30U)                                   
#define RTC_ALRMBR_WDSEL_Msk           (0x1U << RTC_ALRMBR_WDSEL_Pos)          /*!< 0x40000000 */
#define RTC_ALRMBR_WDSEL               RTC_ALRMBR_WDSEL_Msk                    /*!<  */
#define RTC_ALRMBR_DT_Pos              (28U)                                   
#define RTC_ALRMBR_DT_Msk              (0x3U << RTC_ALRMBR_DT_Pos)             /*!< 0x30000000 */
#define RTC_ALRMBR_DT                  RTC_ALRMBR_DT_Msk                       /*!<  */
#define RTC_ALRMBR_DT_0                (0x1U << RTC_ALRMBR_DT_Pos)             /*!< 0x10000000 */
#define RTC_ALRMBR_DT_1                (0x2U << RTC_ALRMBR_DT_Pos)             /*!< 0x20000000 */
#define RTC_ALRMBR_DU_Pos              (24U)                                   
#define RTC_ALRMBR_DU_Msk              (0xFU << RTC_ALRMBR_DU_Pos)             /*!< 0x0F000000 */
#define RTC_ALRMBR_DU                  RTC_ALRMBR_DU_Msk                       /*!<  */
#define RTC_ALRMBR_DU_0                (0x1U << RTC_ALRMBR_DU_Pos)             /*!< 0x01000000 */
#define RTC_ALRMBR_DU_1                (0x2U << RTC_ALRMBR_DU_Pos)             /*!< 0x02000000 */
#define RTC_ALRMBR_DU_2                (0x4U << RTC_ALRMBR_DU_Pos)             /*!< 0x04000000 */
#define RTC_ALRMBR_DU_3                (0x8U << RTC_ALRMBR_DU_Pos)             /*!< 0x08000000 */
#define RTC_ALRMBR_MSK3_Pos            (23U)                                   
#define RTC_ALRMBR_MSK3_Msk            (0x1U << RTC_ALRMBR_MSK3_Pos)           /*!< 0x00800000 */
#define RTC_ALRMBR_MSK3                RTC_ALRMBR_MSK3_Msk                     /*!<  */
#define RTC_ALRMBR_PM_Pos              (22U)                                   
#define RTC_ALRMBR_PM_Msk              (0x1U << RTC_ALRMBR_PM_Pos)             /*!< 0x00400000 */
#define RTC_ALRMBR_PM                  RTC_ALRMBR_PM_Msk                       /*!<  */
#define RTC_ALRMBR_HT_Pos              (20U)                                   
#define RTC_ALRMBR_HT_Msk              (0x3U << RTC_ALRMBR_HT_Pos)             /*!< 0x00300000 */
#define RTC_ALRMBR_HT                  RTC_ALRMBR_HT_Msk                       /*!<  */
#define RTC_ALRMBR_HT_0                (0x1U << RTC_ALRMBR_HT_Pos)             /*!< 0x00100000 */
#define RTC_ALRMBR_HT_1                (0x2U << RTC_ALRMBR_HT_Pos)             /*!< 0x00200000 */
#define RTC_ALRMBR_HU_Pos              (16U)                                   
#define RTC_ALRMBR_HU_Msk              (0xFU << RTC_ALRMBR_HU_Pos)             /*!< 0x000F0000 */
#define RTC_ALRMBR_HU                  RTC_ALRMBR_HU_Msk                       /*!<  */
#define RTC_ALRMBR_HU_0                (0x1U << RTC_ALRMBR_HU_Pos)             /*!< 0x00010000 */
#define RTC_ALRMBR_HU_1                (0x2U << RTC_ALRMBR_HU_Pos)             /*!< 0x00020000 */
#define RTC_ALRMBR_HU_2                (0x4U << RTC_ALRMBR_HU_Pos)             /*!< 0x00040000 */
#define RTC_ALRMBR_HU_3                (0x8U << RTC_ALRMBR_HU_Pos)             /*!< 0x00080000 */
#define RTC_ALRMBR_MSK2_Pos            (15U)                                   
#define RTC_ALRMBR_MSK2_Msk            (0x1U << RTC_ALRMBR_MSK2_Pos)           /*!< 0x00008000 */
#define RTC_ALRMBR_MSK2                RTC_ALRMBR_MSK2_Msk                     /*!<  */
#define RTC_ALRMBR_MNT_Pos             (12U)                                   
#define RTC_ALRMBR_MNT_Msk             (0x7U << RTC_ALRMBR_MNT_Pos)            /*!< 0x00007000 */
#define RTC_ALRMBR_MNT                 RTC_ALRMBR_MNT_Msk                      /*!<  */
#define RTC_ALRMBR_MNT_0               (0x1U << RTC_ALRMBR_MNT_Pos)            /*!< 0x00001000 */
#define RTC_ALRMBR_MNT_1               (0x2U << RTC_ALRMBR_MNT_Pos)            /*!< 0x00002000 */
#define RTC_ALRMBR_MNT_2               (0x4U << RTC_ALRMBR_MNT_Pos)            /*!< 0x00004000 */
#define RTC_ALRMBR_MNU_Pos             (8U)                                    
#define RTC_ALRMBR_MNU_Msk             (0xFU << RTC_ALRMBR_MNU_Pos)            /*!< 0x00000F00 */
#define RTC_ALRMBR_MNU                 RTC_ALRMBR_MNU_Msk                      /*!<  */
#define RTC_ALRMBR_MNU_0               (0x1U << RTC_ALRMBR_MNU_Pos)            /*!< 0x00000100 */
#define RTC_ALRMBR_MNU_1               (0x2U << RTC_ALRMBR_MNU_Pos)            /*!< 0x00000200 */
#define RTC_ALRMBR_MNU_2               (0x4U << RTC_ALRMBR_MNU_Pos)            /*!< 0x00000400 */
#define RTC_ALRMBR_MNU_3               (0x8U << RTC_ALRMBR_MNU_Pos)            /*!< 0x00000800 */
#define RTC_ALRMBR_MSK1_Pos            (7U)                                    
#define RTC_ALRMBR_MSK1_Msk            (0x1U << RTC_ALRMBR_MSK1_Pos)           /*!< 0x00000080 */
#define RTC_ALRMBR_MSK1                RTC_ALRMBR_MSK1_Msk                     /*!<  */
#define RTC_ALRMBR_ST_Pos              (4U)                                    
#define RTC_ALRMBR_ST_Msk              (0x7U << RTC_ALRMBR_ST_Pos)             /*!< 0x00000070 */
#define RTC_ALRMBR_ST                  RTC_ALRMBR_ST_Msk                       /*!<  */
#define RTC_ALRMBR_ST_0                (0x1U << RTC_ALRMBR_ST_Pos)             /*!< 0x00000010 */
#define RTC_ALRMBR_ST_1                (0x2U << RTC_ALRMBR_ST_Pos)             /*!< 0x00000020 */
#define RTC_ALRMBR_ST_2                (0x4U << RTC_ALRMBR_ST_Pos)             /*!< 0x00000040 */
#define RTC_ALRMBR_SU_Pos              (0U)                                    
#define RTC_ALRMBR_SU_Msk              (0xFU << RTC_ALRMBR_SU_Pos)             /*!< 0x0000000F */
#define RTC_ALRMBR_SU                  RTC_ALRMBR_SU_Msk                       /*!<  */
#define RTC_ALRMBR_SU_0                (0x1U << RTC_ALRMBR_SU_Pos)             /*!< 0x00000001 */
#define RTC_ALRMBR_SU_1                (0x2U << RTC_ALRMBR_SU_Pos)             /*!< 0x00000002 */
#define RTC_ALRMBR_SU_2                (0x4U << RTC_ALRMBR_SU_Pos)             /*!< 0x00000004 */
#define RTC_ALRMBR_SU_3                (0x8U << RTC_ALRMBR_SU_Pos)             /*!< 0x00000008 */

/********************  Bits definition for RTC_WPR register  ******************/
#define RTC_WPR_KEY_Pos                (0U)                                    
#define RTC_WPR_KEY_Msk                (0xFFU << RTC_WPR_KEY_Pos)              /*!< 0x000000FF */
#define RTC_WPR_KEY                    RTC_WPR_KEY_Msk                         /*!<  */

/********************  Bits definition for RTC_SSR register  ******************/
#define RTC_SSR_SS_Pos                 (0U)                                    
#define RTC_SSR_SS_Msk                 (0xFFFFU << RTC_SSR_SS_Pos)             /*!< 0x0000FFFF */
#define RTC_SSR_SS                     RTC_SSR_SS_Msk                          /*!<  */

/********************  Bits definition for RTC_SHIFTR register  ***************/
#define RTC_SHIFTR_SUBFS_Pos           (0U)                                    
#define RTC_SHIFTR_SUBFS_Msk           (0x7FFFU << RTC_SHIFTR_SUBFS_Pos)       /*!< 0x00007FFF */
#define RTC_SHIFTR_SUBFS               RTC_SHIFTR_SUBFS_Msk                    /*!<  */
#define RTC_SHIFTR_ADD1S_Pos           (31U)                                   
#define RTC_SHIFTR_ADD1S_Msk           (0x1U << RTC_SHIFTR_ADD1S_Pos)          /*!< 0x80000000 */
#define RTC_SHIFTR_ADD1S               RTC_SHIFTR_ADD1S_Msk                    /*!<  */

/********************  Bits definition for RTC_TSTR register  *****************/
#define RTC_TSTR_PM_Pos                (22U)                                   
#define RTC_TSTR_PM_Msk                (0x1U << RTC_TSTR_PM_Pos)               /*!< 0x00400000 */
#define RTC_TSTR_PM                    RTC_TSTR_PM_Msk                         /*!<  */
#define RTC_TSTR_HT_Pos                (20U)                                   
#define RTC_TSTR_HT_Msk                (0x3U << RTC_TSTR_HT_Pos)               /*!< 0x00300000 */
#define RTC_TSTR_HT                    RTC_TSTR_HT_Msk                         /*!<  */
#define RTC_TSTR_HT_0                  (0x1U << RTC_TSTR_HT_Pos)               /*!< 0x00100000 */
#define RTC_TSTR_HT_1                  (0x2U << RTC_TSTR_HT_Pos)               /*!< 0x00200000 */
#define RTC_TSTR_HU_Pos                (16U)                                   
#define RTC_TSTR_HU_Msk                (0xFU << RTC_TSTR_HU_Pos)               /*!< 0x000F0000 */
#define RTC_TSTR_HU                    RTC_TSTR_HU_Msk                         /*!<  */
#define RTC_TSTR_HU_0                  (0x1U << RTC_TSTR_HU_Pos)               /*!< 0x00010000 */
#define RTC_TSTR_HU_1                  (0x2U << RTC_TSTR_HU_Pos)               /*!< 0x00020000 */
#define RTC_TSTR_HU_2                  (0x4U << RTC_TSTR_HU_Pos)               /*!< 0x00040000 */
#define RTC_TSTR_HU_3                  (0x8U << RTC_TSTR_HU_Pos)               /*!< 0x00080000 */
#define RTC_TSTR_MNT_Pos               (12U)                                   
#define RTC_TSTR_MNT_Msk               (0x7U << RTC_TSTR_MNT_Pos)              /*!< 0x00007000 */
#define RTC_TSTR_MNT                   RTC_TSTR_MNT_Msk                        /*!<  */
#define RTC_TSTR_MNT_0                 (0x1U << RTC_TSTR_MNT_Pos)              /*!< 0x00001000 */
#define RTC_TSTR_MNT_1                 (0x2U << RTC_TSTR_MNT_Pos)              /*!< 0x00002000 */
#define RTC_TSTR_MNT_2                 (0x4U << RTC_TSTR_MNT_Pos)              /*!< 0x00004000 */
#define RTC_TSTR_MNU_Pos               (8U)                                    
#define RTC_TSTR_MNU_Msk               (0xFU << RTC_TSTR_MNU_Pos)              /*!< 0x00000F00 */
#define RTC_TSTR_MNU                   RTC_TSTR_MNU_Msk                        /*!<  */
#define RTC_TSTR_MNU_0                 (0x1U << RTC_TSTR_MNU_Pos)              /*!< 0x00000100 */
#define RTC_TSTR_MNU_1                 (0x2U << RTC_TSTR_MNU_Pos)              /*!< 0x00000200 */
#define RTC_TSTR_MNU_2                 (0x4U << RTC_TSTR_MNU_Pos)              /*!< 0x00000400 */
#define RTC_TSTR_MNU_3                 (0x8U << RTC_TSTR_MNU_Pos)              /*!< 0x00000800 */
#define RTC_TSTR_ST_Pos                (4U)                                    
#define RTC_TSTR_ST_Msk                (0x7U << RTC_TSTR_ST_Pos)               /*!< 0x00000070 */
#define RTC_TSTR_ST                    RTC_TSTR_ST_Msk                         /*!<  */
#define RTC_TSTR_ST_0                  (0x1U << RTC_TSTR_ST_Pos)               /*!< 0x00000010 */
#define RTC_TSTR_ST_1                  (0x2U << RTC_TSTR_ST_Pos)               /*!< 0x00000020 */
#define RTC_TSTR_ST_2                  (0x4U << RTC_TSTR_ST_Pos)               /*!< 0x00000040 */
#define RTC_TSTR_SU_Pos                (0U)                                    
#define RTC_TSTR_SU_Msk                (0xFU << RTC_TSTR_SU_Pos)               /*!< 0x0000000F */
#define RTC_TSTR_SU                    RTC_TSTR_SU_Msk                         /*!<  */
#define RTC_TSTR_SU_0                  (0x1U << RTC_TSTR_SU_Pos)               /*!< 0x00000001 */
#define RTC_TSTR_SU_1                  (0x2U << RTC_TSTR_SU_Pos)               /*!< 0x00000002 */
#define RTC_TSTR_SU_2                  (0x4U << RTC_TSTR_SU_Pos)               /*!< 0x00000004 */
#define RTC_TSTR_SU_3                  (0x8U << RTC_TSTR_SU_Pos)               /*!< 0x00000008 */

/********************  Bits definition for RTC_TSDR register  *****************/
#define RTC_TSDR_WDU_Pos               (13U)                                   
#define RTC_TSDR_WDU_Msk               (0x7U << RTC_TSDR_WDU_Pos)              /*!< 0x0000E000 */
#define RTC_TSDR_WDU                   RTC_TSDR_WDU_Msk                        /*!<  */
#define RTC_TSDR_WDU_0                 (0x1U << RTC_TSDR_WDU_Pos)              /*!< 0x00002000 */
#define RTC_TSDR_WDU_1                 (0x2U << RTC_TSDR_WDU_Pos)              /*!< 0x00004000 */
#define RTC_TSDR_WDU_2                 (0x4U << RTC_TSDR_WDU_Pos)              /*!< 0x00008000 */
#define RTC_TSDR_MT_Pos                (12U)                                   
#define RTC_TSDR_MT_Msk                (0x1U << RTC_TSDR_MT_Pos)               /*!< 0x00001000 */
#define RTC_TSDR_MT                    RTC_TSDR_MT_Msk                         /*!<  */
#define RTC_TSDR_MU_Pos                (8U)                                    
#define RTC_TSDR_MU_Msk                (0xFU << RTC_TSDR_MU_Pos)               /*!< 0x00000F00 */
#define RTC_TSDR_MU                    RTC_TSDR_MU_Msk                         /*!<  */
#define RTC_TSDR_MU_0                  (0x1U << RTC_TSDR_MU_Pos)               /*!< 0x00000100 */
#define RTC_TSDR_MU_1                  (0x2U << RTC_TSDR_MU_Pos)               /*!< 0x00000200 */
#define RTC_TSDR_MU_2                  (0x4U << RTC_TSDR_MU_Pos)               /*!< 0x00000400 */
#define RTC_TSDR_MU_3                  (0x8U << RTC_TSDR_MU_Pos)               /*!< 0x00000800 */
#define RTC_TSDR_DT_Pos                (4U)                                    
#define RTC_TSDR_DT_Msk                (0x3U << RTC_TSDR_DT_Pos)               /*!< 0x00000030 */
#define RTC_TSDR_DT                    RTC_TSDR_DT_Msk                         /*!<  */
#define RTC_TSDR_DT_0                  (0x1U << RTC_TSDR_DT_Pos)               /*!< 0x00000010 */
#define RTC_TSDR_DT_1                  (0x2U << RTC_TSDR_DT_Pos)               /*!< 0x00000020 */
#define RTC_TSDR_DU_Pos                (0U)                                    
#define RTC_TSDR_DU_Msk                (0xFU << RTC_TSDR_DU_Pos)               /*!< 0x0000000F */
#define RTC_TSDR_DU                    RTC_TSDR_DU_Msk                         /*!<  */
#define RTC_TSDR_DU_0                  (0x1U << RTC_TSDR_DU_Pos)               /*!< 0x00000001 */
#define RTC_TSDR_DU_1                  (0x2U << RTC_TSDR_DU_Pos)               /*!< 0x00000002 */
#define RTC_TSDR_DU_2                  (0x4U << RTC_TSDR_DU_Pos)               /*!< 0x00000004 */
#define RTC_TSDR_DU_3                  (0x8U << RTC_TSDR_DU_Pos)               /*!< 0x00000008 */

/********************  Bits definition for RTC_TSSSR register  ****************/
#define RTC_TSSSR_SS_Pos               (0U)                                    
#define RTC_TSSSR_SS_Msk               (0xFFFFU << RTC_TSSSR_SS_Pos)           /*!< 0x0000FFFF */
#define RTC_TSSSR_SS                   RTC_TSSSR_SS_Msk                        

/********************  Bits definition for RTC_CALR register  *****************/
#define RTC_CALR_CALP_Pos              (15U)                                   
#define RTC_CALR_CALP_Msk              (0x1U << RTC_CALR_CALP_Pos)             /*!< 0x00008000 */
#define RTC_CALR_CALP                  RTC_CALR_CALP_Msk                       /*!<  */
#define RTC_CALR_CALW8_Pos             (14U)                                   
#define RTC_CALR_CALW8_Msk             (0x1U << RTC_CALR_CALW8_Pos)            /*!< 0x00004000 */
#define RTC_CALR_CALW8                 RTC_CALR_CALW8_Msk                      /*!<  */
#define RTC_CALR_CALW16_Pos            (13U)                                   
#define RTC_CALR_CALW16_Msk            (0x1U << RTC_CALR_CALW16_Pos)           /*!< 0x00002000 */
#define RTC_CALR_CALW16                RTC_CALR_CALW16_Msk                     /*!<  */
#define RTC_CALR_CALM_Pos              (0U)                                    
#define RTC_CALR_CALM_Msk              (0x1FFU << RTC_CALR_CALM_Pos)           /*!< 0x000001FF */
#define RTC_CALR_CALM                  RTC_CALR_CALM_Msk                       /*!<  */
#define RTC_CALR_CALM_0                (0x001U << RTC_CALR_CALM_Pos)           /*!< 0x00000001 */
#define RTC_CALR_CALM_1                (0x002U << RTC_CALR_CALM_Pos)           /*!< 0x00000002 */
#define RTC_CALR_CALM_2                (0x004U << RTC_CALR_CALM_Pos)           /*!< 0x00000004 */
#define RTC_CALR_CALM_3                (0x008U << RTC_CALR_CALM_Pos)           /*!< 0x00000008 */
#define RTC_CALR_CALM_4                (0x010U << RTC_CALR_CALM_Pos)           /*!< 0x00000010 */
#define RTC_CALR_CALM_5                (0x020U << RTC_CALR_CALM_Pos)           /*!< 0x00000020 */
#define RTC_CALR_CALM_6                (0x040U << RTC_CALR_CALM_Pos)           /*!< 0x00000040 */
#define RTC_CALR_CALM_7                (0x080U << RTC_CALR_CALM_Pos)           /*!< 0x00000080 */
#define RTC_CALR_CALM_8                (0x100U << RTC_CALR_CALM_Pos)           /*!< 0x00000100 */

/* Legacy defines */
#define RTC_CAL_CALP     RTC_CALR_CALP 
#define RTC_CAL_CALW8    RTC_CALR_CALW8  
#define RTC_CAL_CALW16   RTC_CALR_CALW16 
#define RTC_CAL_CALM     RTC_CALR_CALM 
#define RTC_CAL_CALM_0   RTC_CALR_CALM_0 
#define RTC_CAL_CALM_1   RTC_CALR_CALM_1 
#define RTC_CAL_CALM_2   RTC_CALR_CALM_2 
#define RTC_CAL_CALM_3   RTC_CALR_CALM_3 
#define RTC_CAL_CALM_4   RTC_CALR_CALM_4 
#define RTC_CAL_CALM_5   RTC_CALR_CALM_5 
#define RTC_CAL_CALM_6   RTC_CALR_CALM_6 
#define RTC_CAL_CALM_7   RTC_CALR_CALM_7 
#define RTC_CAL_CALM_8   RTC_CALR_CALM_8 

/********************  Bits definition for RTC_TAMPCR register  ****************/
#define RTC_TAMPCR_TAMP2MF_Pos         (21U)                                   
#define RTC_TAMPCR_TAMP2MF_Msk         (0x1U << RTC_TAMPCR_TAMP2MF_Pos)        /*!< 0x00200000 */
#define RTC_TAMPCR_TAMP2MF             RTC_TAMPCR_TAMP2MF_Msk                  /*!<  */
#define RTC_TAMPCR_TAMP2NOERASE_Pos    (20U)                                   
#define RTC_TAMPCR_TAMP2NOERASE_Msk    (0x1U << RTC_TAMPCR_TAMP2NOERASE_Pos)   /*!< 0x00100000 */
#define RTC_TAMPCR_TAMP2NOERASE        RTC_TAMPCR_TAMP2NOERASE_Msk             /*!<  */
#define RTC_TAMPCR_TAMP2IE_Pos         (19U)                                   
#define RTC_TAMPCR_TAMP2IE_Msk         (0x1U << RTC_TAMPCR_TAMP2IE_Pos)        /*!< 0x00080000 */
#define RTC_TAMPCR_TAMP2IE             RTC_TAMPCR_TAMP2IE_Msk                  /*!<  */
#define RTC_TAMPCR_TAMP1MF_Pos         (18U)                                   
#define RTC_TAMPCR_TAMP1MF_Msk         (0x1U << RTC_TAMPCR_TAMP1MF_Pos)        /*!< 0x00040000 */
#define RTC_TAMPCR_TAMP1MF             RTC_TAMPCR_TAMP1MF_Msk                  /*!<  */
#define RTC_TAMPCR_TAMP1NOERASE_Pos    (17U)                                   
#define RTC_TAMPCR_TAMP1NOERASE_Msk    (0x1U << RTC_TAMPCR_TAMP1NOERASE_Pos)   /*!< 0x00020000 */
#define RTC_TAMPCR_TAMP1NOERASE        RTC_TAMPCR_TAMP1NOERASE_Msk             /*!<  */
#define RTC_TAMPCR_TAMP1IE_Pos         (16U)                                   
#define RTC_TAMPCR_TAMP1IE_Msk         (0x1U << RTC_TAMPCR_TAMP1IE_Pos)        /*!< 0x00010000 */
#define RTC_TAMPCR_TAMP1IE             RTC_TAMPCR_TAMP1IE_Msk                  /*!<  */
#define RTC_TAMPCR_TAMPPUDIS_Pos       (15U)                                   
#define RTC_TAMPCR_TAMPPUDIS_Msk       (0x1U << RTC_TAMPCR_TAMPPUDIS_Pos)      /*!< 0x00008000 */
#define RTC_TAMPCR_TAMPPUDIS           RTC_TAMPCR_TAMPPUDIS_Msk                /*!<  */
#define RTC_TAMPCR_TAMPPRCH_Pos        (13U)                                   
#define RTC_TAMPCR_TAMPPRCH_Msk        (0x3U << RTC_TAMPCR_TAMPPRCH_Pos)       /*!< 0x00006000 */
#define RTC_TAMPCR_TAMPPRCH            RTC_TAMPCR_TAMPPRCH_Msk                 /*!<  */
#define RTC_TAMPCR_TAMPPRCH_0          (0x1U << RTC_TAMPCR_TAMPPRCH_Pos)       /*!< 0x00002000 */
#define RTC_TAMPCR_TAMPPRCH_1          (0x2U << RTC_TAMPCR_TAMPPRCH_Pos)       /*!< 0x00004000 */
#define RTC_TAMPCR_TAMPFLT_Pos         (11U)                                   
#define RTC_TAMPCR_TAMPFLT_Msk         (0x3U << RTC_TAMPCR_TAMPFLT_Pos)        /*!< 0x00001800 */
#define RTC_TAMPCR_TAMPFLT             RTC_TAMPCR_TAMPFLT_Msk                  /*!<  */
#define RTC_TAMPCR_TAMPFLT_0           (0x1U << RTC_TAMPCR_TAMPFLT_Pos)        /*!< 0x00000800 */
#define RTC_TAMPCR_TAMPFLT_1           (0x2U << RTC_TAMPCR_TAMPFLT_Pos)        /*!< 0x00001000 */
#define RTC_TAMPCR_TAMPFREQ_Pos        (8U)                                    
#define RTC_TAMPCR_TAMPFREQ_Msk        (0x7U << RTC_TAMPCR_TAMPFREQ_Pos)       /*!< 0x00000700 */
#define RTC_TAMPCR_TAMPFREQ            RTC_TAMPCR_TAMPFREQ_Msk                 /*!<  */
#define RTC_TAMPCR_TAMPFREQ_0          (0x1U << RTC_TAMPCR_TAMPFREQ_Pos)       /*!< 0x00000100 */
#define RTC_TAMPCR_TAMPFREQ_1          (0x2U << RTC_TAMPCR_TAMPFREQ_Pos)       /*!< 0x00000200 */
#define RTC_TAMPCR_TAMPFREQ_2          (0x4U << RTC_TAMPCR_TAMPFREQ_Pos)       /*!< 0x00000400 */
#define RTC_TAMPCR_TAMPTS_Pos          (7U)                                    
#define RTC_TAMPCR_TAMPTS_Msk          (0x1U << RTC_TAMPCR_TAMPTS_Pos)         /*!< 0x00000080 */
#define RTC_TAMPCR_TAMPTS              RTC_TAMPCR_TAMPTS_Msk                   /*!<  */
#define RTC_TAMPCR_TAMP2TRG_Pos        (4U)                                    
#define RTC_TAMPCR_TAMP2TRG_Msk        (0x1U << RTC_TAMPCR_TAMP2TRG_Pos)       /*!< 0x00000010 */
#define RTC_TAMPCR_TAMP2TRG            RTC_TAMPCR_TAMP2TRG_Msk                 /*!<  */
#define RTC_TAMPCR_TAMP2E_Pos          (3U)                                    
#define RTC_TAMPCR_TAMP2E_Msk          (0x1U << RTC_TAMPCR_TAMP2E_Pos)         /*!< 0x00000008 */
#define RTC_TAMPCR_TAMP2E              RTC_TAMPCR_TAMP2E_Msk                   /*!<  */
#define RTC_TAMPCR_TAMPIE_Pos          (2U)                                    
#define RTC_TAMPCR_TAMPIE_Msk          (0x1U << RTC_TAMPCR_TAMPIE_Pos)         /*!< 0x00000004 */
#define RTC_TAMPCR_TAMPIE              RTC_TAMPCR_TAMPIE_Msk                   /*!<  */
#define RTC_TAMPCR_TAMP1TRG_Pos        (1U)                                    
#define RTC_TAMPCR_TAMP1TRG_Msk        (0x1U << RTC_TAMPCR_TAMP1TRG_Pos)       /*!< 0x00000002 */
#define RTC_TAMPCR_TAMP1TRG            RTC_TAMPCR_TAMP1TRG_Msk                 /*!<  */
#define RTC_TAMPCR_TAMP1E_Pos          (0U)                                    
#define RTC_TAMPCR_TAMP1E_Msk          (0x1U << RTC_TAMPCR_TAMP1E_Pos)         /*!< 0x00000001 */
#define RTC_TAMPCR_TAMP1E              RTC_TAMPCR_TAMP1E_Msk                   /*!<  */

/********************  Bits definition for RTC_ALRMASSR register  *************/
#define RTC_ALRMASSR_MASKSS_Pos        (24U)                                   
#define RTC_ALRMASSR_MASKSS_Msk        (0xFU << RTC_ALRMASSR_MASKSS_Pos)       /*!< 0x0F000000 */
#define RTC_ALRMASSR_MASKSS            RTC_ALRMASSR_MASKSS_Msk                 
#define RTC_ALRMASSR_MASKSS_0          (0x1U << RTC_ALRMASSR_MASKSS_Pos)       /*!< 0x01000000 */
#define RTC_ALRMASSR_MASKSS_1          (0x2U << RTC_ALRMASSR_MASKSS_Pos)       /*!< 0x02000000 */
#define RTC_ALRMASSR_MASKSS_2          (0x4U << RTC_ALRMASSR_MASKSS_Pos)       /*!< 0x04000000 */
#define RTC_ALRMASSR_MASKSS_3          (0x8U << RTC_ALRMASSR_MASKSS_Pos)       /*!< 0x08000000 */
#define RTC_ALRMASSR_SS_Pos            (0U)                                    
#define RTC_ALRMASSR_SS_Msk            (0x7FFFU << RTC_ALRMASSR_SS_Pos)        /*!< 0x00007FFF */
#define RTC_ALRMASSR_SS                RTC_ALRMASSR_SS_Msk                     

/********************  Bits definition for RTC_ALRMBSSR register  *************/
#define RTC_ALRMBSSR_MASKSS_Pos        (24U)                                   
#define RTC_ALRMBSSR_MASKSS_Msk        (0xFU << RTC_ALRMBSSR_MASKSS_Pos)       /*!< 0x0F000000 */
#define RTC_ALRMBSSR_MASKSS            RTC_ALRMBSSR_MASKSS_Msk                 
#define RTC_ALRMBSSR_MASKSS_0          (0x1U << RTC_ALRMBSSR_MASKSS_Pos)       /*!< 0x01000000 */
#define RTC_ALRMBSSR_MASKSS_1          (0x2U << RTC_ALRMBSSR_MASKSS_Pos)       /*!< 0x02000000 */
#define RTC_ALRMBSSR_MASKSS_2          (0x4U << RTC_ALRMBSSR_MASKSS_Pos)       /*!< 0x04000000 */
#define RTC_ALRMBSSR_MASKSS_3          (0x8U << RTC_ALRMBSSR_MASKSS_Pos)       /*!< 0x08000000 */
#define RTC_ALRMBSSR_SS_Pos            (0U)                                    
#define RTC_ALRMBSSR_SS_Msk            (0x7FFFU << RTC_ALRMBSSR_SS_Pos)        /*!< 0x00007FFF */
#define RTC_ALRMBSSR_SS                RTC_ALRMBSSR_SS_Msk                     

/********************  Bits definition for RTC_OR register  ****************/
#define RTC_OR_OUT_RMP_Pos             (1U)                                    
#define RTC_OR_OUT_RMP_Msk             (0x1U << RTC_OR_OUT_RMP_Pos)            /*!< 0x00000002 */
#define RTC_OR_OUT_RMP                 RTC_OR_OUT_RMP_Msk                      /*!<  */
#define RTC_OR_ALARMOUTTYPE_Pos        (0U)                                    
#define RTC_OR_ALARMOUTTYPE_Msk        (0x1U << RTC_OR_ALARMOUTTYPE_Pos)       /*!< 0x00000001 */
#define RTC_OR_ALARMOUTTYPE            RTC_OR_ALARMOUTTYPE_Msk                 /*!<  */

/* Legacy defines */
#define RTC_OR_RTC_OUT_RMP                   RTC_OR_OUT_RMP

/********************  Bits definition for RTC_BKP0R register  ****************/
#define RTC_BKP0R_Pos                  (0U)                                    
#define RTC_BKP0R_Msk                  (0xFFFFFFFFU << RTC_BKP0R_Pos)          /*!< 0xFFFFFFFF */
#define RTC_BKP0R                      RTC_BKP0R_Msk                           /*!<  */

/********************  Bits definition for RTC_BKP1R register  ****************/
#define RTC_BKP1R_Pos                  (0U)                                    
#define RTC_BKP1R_Msk                  (0xFFFFFFFFU << RTC_BKP1R_Pos)          /*!< 0xFFFFFFFF */
#define RTC_BKP1R                      RTC_BKP1R_Msk                           /*!<  */

/********************  Bits definition for RTC_BKP2R register  ****************/
#define RTC_BKP2R_Pos                  (0U)                                    
#define RTC_BKP2R_Msk                  (0xFFFFFFFFU << RTC_BKP2R_Pos)          /*!< 0xFFFFFFFF */
#define RTC_BKP2R                      RTC_BKP2R_Msk                           /*!<  */

/********************  Bits definition for RTC_BKP3R register  ****************/
#define RTC_BKP3R_Pos                  (0U)                                    
#define RTC_BKP3R_Msk                  (0xFFFFFFFFU << RTC_BKP3R_Pos)          /*!< 0xFFFFFFFF */
#define RTC_BKP3R                      RTC_BKP3R_Msk                           /*!<  */

/********************  Bits definition for RTC_BKP4R register  ****************/
#define RTC_BKP4R_Pos                  (0U)                                    
#define RTC_BKP4R_Msk                  (0xFFFFFFFFU << RTC_BKP4R_Pos)          /*!< 0xFFFFFFFF */
#define RTC_BKP4R                      RTC_BKP4R_Msk                           /*!<  */

/******************** Number of backup registers ******************************/
#define RTC_BKP_NUMBER                       (0x00000005U)                  /*!<  */

/******************************************************************************/
/*                                                                            */
/*                        Serial Peripheral Interface (SPI)                   */
/*                                                                            */
/******************************************************************************/

/*
 * @brief Specific device feature definitions (not present on all devices in the STM32L0 family)
 */
#define SPI_I2S_SUPPORT                       /*!< I2S support */

/*******************  Bit definition for SPI_CR1 register  ********************/
#define SPI_CR1_CPHA_Pos            (0U)                                       
#define SPI_CR1_CPHA_Msk            (0x1U << SPI_CR1_CPHA_Pos)                 /*!< 0x00000001 */
#define SPI_CR1_CPHA                SPI_CR1_CPHA_Msk                           /*!< Clock Phase */
#define SPI_CR1_CPOL_Pos            (1U)                                       
#define SPI_CR1_CPOL_Msk            (0x1U << SPI_CR1_CPOL_Pos)                 /*!< 0x00000002 */
#define SPI_CR1_CPOL                SPI_CR1_CPOL_Msk                           /*!< Clock Polarity */
#define SPI_CR1_MSTR_Pos            (2U)                                       
#define SPI_CR1_MSTR_Msk            (0x1U << SPI_CR1_MSTR_Pos)                 /*!< 0x00000004 */
#define SPI_CR1_MSTR                SPI_CR1_MSTR_Msk                           /*!< Master Selection */
#define SPI_CR1_BR_Pos              (3U)                                       
#define SPI_CR1_BR_Msk              (0x7U << SPI_CR1_BR_Pos)                   /*!< 0x00000038 */
#define SPI_CR1_BR                  SPI_CR1_BR_Msk                             /*!< BR[2:0] bits (Baud Rate Control) */
#define SPI_CR1_BR_0                (0x1U << SPI_CR1_BR_Pos)                   /*!< 0x00000008 */
#define SPI_CR1_BR_1                (0x2U << SPI_CR1_BR_Pos)                   /*!< 0x00000010 */
#define SPI_CR1_BR_2                (0x4U << SPI_CR1_BR_Pos)                   /*!< 0x00000020 */
#define SPI_CR1_SPE_Pos             (6U)                                       
#define SPI_CR1_SPE_Msk             (0x1U << SPI_CR1_SPE_Pos)                  /*!< 0x00000040 */
#define SPI_CR1_SPE                 SPI_CR1_SPE_Msk                            /*!< SPI Enable */
#define SPI_CR1_LSBFIRST_Pos        (7U)                                       
#define SPI_CR1_LSBFIRST_Msk        (0x1U << SPI_CR1_LSBFIRST_Pos)             /*!< 0x00000080 */
#define SPI_CR1_LSBFIRST            SPI_CR1_LSBFIRST_Msk                       /*!< Frame Format */
#define SPI_CR1_SSI_Pos             (8U)                                       
#define SPI_CR1_SSI_Msk             (0x1U << SPI_CR1_SSI_Pos)                  /*!< 0x00000100 */
#define SPI_CR1_SSI                 SPI_CR1_SSI_Msk                            /*!< Internal slave select */
#define SPI_CR1_SSM_Pos             (9U)                                       
#define SPI_CR1_SSM_Msk             (0x1U << SPI_CR1_SSM_Pos)                  /*!< 0x00000200 */
#define SPI_CR1_SSM                 SPI_CR1_SSM_Msk                            /*!< Software slave management */
#define SPI_CR1_RXONLY_Pos          (10U)                                      
#define SPI_CR1_RXONLY_Msk          (0x1U << SPI_CR1_RXONLY_Pos)               /*!< 0x00000400 */
#define SPI_CR1_RXONLY              SPI_CR1_RXONLY_Msk                         /*!< Receive only */
#define SPI_CR1_DFF_Pos             (11U)                                      
#define SPI_CR1_DFF_Msk             (0x1U << SPI_CR1_DFF_Pos)                  /*!< 0x00000800 */
#define SPI_CR1_DFF                 SPI_CR1_DFF_Msk                            /*!< Data Frame Format */
#define SPI_CR1_CRCNEXT_Pos         (12U)                                      
#define SPI_CR1_CRCNEXT_Msk         (0x1U << SPI_CR1_CRCNEXT_Pos)              /*!< 0x00001000 */
#define SPI_CR1_CRCNEXT             SPI_CR1_CRCNEXT_Msk                        /*!< Transmit CRC next */
#define SPI_CR1_CRCEN_Pos           (13U)                                      
#define SPI_CR1_CRCEN_Msk           (0x1U << SPI_CR1_CRCEN_Pos)                /*!< 0x00002000 */
#define SPI_CR1_CRCEN               SPI_CR1_CRCEN_Msk                          /*!< Hardware CRC calculation enable */
#define SPI_CR1_BIDIOE_Pos          (14U)                                      
#define SPI_CR1_BIDIOE_Msk          (0x1U << SPI_CR1_BIDIOE_Pos)               /*!< 0x00004000 */
#define SPI_CR1_BIDIOE              SPI_CR1_BIDIOE_Msk                         /*!< Output enable in bidirectional mode */
#define SPI_CR1_BIDIMODE_Pos        (15U)                                      
#define SPI_CR1_BIDIMODE_Msk        (0x1U << SPI_CR1_BIDIMODE_Pos)             /*!< 0x00008000 */
#define SPI_CR1_BIDIMODE            SPI_CR1_BIDIMODE_Msk                       /*!< Bidirectional data mode enable */

/*******************  Bit definition for SPI_CR2 register  ********************/
#define SPI_CR2_RXDMAEN_Pos         (0U)                                       
#define SPI_CR2_RXDMAEN_Msk         (0x1U << SPI_CR2_RXDMAEN_Pos)              /*!< 0x00000001 */
#define SPI_CR2_RXDMAEN             SPI_CR2_RXDMAEN_Msk                        /*!< Rx Buffer DMA Enable */
#define SPI_CR2_TXDMAEN_Pos         (1U)                                       
#define SPI_CR2_TXDMAEN_Msk         (0x1U << SPI_CR2_TXDMAEN_Pos)              /*!< 0x00000002 */
#define SPI_CR2_TXDMAEN             SPI_CR2_TXDMAEN_Msk                        /*!< Tx Buffer DMA Enable */
#define SPI_CR2_SSOE_Pos            (2U)                                       
#define SPI_CR2_SSOE_Msk            (0x1U << SPI_CR2_SSOE_Pos)                 /*!< 0x00000004 */
#define SPI_CR2_SSOE                SPI_CR2_SSOE_Msk                           /*!< SS Output Enable */
#define SPI_CR2_FRF_Pos             (4U)                                       
#define SPI_CR2_FRF_Msk             (0x1U << SPI_CR2_FRF_Pos)                  /*!< 0x00000010 */
#define SPI_CR2_FRF                 SPI_CR2_FRF_Msk                            /*!< Frame Format Enable */
#define SPI_CR2_ERRIE_Pos           (5U)                                       
#define SPI_CR2_ERRIE_Msk           (0x1U << SPI_CR2_ERRIE_Pos)                /*!< 0x00000020 */
#define SPI_CR2_ERRIE               SPI_CR2_ERRIE_Msk                          /*!< Error Interrupt Enable */
#define SPI_CR2_RXNEIE_Pos          (6U)                                       
#define SPI_CR2_RXNEIE_Msk          (0x1U << SPI_CR2_RXNEIE_Pos)               /*!< 0x00000040 */
#define SPI_CR2_RXNEIE              SPI_CR2_RXNEIE_Msk                         /*!< RX buffer Not Empty Interrupt Enable */
#define SPI_CR2_TXEIE_Pos           (7U)                                       
#define SPI_CR2_TXEIE_Msk           (0x1U << SPI_CR2_TXEIE_Pos)                /*!< 0x00000080 */
#define SPI_CR2_TXEIE               SPI_CR2_TXEIE_Msk                          /*!< Tx buffer Empty Interrupt Enable */

/********************  Bit definition for SPI_SR register  ********************/
#define SPI_SR_RXNE_Pos             (0U)                                       
#define SPI_SR_RXNE_Msk             (0x1U << SPI_SR_RXNE_Pos)                  /*!< 0x00000001 */
#define SPI_SR_RXNE                 SPI_SR_RXNE_Msk                            /*!< Receive buffer Not Empty */
#define SPI_SR_TXE_Pos              (1U)                                       
#define SPI_SR_TXE_Msk              (0x1U << SPI_SR_TXE_Pos)                   /*!< 0x00000002 */
#define SPI_SR_TXE                  SPI_SR_TXE_Msk                             /*!< Transmit buffer Empty */
#define SPI_SR_CHSIDE_Pos           (2U)                                       
#define SPI_SR_CHSIDE_Msk           (0x1U << SPI_SR_CHSIDE_Pos)                /*!< 0x00000004 */
#define SPI_SR_CHSIDE               SPI_SR_CHSIDE_Msk                          /*!< Channel side */
#define SPI_SR_UDR_Pos              (3U)                                       
#define SPI_SR_UDR_Msk              (0x1U << SPI_SR_UDR_Pos)                   /*!< 0x00000008 */
#define SPI_SR_UDR                  SPI_SR_UDR_Msk                             /*!< Underrun flag */
#define SPI_SR_CRCERR_Pos           (4U)                                       
#define SPI_SR_CRCERR_Msk           (0x1U << SPI_SR_CRCERR_Pos)                /*!< 0x00000010 */
#define SPI_SR_CRCERR               SPI_SR_CRCERR_Msk                          /*!< CRC Error flag */
#define SPI_SR_MODF_Pos             (5U)                                       
#define SPI_SR_MODF_Msk             (0x1U << SPI_SR_MODF_Pos)                  /*!< 0x00000020 */
#define SPI_SR_MODF                 SPI_SR_MODF_Msk                            /*!< Mode fault */
#define SPI_SR_OVR_Pos              (6U)                                       
#define SPI_SR_OVR_Msk              (0x1U << SPI_SR_OVR_Pos)                   /*!< 0x00000040 */
#define SPI_SR_OVR                  SPI_SR_OVR_Msk                             /*!< Overrun flag */
#define SPI_SR_BSY_Pos              (7U)                                       
#define SPI_SR_BSY_Msk              (0x1U << SPI_SR_BSY_Pos)                   /*!< 0x00000080 */
#define SPI_SR_BSY                  SPI_SR_BSY_Msk                             /*!< Busy flag */
#define SPI_SR_FRE_Pos              (8U)                                       
#define SPI_SR_FRE_Msk              (0x1U << SPI_SR_FRE_Pos)                   /*!< 0x00000100 */
#define SPI_SR_FRE                  SPI_SR_FRE_Msk                             /*!< TI frame format error */  

/********************  Bit definition for SPI_DR register  ********************/
#define SPI_DR_DR_Pos               (0U)                                       
#define SPI_DR_DR_Msk               (0xFFFFU << SPI_DR_DR_Pos)                 /*!< 0x0000FFFF */
#define SPI_DR_DR                   SPI_DR_DR_Msk                              /*!< Data Register */

/*******************  Bit definition for SPI_CRCPR register  ******************/
#define SPI_CRCPR_CRCPOLY_Pos       (0U)                                       
#define SPI_CRCPR_CRCPOLY_Msk       (0xFFFFU << SPI_CRCPR_CRCPOLY_Pos)         /*!< 0x0000FFFF */
#define SPI_CRCPR_CRCPOLY           SPI_CRCPR_CRCPOLY_Msk                      /*!< CRC polynomial register */

/******************  Bit definition for SPI_RXCRCR register  ******************/
#define SPI_RXCRCR_RXCRC_Pos        (0U)                                       
#define SPI_RXCRCR_RXCRC_Msk        (0xFFFFU << SPI_RXCRCR_RXCRC_Pos)          /*!< 0x0000FFFF */
#define SPI_RXCRCR_RXCRC            SPI_RXCRCR_RXCRC_Msk                       /*!< Rx CRC Register */

/******************  Bit definition for SPI_TXCRCR register  ******************/
#define SPI_TXCRCR_TXCRC_Pos        (0U)                                       
#define SPI_TXCRCR_TXCRC_Msk        (0xFFFFU << SPI_TXCRCR_TXCRC_Pos)          /*!< 0x0000FFFF */
#define SPI_TXCRCR_TXCRC            SPI_TXCRCR_TXCRC_Msk                       /*!< Tx CRC Register */

/******************  Bit definition for SPI_I2SCFGR register  *****************/
#define SPI_I2SCFGR_CHLEN_Pos       (0U)                                       
#define SPI_I2SCFGR_CHLEN_Msk       (0x1U << SPI_I2SCFGR_CHLEN_Pos)            /*!< 0x00000001 */
#define SPI_I2SCFGR_CHLEN           SPI_I2SCFGR_CHLEN_Msk                      /*!<Channel length (number of bits per audio channel) */
#define SPI_I2SCFGR_DATLEN_Pos      (1U)                                       
#define SPI_I2SCFGR_DATLEN_Msk      (0x3U << SPI_I2SCFGR_DATLEN_Pos)           /*!< 0x00000006 */
#define SPI_I2SCFGR_DATLEN          SPI_I2SCFGR_DATLEN_Msk                     /*!<DATLEN[1:0] bits (Data length to be transferred) */
#define SPI_I2SCFGR_DATLEN_0        (0x1U << SPI_I2SCFGR_DATLEN_Pos)           /*!< 0x00000002 */
#define SPI_I2SCFGR_DATLEN_1        (0x2U << SPI_I2SCFGR_DATLEN_Pos)           /*!< 0x00000004 */
#define SPI_I2SCFGR_CKPOL_Pos       (3U)                                       
#define SPI_I2SCFGR_CKPOL_Msk       (0x1U << SPI_I2SCFGR_CKPOL_Pos)            /*!< 0x00000008 */
#define SPI_I2SCFGR_CKPOL           SPI_I2SCFGR_CKPOL_Msk                      /*!<steady state clock polarity */
#define SPI_I2SCFGR_I2SSTD_Pos      (4U)                                       
#define SPI_I2SCFGR_I2SSTD_Msk      (0x3U << SPI_I2SCFGR_I2SSTD_Pos)           /*!< 0x00000030 */
#define SPI_I2SCFGR_I2SSTD          SPI_I2SCFGR_I2SSTD_Msk                     /*!<I2SSTD[1:0] bits (I2S standard selection) */
#define SPI_I2SCFGR_I2SSTD_0        (0x1U << SPI_I2SCFGR_I2SSTD_Pos)           /*!< 0x00000010 */
#define SPI_I2SCFGR_I2SSTD_1        (0x2U << SPI_I2SCFGR_I2SSTD_Pos)           /*!< 0x00000020 */
#define SPI_I2SCFGR_PCMSYNC_Pos     (7U)                                       
#define SPI_I2SCFGR_PCMSYNC_Msk     (0x1U << SPI_I2SCFGR_PCMSYNC_Pos)          /*!< 0x00000080 */
#define SPI_I2SCFGR_PCMSYNC         SPI_I2SCFGR_PCMSYNC_Msk                    /*!<PCM frame synchronization */
#define SPI_I2SCFGR_I2SCFG_Pos      (8U)                                       
#define SPI_I2SCFGR_I2SCFG_Msk      (0x3U << SPI_I2SCFGR_I2SCFG_Pos)           /*!< 0x00000300 */
#define SPI_I2SCFGR_I2SCFG          SPI_I2SCFGR_I2SCFG_Msk                     /*!<I2SCFG[1:0] bits (I2S configuration mode) */
#define SPI_I2SCFGR_I2SCFG_0        (0x1U << SPI_I2SCFGR_I2SCFG_Pos)           /*!< 0x00000100 */
#define SPI_I2SCFGR_I2SCFG_1        (0x2U << SPI_I2SCFGR_I2SCFG_Pos)           /*!< 0x00000200 */
#define SPI_I2SCFGR_I2SE_Pos        (10U)                                      
#define SPI_I2SCFGR_I2SE_Msk        (0x1U << SPI_I2SCFGR_I2SE_Pos)             /*!< 0x00000400 */
#define SPI_I2SCFGR_I2SE            SPI_I2SCFGR_I2SE_Msk                       /*!<I2S Enable */
#define SPI_I2SCFGR_I2SMOD_Pos      (11U)                                      
#define SPI_I2SCFGR_I2SMOD_Msk      (0x1U << SPI_I2SCFGR_I2SMOD_Pos)           /*!< 0x00000800 */
#define SPI_I2SCFGR_I2SMOD          SPI_I2SCFGR_I2SMOD_Msk                     /*!<I2S mode selection */
/******************  Bit definition for SPI_I2SPR register  *******************/
#define SPI_I2SPR_I2SDIV_Pos        (0U)                                       
#define SPI_I2SPR_I2SDIV_Msk        (0xFFU << SPI_I2SPR_I2SDIV_Pos)            /*!< 0x000000FF */
#define SPI_I2SPR_I2SDIV            SPI_I2SPR_I2SDIV_Msk                       /*!<I2S Linear prescaler */
#define SPI_I2SPR_ODD_Pos           (8U)                                       
#define SPI_I2SPR_ODD_Msk           (0x1U << SPI_I2SPR_ODD_Pos)                /*!< 0x00000100 */
#define SPI_I2SPR_ODD               SPI_I2SPR_ODD_Msk                          /*!<Odd factor for the prescaler */
#define SPI_I2SPR_MCKOE_Pos         (9U)                                       
#define SPI_I2SPR_MCKOE_Msk         (0x1U << SPI_I2SPR_MCKOE_Pos)              /*!< 0x00000200 */
#define SPI_I2SPR_MCKOE             SPI_I2SPR_MCKOE_Msk                        /*!<Master Clock Output Enable */

/******************************************************************************/
/*                                                                            */
/*                       System Configuration (SYSCFG)                        */
/*                                                                            */
/******************************************************************************/
/*****************  Bit definition for SYSCFG_CFGR1 register  ****************/
#define SYSCFG_CFGR1_MEM_MODE_Pos                (0U)                          
#define SYSCFG_CFGR1_MEM_MODE_Msk                (0x3U << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000003 */
#define SYSCFG_CFGR1_MEM_MODE                    SYSCFG_CFGR1_MEM_MODE_Msk     /*!< SYSCFG_Memory Remap Config */
#define SYSCFG_CFGR1_MEM_MODE_0                  (0x1U << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000001 */
#define SYSCFG_CFGR1_MEM_MODE_1                  (0x2U << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000002 */
#define SYSCFG_CFGR1_BOOT_MODE_Pos               (8U)                          
#define SYSCFG_CFGR1_BOOT_MODE_Msk               (0x3U << SYSCFG_CFGR1_BOOT_MODE_Pos) /*!< 0x00000300 */
#define SYSCFG_CFGR1_BOOT_MODE                   SYSCFG_CFGR1_BOOT_MODE_Msk    /*!< SYSCFG_Boot mode Config */
#define SYSCFG_CFGR1_BOOT_MODE_0                 (0x1U << SYSCFG_CFGR1_BOOT_MODE_Pos) /*!< 0x00000100 */
#define SYSCFG_CFGR1_BOOT_MODE_1                 (0x2U << SYSCFG_CFGR1_BOOT_MODE_Pos) /*!< 0x00000200 */

/*****************  Bit definition for SYSCFG_CFGR2 register  ****************/
#define SYSCFG_CFGR2_FWDISEN_Pos                 (0U)                          
#define SYSCFG_CFGR2_FWDISEN_Msk                 (0x1U << SYSCFG_CFGR2_FWDISEN_Pos) /*!< 0x00000001 */
#define SYSCFG_CFGR2_FWDISEN                     SYSCFG_CFGR2_FWDISEN_Msk      /*!< Firewall disable bit */
#define SYSCFG_CFGR2_I2C_PB6_FMP_Pos             (8U)                          
#define SYSCFG_CFGR2_I2C_PB6_FMP_Msk             (0x1U << SYSCFG_CFGR2_I2C_PB6_FMP_Pos) /*!< 0x00000100 */
#define SYSCFG_CFGR2_I2C_PB6_FMP                 SYSCFG_CFGR2_I2C_PB6_FMP_Msk  /*!< I2C PB6 Fast mode plus */
#define SYSCFG_CFGR2_I2C_PB7_FMP_Pos             (9U)                          
#define SYSCFG_CFGR2_I2C_PB7_FMP_Msk             (0x1U << SYSCFG_CFGR2_I2C_PB7_FMP_Pos) /*!< 0x00000200 */
#define SYSCFG_CFGR2_I2C_PB7_FMP                 SYSCFG_CFGR2_I2C_PB7_FMP_Msk  /*!< I2C PB7 Fast mode plus */
#define SYSCFG_CFGR2_I2C_PB8_FMP_Pos             (10U)                         
#define SYSCFG_CFGR2_I2C_PB8_FMP_Msk             (0x1U << SYSCFG_CFGR2_I2C_PB8_FMP_Pos) /*!< 0x00000400 */
#define SYSCFG_CFGR2_I2C_PB8_FMP                 SYSCFG_CFGR2_I2C_PB8_FMP_Msk  /*!< I2C PB8 Fast mode plus */
#define SYSCFG_CFGR2_I2C_PB9_FMP_Pos             (11U)                         
#define SYSCFG_CFGR2_I2C_PB9_FMP_Msk             (0x1U << SYSCFG_CFGR2_I2C_PB9_FMP_Pos) /*!< 0x00000800 */
#define SYSCFG_CFGR2_I2C_PB9_FMP                 SYSCFG_CFGR2_I2C_PB9_FMP_Msk  /*!< I2C PB9 Fast mode plus */
#define SYSCFG_CFGR2_I2C1_FMP_Pos                (12U)                         
#define SYSCFG_CFGR2_I2C1_FMP_Msk                (0x1U << SYSCFG_CFGR2_I2C1_FMP_Pos) /*!< 0x00001000 */
#define SYSCFG_CFGR2_I2C1_FMP                    SYSCFG_CFGR2_I2C1_FMP_Msk     /*!< I2C1 Fast mode plus */
#define SYSCFG_CFGR2_I2C2_FMP_Pos                (13U)                         
#define SYSCFG_CFGR2_I2C2_FMP_Msk                (0x1U << SYSCFG_CFGR2_I2C2_FMP_Pos) /*!< 0x00002000 */
#define SYSCFG_CFGR2_I2C2_FMP                    SYSCFG_CFGR2_I2C2_FMP_Msk     /*!< I2C2 Fast mode plus */

/*****************  Bit definition for SYSCFG_EXTICR1 register  ***************/
#define SYSCFG_EXTICR1_EXTI0_Pos                 (0U)                          
#define SYSCFG_EXTICR1_EXTI0_Msk                 (0xFU << SYSCFG_EXTICR1_EXTI0_Pos) /*!< 0x0000000F */
#define SYSCFG_EXTICR1_EXTI0                     SYSCFG_EXTICR1_EXTI0_Msk      /*!< EXTI 0 configuration */
#define SYSCFG_EXTICR1_EXTI1_Pos                 (4U)                          
#define SYSCFG_EXTICR1_EXTI1_Msk                 (0xFU << SYSCFG_EXTICR1_EXTI1_Pos) /*!< 0x000000F0 */
#define SYSCFG_EXTICR1_EXTI1                     SYSCFG_EXTICR1_EXTI1_Msk      /*!< EXTI 1 configuration */
#define SYSCFG_EXTICR1_EXTI2_Pos                 (8U)                          
#define SYSCFG_EXTICR1_EXTI2_Msk                 (0xFU << SYSCFG_EXTICR1_EXTI2_Pos) /*!< 0x00000F00 */
#define SYSCFG_EXTICR1_EXTI2                     SYSCFG_EXTICR1_EXTI2_Msk      /*!< EXTI 2 configuration */
#define SYSCFG_EXTICR1_EXTI3_Pos                 (12U)                         
#define SYSCFG_EXTICR1_EXTI3_Msk                 (0xFU << SYSCFG_EXTICR1_EXTI3_Pos) /*!< 0x0000F000 */
#define SYSCFG_EXTICR1_EXTI3                     SYSCFG_EXTICR1_EXTI3_Msk      /*!< EXTI 3 configuration */

/** 
  * @brief  EXTI0 configuration  
  */
#define SYSCFG_EXTICR1_EXTI0_PA                  (0x00000000U)                 /*!< PA[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PB                  (0x00000001U)                 /*!< PB[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PC                  (0x00000002U)                 /*!< PC[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PH                  (0x00000005U)                 /*!< PH[0] pin */

/** 
  * @brief  EXTI1 configuration  
  */ 
#define SYSCFG_EXTICR1_EXTI1_PA                  (0x00000000U)                 /*!< PA[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PB                  (0x00000010U)                 /*!< PB[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PC                  (0x00000020U)                 /*!< PC[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PH                  (0x00000050U)                 /*!< PH[1] pin */

/** 
  * @brief  EXTI2 configuration  
  */
#define SYSCFG_EXTICR1_EXTI2_PA                  (0x00000000U)                 /*!< PA[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PB                  (0x00000100U)                 /*!< PB[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PC                  (0x00000200U)                 /*!< PC[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PD                  (0x00000300U)                 /*!< PD[2] pin */

/** 
  * @brief  EXTI3 configuration  
  */
#define SYSCFG_EXTICR1_EXTI3_PA                  (0x00000000U)                 /*!< PA[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PB                  (0x00001000U)                 /*!< PB[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PC                  (0x00002000U)                 /*!< PC[3] pin */

/*****************  Bit definition for SYSCFG_EXTICR2 register  *****************/
#define SYSCFG_EXTICR2_EXTI4_Pos                 (0U)                          
#define SYSCFG_EXTICR2_EXTI4_Msk                 (0xFU << SYSCFG_EXTICR2_EXTI4_Pos) /*!< 0x0000000F */
#define SYSCFG_EXTICR2_EXTI4                     SYSCFG_EXTICR2_EXTI4_Msk      /*!< EXTI 4 configuration */
#define SYSCFG_EXTICR2_EXTI5_Pos                 (4U)                          
#define SYSCFG_EXTICR2_EXTI5_Msk                 (0xFU << SYSCFG_EXTICR2_EXTI5_Pos) /*!< 0x000000F0 */
#define SYSCFG_EXTICR2_EXTI5                     SYSCFG_EXTICR2_EXTI5_Msk      /*!< EXTI 5 configuration */
#define SYSCFG_EXTICR2_EXTI6_Pos                 (8U)                          
#define SYSCFG_EXTICR2_EXTI6_Msk                 (0xFU << SYSCFG_EXTICR2_EXTI6_Pos) /*!< 0x00000F00 */
#define SYSCFG_EXTICR2_EXTI6                     SYSCFG_EXTICR2_EXTI6_Msk      /*!< EXTI 6 configuration */
#define SYSCFG_EXTICR2_EXTI7_Pos                 (12U)                         
#define SYSCFG_EXTICR2_EXTI7_Msk                 (0xFU << SYSCFG_EXTICR2_EXTI7_Pos) /*!< 0x0000F000 */
#define SYSCFG_EXTICR2_EXTI7                     SYSCFG_EXTICR2_EXTI7_Msk      /*!< EXTI 7 configuration */

/** 
  * @brief  EXTI4 configuration  
  */
#define SYSCFG_EXTICR2_EXTI4_PA                  (0x00000000U)                 /*!< PA[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PB                  (0x00000001U)                 /*!< PB[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PC                  (0x00000002U)                 /*!< PC[4] pin */

/** 
  * @brief  EXTI5 configuration  
  */
#define SYSCFG_EXTICR2_EXTI5_PA                  (0x00000000U)                 /*!< PA[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PB                  (0x00000010U)                 /*!< PB[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PC                  (0x00000020U)                 /*!< PC[5] pin */

/** 
  * @brief  EXTI6 configuration  
  */
#define SYSCFG_EXTICR2_EXTI6_PA                  (0x00000000U)                 /*!< PA[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PB                  (0x00000100U)                 /*!< PB[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PC                  (0x00000200U)                 /*!< PC[6] pin */

/** 
  * @brief  EXTI7 configuration  
  */
#define SYSCFG_EXTICR2_EXTI7_PA                  (0x00000000U)                 /*!< PA[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PB                  (0x00001000U)                 /*!< PB[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PC                  (0x00002000U)                 /*!< PC[7] pin */

/*****************  Bit definition for SYSCFG_EXTICR3 register  *****************/
#define SYSCFG_EXTICR3_EXTI8_Pos                 (0U)                          
#define SYSCFG_EXTICR3_EXTI8_Msk                 (0xFU << SYSCFG_EXTICR3_EXTI8_Pos) /*!< 0x0000000F */
#define SYSCFG_EXTICR3_EXTI8                     SYSCFG_EXTICR3_EXTI8_Msk      /*!< EXTI 8 configuration */
#define SYSCFG_EXTICR3_EXTI9_Pos                 (4U)                          
#define SYSCFG_EXTICR3_EXTI9_Msk                 (0xFU << SYSCFG_EXTICR3_EXTI9_Pos) /*!< 0x000000F0 */
#define SYSCFG_EXTICR3_EXTI9                     SYSCFG_EXTICR3_EXTI9_Msk      /*!< EXTI 9 configuration */
#define SYSCFG_EXTICR3_EXTI10_Pos                (8U)                          
#define SYSCFG_EXTICR3_EXTI10_Msk                (0xFU << SYSCFG_EXTICR3_EXTI10_Pos) /*!< 0x00000F00 */
#define SYSCFG_EXTICR3_EXTI10                    SYSCFG_EXTICR3_EXTI10_Msk     /*!< EXTI 10 configuration */
#define SYSCFG_EXTICR3_EXTI11_Pos                (12U)                         
#define SYSCFG_EXTICR3_EXTI11_Msk                (0xFU << SYSCFG_EXTICR3_EXTI11_Pos) /*!< 0x0000F000 */
#define SYSCFG_EXTICR3_EXTI11                    SYSCFG_EXTICR3_EXTI11_Msk     /*!< EXTI 11 configuration */

/** 
  * @brief  EXTI8 configuration  
  */
#define SYSCFG_EXTICR3_EXTI8_PA                  (0x00000000U)                 /*!< PA[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PB                  (0x00000001U)                 /*!< PB[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PC                  (0x00000002U)                 /*!< PC[8] pin */

/** 
  * @brief  EXTI9 configuration  
  */
#define SYSCFG_EXTICR3_EXTI9_PA                  (0x00000000U)                 /*!< PA[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PB                  (0x00000010U)                 /*!< PB[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PC                  (0x00000020U)                 /*!< PC[9] pin */

/** 
  * @brief  EXTI10 configuration  
  */
#define SYSCFG_EXTICR3_EXTI10_PA                 (0x00000000U)                 /*!< PA[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PB                 (0x00000100U)                 /*!< PB[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PC                 (0x00000200U)                 /*!< PC[10] pin */

/** 
  * @brief  EXTI11 configuration  
  */
#define SYSCFG_EXTICR3_EXTI11_PA                 (0x00000000U)                 /*!< PA[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PB                 (0x00001000U)                 /*!< PB[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PC                 (0x00002000U)                 /*!< PC[11] pin */

/*****************  Bit definition for SYSCFG_EXTICR4 register  *****************/
#define SYSCFG_EXTICR4_EXTI12_Pos                (0U)                          
#define SYSCFG_EXTICR4_EXTI12_Msk                (0xFU << SYSCFG_EXTICR4_EXTI12_Pos) /*!< 0x0000000F */
#define SYSCFG_EXTICR4_EXTI12                    SYSCFG_EXTICR4_EXTI12_Msk     /*!< EXTI 12 configuration */
#define SYSCFG_EXTICR4_EXTI13_Pos                (4U)                          
#define SYSCFG_EXTICR4_EXTI13_Msk                (0xFU << SYSCFG_EXTICR4_EXTI13_Pos) /*!< 0x000000F0 */
#define SYSCFG_EXTICR4_EXTI13                    SYSCFG_EXTICR4_EXTI13_Msk     /*!< EXTI 13 configuration */
#define SYSCFG_EXTICR4_EXTI14_Pos                (8U)                          
#define SYSCFG_EXTICR4_EXTI14_Msk                (0xFU << SYSCFG_EXTICR4_EXTI14_Pos) /*!< 0x00000F00 */
#define SYSCFG_EXTICR4_EXTI14                    SYSCFG_EXTICR4_EXTI14_Msk     /*!< EXTI 14 configuration */
#define SYSCFG_EXTICR4_EXTI15_Pos                (12U)                         
#define SYSCFG_EXTICR4_EXTI15_Msk                (0xFU << SYSCFG_EXTICR4_EXTI15_Pos) /*!< 0x0000F000 */
#define SYSCFG_EXTICR4_EXTI15                    SYSCFG_EXTICR4_EXTI15_Msk     /*!< EXTI 15 configuration */

/** 
  * @brief  EXTI12 configuration  
  */
#define SYSCFG_EXTICR4_EXTI12_PA                 (0x00000000U)                 /*!< PA[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PB                 (0x00000001U)                 /*!< PB[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PC                 (0x00000002U)                 /*!< PC[12] pin */

/** 
  * @brief  EXTI13 configuration  
  */
#define SYSCFG_EXTICR4_EXTI13_PA                 (0x00000000U)                 /*!< PA[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PB                 (0x00000010U)                 /*!< PB[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PC                 (0x00000020U)                 /*!< PC[13] pin */

/** 
  * @brief  EXTI14 configuration  
  */
#define SYSCFG_EXTICR4_EXTI14_PA                 (0x00000000U)                 /*!< PA[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PB                 (0x00000100U)                 /*!< PB[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PC                 (0x00000200U)                 /*!< PC[14] pin */

/** 
  * @brief  EXTI15 configuration  
  */
#define SYSCFG_EXTICR4_EXTI15_PA                 (0x00000000U)                 /*!< PA[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PB                 (0x00001000U)                 /*!< PB[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PC                 (0x00002000U)                 /*!< PC[15] pin */


/*****************  Bit definition for SYSCFG_CFGR3 register  ****************/
#define SYSCFG_CFGR3_VREF_OUT_Pos                (4U)                          
#define SYSCFG_CFGR3_VREF_OUT_Msk                (0x3U << SYSCFG_CFGR3_VREF_OUT_Pos) /*!< 0x00000030 */
#define SYSCFG_CFGR3_VREF_OUT                    SYSCFG_CFGR3_VREF_OUT_Msk     /*!< Verf_ADC connection bit */
#define SYSCFG_CFGR3_VREF_OUT_0                  (0x1U << SYSCFG_CFGR3_VREF_OUT_Pos) /*!< 0x00000010 */
#define SYSCFG_CFGR3_VREF_OUT_1                  (0x2U << SYSCFG_CFGR3_VREF_OUT_Pos) /*!< 0x00000020 */
#define SYSCFG_CFGR3_ENBUF_VREFINT_ADC_Pos       (8U)                          
#define SYSCFG_CFGR3_ENBUF_VREFINT_ADC_Msk       (0x1U << SYSCFG_CFGR3_ENBUF_VREFINT_ADC_Pos) /*!< 0x00000100 */
#define SYSCFG_CFGR3_ENBUF_VREFINT_ADC           SYSCFG_CFGR3_ENBUF_VREFINT_ADC_Msk /*!< VREFINT reference for ADC enable bit */
#define SYSCFG_CFGR3_ENBUF_SENSOR_ADC_Pos        (9U)                          
#define SYSCFG_CFGR3_ENBUF_SENSOR_ADC_Msk        (0x1U << SYSCFG_CFGR3_ENBUF_SENSOR_ADC_Pos) /*!< 0x00000200 */
#define SYSCFG_CFGR3_ENBUF_SENSOR_ADC            SYSCFG_CFGR3_ENBUF_SENSOR_ADC_Msk /*!< Sensor reference for ADC enable bit */
#define SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP_Pos    (12U)                         
#define SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP_Msk    (0x1U << SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP_Pos) /*!< 0x00001000 */
#define SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP        SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP_Msk /*!< VREFINT reference for comparator 2 enable bit */
#define SYSCFG_CFGR3_VREFINT_RDYF_Pos            (30U)                         
#define SYSCFG_CFGR3_VREFINT_RDYF_Msk            (0x1U << SYSCFG_CFGR3_VREFINT_RDYF_Pos) /*!< 0x40000000 */
#define SYSCFG_CFGR3_VREFINT_RDYF                SYSCFG_CFGR3_VREFINT_RDYF_Msk /*!< VREFINT ready flag */
#define SYSCFG_CFGR3_REF_LOCK_Pos                (31U)                         
#define SYSCFG_CFGR3_REF_LOCK_Msk                (0x1U << SYSCFG_CFGR3_REF_LOCK_Pos) /*!< 0x80000000 */
#define SYSCFG_CFGR3_REF_LOCK                    SYSCFG_CFGR3_REF_LOCK_Msk     /*!< CFGR3 lock bit */

/* Legacy defines */

#define SYSCFG_CFGR3_ENBUF_BGAP_ADC           SYSCFG_CFGR3_ENBUF_VREFINT_ADC
#define SYSCFG_CFGR3_ENBUFLP_BGAP_COMP        SYSCFG_CFGR3_ENBUFLP_VREFINT_COMP
#define SYSCFG_VREFINT_ADC_RDYF               SYSCFG_CFGR3_VREFINT_RDYF
#define SYSCFG_CFGR3_SENSOR_ADC_RDYF          SYSCFG_CFGR3_VREFINT_RDYF
#define SYSCFG_CFGR3_VREFINT_ADC_RDYF         SYSCFG_CFGR3_VREFINT_RDYF
#define SYSCFG_CFGR3_VREFINT_COMP_RDYF        SYSCFG_CFGR3_VREFINT_RDYF

/******************************************************************************/
/*                                                                            */
/*                               Timers (TIM)                                 */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions (not present on all devices in the STM32L0 family)
*/
#if defined (STM32L071xx) || defined (STM32L072xx) || defined (STM32L073xx) \
    || defined (STM32L081xx) || defined (STM32L082xx) || defined (STM32L083xx)
#define TIM_TIM2_REMAP_HSI_SUPPORT       /*!<Support remap HSI on TIM2 */
#define TIM_TIM2_REMAP_HSI48_SUPPORT     /*!<Support remap HSI48 on TIM2 */
#else
#define TIM_TIM2_REMAP_HSI48_SUPPORT     /*!<Support remap HSI48 on TIM2 */
#endif	

/*******************  Bit definition for TIM_CR1 register  ********************/
#define TIM_CR1_CEN_Pos           (0U)                                         
#define TIM_CR1_CEN_Msk           (0x1U << TIM_CR1_CEN_Pos)                    /*!< 0x00000001 */
#define TIM_CR1_CEN               TIM_CR1_CEN_Msk                              /*!<Counter enable */
#define TIM_CR1_UDIS_Pos          (1U)                                         
#define TIM_CR1_UDIS_Msk          (0x1U << TIM_CR1_UDIS_Pos)                   /*!< 0x00000002 */
#define TIM_CR1_UDIS              TIM_CR1_UDIS_Msk                             /*!<Update disable */
#define TIM_CR1_URS_Pos           (2U)                                         
#define TIM_CR1_URS_Msk           (0x1U << TIM_CR1_URS_Pos)                    /*!< 0x00000004 */
#define TIM_CR1_URS               TIM_CR1_URS_Msk                              /*!<Update request source */
#define TIM_CR1_OPM_Pos           (3U)                                         
#define TIM_CR1_OPM_Msk           (0x1U << TIM_CR1_OPM_Pos)                    /*!< 0x00000008 */
#define TIM_CR1_OPM               TIM_CR1_OPM_Msk                              /*!<One pulse mode */
#define TIM_CR1_DIR_Pos           (4U)                                         
#define TIM_CR1_DIR_Msk           (0x1U << TIM_CR1_DIR_Pos)                    /*!< 0x00000010 */
#define TIM_CR1_DIR               TIM_CR1_DIR_Msk                              /*!<Direction */

#define TIM_CR1_CMS_Pos           (5U)                                         
#define TIM_CR1_CMS_Msk           (0x3U << TIM_CR1_CMS_Pos)                    /*!< 0x00000060 */
#define TIM_CR1_CMS               TIM_CR1_CMS_Msk                              /*!<CMS[1:0] bits (Center-aligned mode selection) */
#define TIM_CR1_CMS_0             (0x1U << TIM_CR1_CMS_Pos)                    /*!< 0x00000020 */
#define TIM_CR1_CMS_1             (0x2U << TIM_CR1_CMS_Pos)                    /*!< 0x00000040 */

#define TIM_CR1_ARPE_Pos          (7U)                                         
#define TIM_CR1_ARPE_Msk          (0x1U << TIM_CR1_ARPE_Pos)                   /*!< 0x00000080 */
#define TIM_CR1_ARPE              TIM_CR1_ARPE_Msk                             /*!<Auto-reload preload enable */

#define TIM_CR1_CKD_Pos           (8U)                                         
#define TIM_CR1_CKD_Msk           (0x3U << TIM_CR1_CKD_Pos)                    /*!< 0x00000300 */
#define TIM_CR1_CKD               TIM_CR1_CKD_Msk                              /*!<CKD[1:0] bits (clock division) */
#define TIM_CR1_CKD_0             (0x1U << TIM_CR1_CKD_Pos)                    /*!< 0x00000100 */
#define TIM_CR1_CKD_1             (0x2U << TIM_CR1_CKD_Pos)                    /*!< 0x00000200 */

/*******************  Bit definition for TIM_CR2 register  ********************/
#define TIM_CR2_CCDS_Pos          (3U)                                         
#define TIM_CR2_CCDS_Msk          (0x1U << TIM_CR2_CCDS_Pos)                   /*!< 0x00000008 */
#define TIM_CR2_CCDS              TIM_CR2_CCDS_Msk                             /*!<Capture/Compare DMA Selection */

#define TIM_CR2_MMS_Pos           (4U)                                         
#define TIM_CR2_MMS_Msk           (0x7U << TIM_CR2_MMS_Pos)                    /*!< 0x00000070 */
#define TIM_CR2_MMS               TIM_CR2_MMS_Msk                              /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS_0             (0x1U << TIM_CR2_MMS_Pos)                    /*!< 0x00000010 */
#define TIM_CR2_MMS_1             (0x2U << TIM_CR2_MMS_Pos)                    /*!< 0x00000020 */
#define TIM_CR2_MMS_2             (0x4U << TIM_CR2_MMS_Pos)                    /*!< 0x00000040 */

#define TIM_CR2_TI1S_Pos          (7U)                                         
#define TIM_CR2_TI1S_Msk          (0x1U << TIM_CR2_TI1S_Pos)                   /*!< 0x00000080 */
#define TIM_CR2_TI1S              TIM_CR2_TI1S_Msk                             /*!<TI1 Selection */

/*******************  Bit definition for TIM_SMCR register  *******************/
#define TIM_SMCR_SMS_Pos          (0U)                                         
#define TIM_SMCR_SMS_Msk          (0x7U << TIM_SMCR_SMS_Pos)                   /*!< 0x00000007 */
#define TIM_SMCR_SMS              TIM_SMCR_SMS_Msk                             /*!<SMS[2:0] bits (Slave mode selection) */
#define TIM_SMCR_SMS_0            (0x1U << TIM_SMCR_SMS_Pos)                   /*!< 0x00000001 */
#define TIM_SMCR_SMS_1            (0x2U << TIM_SMCR_SMS_Pos)                   /*!< 0x00000002 */
#define TIM_SMCR_SMS_2            (0x4U << TIM_SMCR_SMS_Pos)                   /*!< 0x00000004 */

#define TIM_SMCR_OCCS_Pos         (3U)                                         
#define TIM_SMCR_OCCS_Msk         (0x1U << TIM_SMCR_OCCS_Pos)                  /*!< 0x00000008 */
#define TIM_SMCR_OCCS             TIM_SMCR_OCCS_Msk                            /*!< OCREF clear selection */

#define TIM_SMCR_TS_Pos           (4U)                                         
#define TIM_SMCR_TS_Msk           (0x7U << TIM_SMCR_TS_Pos)                    /*!< 0x00000070 */
#define TIM_SMCR_TS               TIM_SMCR_TS_Msk                              /*!<TS[2:0] bits (Trigger selection) */
#define TIM_SMCR_TS_0             (0x1U << TIM_SMCR_TS_Pos)                    /*!< 0x00000010 */
#define TIM_SMCR_TS_1             (0x2U << TIM_SMCR_TS_Pos)                    /*!< 0x00000020 */
#define TIM_SMCR_TS_2             (0x4U << TIM_SMCR_TS_Pos)                    /*!< 0x00000040 */

#define TIM_SMCR_MSM_Pos          (7U)                                         
#define TIM_SMCR_MSM_Msk          (0x1U << TIM_SMCR_MSM_Pos)                   /*!< 0x00000080 */
#define TIM_SMCR_MSM              TIM_SMCR_MSM_Msk                             /*!<Master/slave mode */

#define TIM_SMCR_ETF_Pos          (8U)                                         
#define TIM_SMCR_ETF_Msk          (0xFU << TIM_SMCR_ETF_Pos)                   /*!< 0x00000F00 */
#define TIM_SMCR_ETF              TIM_SMCR_ETF_Msk                             /*!<ETF[3:0] bits (External trigger filter) */
#define TIM_SMCR_ETF_0            (0x1U << TIM_SMCR_ETF_Pos)                   /*!< 0x00000100 */
#define TIM_SMCR_ETF_1            (0x2U << TIM_SMCR_ETF_Pos)                   /*!< 0x00000200 */
#define TIM_SMCR_ETF_2            (0x4U << TIM_SMCR_ETF_Pos)                   /*!< 0x00000400 */
#define TIM_SMCR_ETF_3            (0x8U << TIM_SMCR_ETF_Pos)                   /*!< 0x00000800 */

#define TIM_SMCR_ETPS_Pos         (12U)                                        
#define TIM_SMCR_ETPS_Msk         (0x3U << TIM_SMCR_ETPS_Pos)                  /*!< 0x00003000 */
#define TIM_SMCR_ETPS             TIM_SMCR_ETPS_Msk                            /*!<ETPS[1:0] bits (External trigger prescaler) */
#define TIM_SMCR_ETPS_0           (0x1U << TIM_SMCR_ETPS_Pos)                  /*!< 0x00001000 */
#define TIM_SMCR_ETPS_1           (0x2U << TIM_SMCR_ETPS_Pos)                  /*!< 0x00002000 */

#define TIM_SMCR_ECE_Pos          (14U)                                        
#define TIM_SMCR_ECE_Msk          (0x1U << TIM_SMCR_ECE_Pos)                   /*!< 0x00004000 */
#define TIM_SMCR_ECE              TIM_SMCR_ECE_Msk                             /*!<External clock enable */
#define TIM_SMCR_ETP_Pos          (15U)                                        
#define TIM_SMCR_ETP_Msk          (0x1U << TIM_SMCR_ETP_Pos)                   /*!< 0x00008000 */
#define TIM_SMCR_ETP              TIM_SMCR_ETP_Msk                             /*!<External trigger polarity */

/*******************  Bit definition for TIM_DIER register  *******************/
#define TIM_DIER_UIE_Pos          (0U)                                         
#define TIM_DIER_UIE_Msk          (0x1U << TIM_DIER_UIE_Pos)                   /*!< 0x00000001 */
#define TIM_DIER_UIE              TIM_DIER_UIE_Msk                             /*!<Update interrupt enable */
#define TIM_DIER_CC1IE_Pos        (1U)                                         
#define TIM_DIER_CC1IE_Msk        (0x1U << TIM_DIER_CC1IE_Pos)                 /*!< 0x00000002 */
#define TIM_DIER_CC1IE            TIM_DIER_CC1IE_Msk                           /*!<Capture/Compare 1 interrupt enable */
#define TIM_DIER_CC2IE_Pos        (2U)                                         
#define TIM_DIER_CC2IE_Msk        (0x1U << TIM_DIER_CC2IE_Pos)                 /*!< 0x00000004 */
#define TIM_DIER_CC2IE            TIM_DIER_CC2IE_Msk                           /*!<Capture/Compare 2 interrupt enable */
#define TIM_DIER_CC3IE_Pos        (3U)                                         
#define TIM_DIER_CC3IE_Msk        (0x1U << TIM_DIER_CC3IE_Pos)                 /*!< 0x00000008 */
#define TIM_DIER_CC3IE            TIM_DIER_CC3IE_Msk                           /*!<Capture/Compare 3 interrupt enable */
#define TIM_DIER_CC4IE_Pos        (4U)                                         
#define TIM_DIER_CC4IE_Msk        (0x1U << TIM_DIER_CC4IE_Pos)                 /*!< 0x00000010 */
#define TIM_DIER_CC4IE            TIM_DIER_CC4IE_Msk                           /*!<Capture/Compare 4 interrupt enable */
#define TIM_DIER_TIE_Pos          (6U)                                         
#define TIM_DIER_TIE_Msk          (0x1U << TIM_DIER_TIE_Pos)                   /*!< 0x00000040 */
#define TIM_DIER_TIE              TIM_DIER_TIE_Msk                             /*!<Trigger interrupt enable */
#define TIM_DIER_UDE_Pos          (8U)                                         
#define TIM_DIER_UDE_Msk          (0x1U << TIM_DIER_UDE_Pos)                   /*!< 0x00000100 */
#define TIM_DIER_UDE              TIM_DIER_UDE_Msk                             /*!<Update DMA request enable */
#define TIM_DIER_CC1DE_Pos        (9U)                                         
#define TIM_DIER_CC1DE_Msk        (0x1U << TIM_DIER_CC1DE_Pos)                 /*!< 0x00000200 */
#define TIM_DIER_CC1DE            TIM_DIER_CC1DE_Msk                           /*!<Capture/Compare 1 DMA request enable */
#define TIM_DIER_CC2DE_Pos        (10U)                                        
#define TIM_DIER_CC2DE_Msk        (0x1U << TIM_DIER_CC2DE_Pos)                 /*!< 0x00000400 */
#define TIM_DIER_CC2DE            TIM_DIER_CC2DE_Msk                           /*!<Capture/Compare 2 DMA request enable */
#define TIM_DIER_CC3DE_Pos        (11U)                                        
#define TIM_DIER_CC3DE_Msk        (0x1U << TIM_DIER_CC3DE_Pos)                 /*!< 0x00000800 */
#define TIM_DIER_CC3DE            TIM_DIER_CC3DE_Msk                           /*!<Capture/Compare 3 DMA request enable */
#define TIM_DIER_CC4DE_Pos        (12U)                                        
#define TIM_DIER_CC4DE_Msk        (0x1U << TIM_DIER_CC4DE_Pos)                 /*!< 0x00001000 */
#define TIM_DIER_CC4DE            TIM_DIER_CC4DE_Msk                           /*!<Capture/Compare 4 DMA request enable */
#define TIM_DIER_TDE_Pos          (14U)                                        
#define TIM_DIER_TDE_Msk          (0x1U << TIM_DIER_TDE_Pos)                   /*!< 0x00004000 */
#define TIM_DIER_TDE              TIM_DIER_TDE_Msk                             /*!<Trigger DMA request enable */

/********************  Bit definition for TIM_SR register  ********************/
#define TIM_SR_UIF_Pos            (0U)                                         
#define TIM_SR_UIF_Msk            (0x1U << TIM_SR_UIF_Pos)                     /*!< 0x00000001 */
#define TIM_SR_UIF                TIM_SR_UIF_Msk                               /*!<Update interrupt Flag */
#define TIM_SR_CC1IF_Pos          (1U)                                         
#define TIM_SR_CC1IF_Msk          (0x1U << TIM_SR_CC1IF_Pos)                   /*!< 0x00000002 */
#define TIM_SR_CC1IF              TIM_SR_CC1IF_Msk                             /*!<Capture/Compare 1 interrupt Flag */
#define TIM_SR_CC2IF_Pos          (2U)                                         
#define TIM_SR_CC2IF_Msk          (0x1U << TIM_SR_CC2IF_Pos)                   /*!< 0x00000004 */
#define TIM_SR_CC2IF              TIM_SR_CC2IF_Msk                             /*!<Capture/Compare 2 interrupt Flag */
#define TIM_SR_CC3IF_Pos          (3U)                                         
#define TIM_SR_CC3IF_Msk          (0x1U << TIM_SR_CC3IF_Pos)                   /*!< 0x00000008 */
#define TIM_SR_CC3IF              TIM_SR_CC3IF_Msk                             /*!<Capture/Compare 3 interrupt Flag */
#define TIM_SR_CC4IF_Pos          (4U)                                         
#define TIM_SR_CC4IF_Msk          (0x1U << TIM_SR_CC4IF_Pos)                   /*!< 0x00000010 */
#define TIM_SR_CC4IF              TIM_SR_CC4IF_Msk                             /*!<Capture/Compare 4 interrupt Flag */
#define TIM_SR_TIF_Pos            (6U)                                         
#define TIM_SR_TIF_Msk            (0x1U << TIM_SR_TIF_Pos)                     /*!< 0x00000040 */
#define TIM_SR_TIF                TIM_SR_TIF_Msk                               /*!<Trigger interrupt Flag */
#define TIM_SR_CC1OF_Pos          (9U)                                         
#define TIM_SR_CC1OF_Msk          (0x1U << TIM_SR_CC1OF_Pos)                   /*!< 0x00000200 */
#define TIM_SR_CC1OF              TIM_SR_CC1OF_Msk                             /*!<Capture/Compare 1 Overcapture Flag */
#define TIM_SR_CC2OF_Pos          (10U)                                        
#define TIM_SR_CC2OF_Msk          (0x1U << TIM_SR_CC2OF_Pos)                   /*!< 0x00000400 */
#define TIM_SR_CC2OF              TIM_SR_CC2OF_Msk                             /*!<Capture/Compare 2 Overcapture Flag */
#define TIM_SR_CC3OF_Pos          (11U)                                        
#define TIM_SR_CC3OF_Msk          (0x1U << TIM_SR_CC3OF_Pos)                   /*!< 0x00000800 */
#define TIM_SR_CC3OF              TIM_SR_CC3OF_Msk                             /*!<Capture/Compare 3 Overcapture Flag */
#define TIM_SR_CC4OF_Pos          (12U)                                        
#define TIM_SR_CC4OF_Msk          (0x1U << TIM_SR_CC4OF_Pos)                   /*!< 0x00001000 */
#define TIM_SR_CC4OF              TIM_SR_CC4OF_Msk                             /*!<Capture/Compare 4 Overcapture Flag */

/*******************  Bit definition for TIM_EGR register  ********************/
#define TIM_EGR_UG_Pos            (0U)                                         
#define TIM_EGR_UG_Msk            (0x1U << TIM_EGR_UG_Pos)                     /*!< 0x00000001 */
#define TIM_EGR_UG                TIM_EGR_UG_Msk                               /*!<Update Generation */
#define TIM_EGR_CC1G_Pos          (1U)                                         
#define TIM_EGR_CC1G_Msk          (0x1U << TIM_EGR_CC1G_Pos)                   /*!< 0x00000002 */
#define TIM_EGR_CC1G              TIM_EGR_CC1G_Msk                             /*!<Capture/Compare 1 Generation */
#define TIM_EGR_CC2G_Pos          (2U)                                         
#define TIM_EGR_CC2G_Msk          (0x1U << TIM_EGR_CC2G_Pos)                   /*!< 0x00000004 */
#define TIM_EGR_CC2G              TIM_EGR_CC2G_Msk                             /*!<Capture/Compare 2 Generation */
#define TIM_EGR_CC3G_Pos          (3U)                                         
#define TIM_EGR_CC3G_Msk          (0x1U << TIM_EGR_CC3G_Pos)                   /*!< 0x00000008 */
#define TIM_EGR_CC3G              TIM_EGR_CC3G_Msk                             /*!<Capture/Compare 3 Generation */
#define TIM_EGR_CC4G_Pos          (4U)                                         
#define TIM_EGR_CC4G_Msk          (0x1U << TIM_EGR_CC4G_Pos)                   /*!< 0x00000010 */
#define TIM_EGR_CC4G              TIM_EGR_CC4G_Msk                             /*!<Capture/Compare 4 Generation */
#define TIM_EGR_TG_Pos            (6U)                                         
#define TIM_EGR_TG_Msk            (0x1U << TIM_EGR_TG_Pos)                     /*!< 0x00000040 */
#define TIM_EGR_TG                TIM_EGR_TG_Msk                               /*!<Trigger Generation */

/******************  Bit definition for TIM_CCMR1 register  *******************/
#define TIM_CCMR1_CC1S_Pos        (0U)                                         
#define TIM_CCMR1_CC1S_Msk        (0x3U << TIM_CCMR1_CC1S_Pos)                 /*!< 0x00000003 */
#define TIM_CCMR1_CC1S            TIM_CCMR1_CC1S_Msk                           /*!<CC1S[1:0] bits (Capture/Compare 1 Selection) */
#define TIM_CCMR1_CC1S_0          (0x1U << TIM_CCMR1_CC1S_Pos)                 /*!< 0x00000001 */
#define TIM_CCMR1_CC1S_1          (0x2U << TIM_CCMR1_CC1S_Pos)                 /*!< 0x00000002 */

#define TIM_CCMR1_OC1FE_Pos       (2U)                                         
#define TIM_CCMR1_OC1FE_Msk       (0x1U << TIM_CCMR1_OC1FE_Pos)                /*!< 0x00000004 */
#define TIM_CCMR1_OC1FE           TIM_CCMR1_OC1FE_Msk                          /*!<Output Compare 1 Fast enable */
#define TIM_CCMR1_OC1PE_Pos       (3U)                                         
#define TIM_CCMR1_OC1PE_Msk       (0x1U << TIM_CCMR1_OC1PE_Pos)                /*!< 0x00000008 */
#define TIM_CCMR1_OC1PE           TIM_CCMR1_OC1PE_Msk                          /*!<Output Compare 1 Preload enable */

#define TIM_CCMR1_OC1M_Pos        (4U)                                         
#define TIM_CCMR1_OC1M_Msk        (0x7U << TIM_CCMR1_OC1M_Pos)                 /*!< 0x00000070 */
#define TIM_CCMR1_OC1M            TIM_CCMR1_OC1M_Msk                           /*!<OC1M[2:0] bits (Output Compare 1 Mode) */
#define TIM_CCMR1_OC1M_0          (0x1U << TIM_CCMR1_OC1M_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR1_OC1M_1          (0x2U << TIM_CCMR1_OC1M_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR1_OC1M_2          (0x4U << TIM_CCMR1_OC1M_Pos)                 /*!< 0x00000040 */

#define TIM_CCMR1_OC1CE_Pos       (7U)                                         
#define TIM_CCMR1_OC1CE_Msk       (0x1U << TIM_CCMR1_OC1CE_Pos)                /*!< 0x00000080 */
#define TIM_CCMR1_OC1CE           TIM_CCMR1_OC1CE_Msk                          /*!<Output Compare 1Clear Enable */

#define TIM_CCMR1_CC2S_Pos        (8U)                                         
#define TIM_CCMR1_CC2S_Msk        (0x3U << TIM_CCMR1_CC2S_Pos)                 /*!< 0x00000300 */
#define TIM_CCMR1_CC2S            TIM_CCMR1_CC2S_Msk                           /*!<CC2S[1:0] bits (Capture/Compare 2 Selection) */
#define TIM_CCMR1_CC2S_0          (0x1U << TIM_CCMR1_CC2S_Pos)                 /*!< 0x00000100 */
#define TIM_CCMR1_CC2S_1          (0x2U << TIM_CCMR1_CC2S_Pos)                 /*!< 0x00000200 */

#define TIM_CCMR1_OC2FE_Pos       (10U)                                        
#define TIM_CCMR1_OC2FE_Msk       (0x1U << TIM_CCMR1_OC2FE_Pos)                /*!< 0x00000400 */
#define TIM_CCMR1_OC2FE           TIM_CCMR1_OC2FE_Msk                          /*!<Output Compare 2 Fast enable */
#define TIM_CCMR1_OC2PE_Pos       (11U)                                        
#define TIM_CCMR1_OC2PE_Msk       (0x1U << TIM_CCMR1_OC2PE_Pos)                /*!< 0x00000800 */
#define TIM_CCMR1_OC2PE           TIM_CCMR1_OC2PE_Msk                          /*!<Output Compare 2 Preload enable */

#define TIM_CCMR1_OC2M_Pos        (12U)                                        
#define TIM_CCMR1_OC2M_Msk        (0x7U << TIM_CCMR1_OC2M_Pos)                 /*!< 0x00007000 */
#define TIM_CCMR1_OC2M            TIM_CCMR1_OC2M_Msk                           /*!<OC2M[2:0] bits (Output Compare 2 Mode) */
#define TIM_CCMR1_OC2M_0          (0x1U << TIM_CCMR1_OC2M_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR1_OC2M_1          (0x2U << TIM_CCMR1_OC2M_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR1_OC2M_2          (0x4U << TIM_CCMR1_OC2M_Pos)                 /*!< 0x00004000 */

#define TIM_CCMR1_OC2CE_Pos       (15U)                                        
#define TIM_CCMR1_OC2CE_Msk       (0x1U << TIM_CCMR1_OC2CE_Pos)                /*!< 0x00008000 */
#define TIM_CCMR1_OC2CE           TIM_CCMR1_OC2CE_Msk                          /*!<Output Compare 2 Clear Enable */

/*----------------------------------------------------------------------------*/

#define TIM_CCMR1_IC1PSC_Pos      (2U)                                         
#define TIM_CCMR1_IC1PSC_Msk      (0x3U << TIM_CCMR1_IC1PSC_Pos)               /*!< 0x0000000C */
#define TIM_CCMR1_IC1PSC          TIM_CCMR1_IC1PSC_Msk                         /*!<IC1PSC[1:0] bits (Input Capture 1 Prescaler) */
#define TIM_CCMR1_IC1PSC_0        (0x1U << TIM_CCMR1_IC1PSC_Pos)               /*!< 0x00000004 */
#define TIM_CCMR1_IC1PSC_1        (0x2U << TIM_CCMR1_IC1PSC_Pos)               /*!< 0x00000008 */

#define TIM_CCMR1_IC1F_Pos        (4U)                                         
#define TIM_CCMR1_IC1F_Msk        (0xFU << TIM_CCMR1_IC1F_Pos)                 /*!< 0x000000F0 */
#define TIM_CCMR1_IC1F            TIM_CCMR1_IC1F_Msk                           /*!<IC1F[3:0] bits (Input Capture 1 Filter) */
#define TIM_CCMR1_IC1F_0          (0x1U << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR1_IC1F_1          (0x2U << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR1_IC1F_2          (0x4U << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR1_IC1F_3          (0x8U << TIM_CCMR1_IC1F_Pos)                 /*!< 0x00000080 */

#define TIM_CCMR1_IC2PSC_Pos      (10U)                                        
#define TIM_CCMR1_IC2PSC_Msk      (0x3U << TIM_CCMR1_IC2PSC_Pos)               /*!< 0x00000C00 */
#define TIM_CCMR1_IC2PSC          TIM_CCMR1_IC2PSC_Msk                         /*!<IC2PSC[1:0] bits (Input Capture 2 Prescaler) */
#define TIM_CCMR1_IC2PSC_0        (0x1U << TIM_CCMR1_IC2PSC_Pos)               /*!< 0x00000400 */
#define TIM_CCMR1_IC2PSC_1        (0x2U << TIM_CCMR1_IC2PSC_Pos)               /*!< 0x00000800 */

#define TIM_CCMR1_IC2F_Pos        (12U)                                        
#define TIM_CCMR1_IC2F_Msk        (0xFU << TIM_CCMR1_IC2F_Pos)                 /*!< 0x0000F000 */
#define TIM_CCMR1_IC2F            TIM_CCMR1_IC2F_Msk                           /*!<IC2F[3:0] bits (Input Capture 2 Filter) */
#define TIM_CCMR1_IC2F_0          (0x1U << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR1_IC2F_1          (0x2U << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR1_IC2F_2          (0x4U << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR1_IC2F_3          (0x8U << TIM_CCMR1_IC2F_Pos)                 /*!< 0x00008000 */

/******************  Bit definition for TIM_CCMR2 register  *******************/
#define TIM_CCMR2_CC3S_Pos        (0U)                                         
#define TIM_CCMR2_CC3S_Msk        (0x3U << TIM_CCMR2_CC3S_Pos)                 /*!< 0x00000003 */
#define TIM_CCMR2_CC3S            TIM_CCMR2_CC3S_Msk                           /*!<CC3S[1:0] bits (Capture/Compare 3 Selection) */
#define TIM_CCMR2_CC3S_0          (0x1U << TIM_CCMR2_CC3S_Pos)                 /*!< 0x00000001 */
#define TIM_CCMR2_CC3S_1          (0x2U << TIM_CCMR2_CC3S_Pos)                 /*!< 0x00000002 */

#define TIM_CCMR2_OC3FE_Pos       (2U)                                         
#define TIM_CCMR2_OC3FE_Msk       (0x1U << TIM_CCMR2_OC3FE_Pos)                /*!< 0x00000004 */
#define TIM_CCMR2_OC3FE           TIM_CCMR2_OC3FE_Msk                          /*!<Output Compare 3 Fast enable */
#define TIM_CCMR2_OC3PE_Pos       (3U)                                         
#define TIM_CCMR2_OC3PE_Msk       (0x1U << TIM_CCMR2_OC3PE_Pos)                /*!< 0x00000008 */
#define TIM_CCMR2_OC3PE           TIM_CCMR2_OC3PE_Msk                          /*!<Output Compare 3 Preload enable */

#define TIM_CCMR2_OC3M_Pos        (4U)                                         
#define TIM_CCMR2_OC3M_Msk        (0x7U << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000070 */
#define TIM_CCMR2_OC3M            TIM_CCMR2_OC3M_Msk                           /*!<OC3M[2:0] bits (Output Compare 3 Mode) */
#define TIM_CCMR2_OC3M_0          (0x1U << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR2_OC3M_1          (0x2U << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR2_OC3M_2          (0x4U << TIM_CCMR2_OC3M_Pos)                 /*!< 0x00000040 */

#define TIM_CCMR2_OC3CE_Pos       (7U)                                         
#define TIM_CCMR2_OC3CE_Msk       (0x1U << TIM_CCMR2_OC3CE_Pos)                /*!< 0x00000080 */
#define TIM_CCMR2_OC3CE           TIM_CCMR2_OC3CE_Msk                          /*!<Output Compare 3 Clear Enable */

#define TIM_CCMR2_CC4S_Pos        (8U)                                         
#define TIM_CCMR2_CC4S_Msk        (0x3U << TIM_CCMR2_CC4S_Pos)                 /*!< 0x00000300 */
#define TIM_CCMR2_CC4S            TIM_CCMR2_CC4S_Msk                           /*!<CC4S[1:0] bits (Capture/Compare 4 Selection) */
#define TIM_CCMR2_CC4S_0          (0x1U << TIM_CCMR2_CC4S_Pos)                 /*!< 0x00000100 */
#define TIM_CCMR2_CC4S_1          (0x2U << TIM_CCMR2_CC4S_Pos)                 /*!< 0x00000200 */

#define TIM_CCMR2_OC4FE_Pos       (10U)                                        
#define TIM_CCMR2_OC4FE_Msk       (0x1U << TIM_CCMR2_OC4FE_Pos)                /*!< 0x00000400 */
#define TIM_CCMR2_OC4FE           TIM_CCMR2_OC4FE_Msk                          /*!<Output Compare 4 Fast enable */
#define TIM_CCMR2_OC4PE_Pos       (11U)                                        
#define TIM_CCMR2_OC4PE_Msk       (0x1U << TIM_CCMR2_OC4PE_Pos)                /*!< 0x00000800 */
#define TIM_CCMR2_OC4PE           TIM_CCMR2_OC4PE_Msk                          /*!<Output Compare 4 Preload enable */

#define TIM_CCMR2_OC4M_Pos        (12U)                                        
#define TIM_CCMR2_OC4M_Msk        (0x7U << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00007000 */
#define TIM_CCMR2_OC4M            TIM_CCMR2_OC4M_Msk                           /*!<OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_CCMR2_OC4M_0          (0x1U << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR2_OC4M_1          (0x2U << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR2_OC4M_2          (0x4U << TIM_CCMR2_OC4M_Pos)                 /*!< 0x00004000 */

#define TIM_CCMR2_OC4CE_Pos       (15U)                                        
#define TIM_CCMR2_OC4CE_Msk       (0x1U << TIM_CCMR2_OC4CE_Pos)                /*!< 0x00008000 */
#define TIM_CCMR2_OC4CE           TIM_CCMR2_OC4CE_Msk                          /*!<Output Compare 4 Clear Enable */

/*----------------------------------------------------------------------------*/

#define TIM_CCMR2_IC3PSC_Pos      (2U)                                         
#define TIM_CCMR2_IC3PSC_Msk      (0x3U << TIM_CCMR2_IC3PSC_Pos)               /*!< 0x0000000C */
#define TIM_CCMR2_IC3PSC          TIM_CCMR2_IC3PSC_Msk                         /*!<IC3PSC[1:0] bits (Input Capture 3 Prescaler) */
#define TIM_CCMR2_IC3PSC_0        (0x1U << TIM_CCMR2_IC3PSC_Pos)               /*!< 0x00000004 */
#define TIM_CCMR2_IC3PSC_1        (0x2U << TIM_CCMR2_IC3PSC_Pos)               /*!< 0x00000008 */

#define TIM_CCMR2_IC3F_Pos        (4U)                                         
#define TIM_CCMR2_IC3F_Msk        (0xFU << TIM_CCMR2_IC3F_Pos)                 /*!< 0x000000F0 */
#define TIM_CCMR2_IC3F            TIM_CCMR2_IC3F_Msk                           /*!<IC3F[3:0] bits (Input Capture 3 Filter) */
#define TIM_CCMR2_IC3F_0          (0x1U << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000010 */
#define TIM_CCMR2_IC3F_1          (0x2U << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000020 */
#define TIM_CCMR2_IC3F_2          (0x4U << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000040 */
#define TIM_CCMR2_IC3F_3          (0x8U << TIM_CCMR2_IC3F_Pos)                 /*!< 0x00000080 */

#define TIM_CCMR2_IC4PSC_Pos      (10U)                                        
#define TIM_CCMR2_IC4PSC_Msk      (0x3U << TIM_CCMR2_IC4PSC_Pos)               /*!< 0x00000C00 */
#define TIM_CCMR2_IC4PSC          TIM_CCMR2_IC4PSC_Msk                         /*!<IC4PSC[1:0] bits (Input Capture 4 Prescaler) */
#define TIM_CCMR2_IC4PSC_0        (0x1U << TIM_CCMR2_IC4PSC_Pos)               /*!< 0x00000400 */
#define TIM_CCMR2_IC4PSC_1        (0x2U << TIM_CCMR2_IC4PSC_Pos)               /*!< 0x00000800 */

#define TIM_CCMR2_IC4F_Pos        (12U)                                        
#define TIM_CCMR2_IC4F_Msk        (0xFU << TIM_CCMR2_IC4F_Pos)                 /*!< 0x0000F000 */
#define TIM_CCMR2_IC4F            TIM_CCMR2_IC4F_Msk                           /*!<IC4F[3:0] bits (Input Capture 4 Filter) */
#define TIM_CCMR2_IC4F_0          (0x1U << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00001000 */
#define TIM_CCMR2_IC4F_1          (0x2U << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00002000 */
#define TIM_CCMR2_IC4F_2          (0x4U << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00004000 */
#define TIM_CCMR2_IC4F_3          (0x8U << TIM_CCMR2_IC4F_Pos)                 /*!< 0x00008000 */

/*******************  Bit definition for TIM_CCER register  *******************/
#define TIM_CCER_CC1E_Pos         (0U)                                         
#define TIM_CCER_CC1E_Msk         (0x1U << TIM_CCER_CC1E_Pos)                  /*!< 0x00000001 */
#define TIM_CCER_CC1E             TIM_CCER_CC1E_Msk                            /*!<Capture/Compare 1 output enable */
#define TIM_CCER_CC1P_Pos         (1U)                                         
#define TIM_CCER_CC1P_Msk         (0x1U << TIM_CCER_CC1P_Pos)                  /*!< 0x00000002 */
#define TIM_CCER_CC1P             TIM_CCER_CC1P_Msk                            /*!<Capture/Compare 1 output Polarity */
#define TIM_CCER_CC1NP_Pos        (3U)                                         
#define TIM_CCER_CC1NP_Msk        (0x1U << TIM_CCER_CC1NP_Pos)                 /*!< 0x00000008 */
#define TIM_CCER_CC1NP            TIM_CCER_CC1NP_Msk                           /*!<Capture/Compare 1 Complementary output Polarity */
#define TIM_CCER_CC2E_Pos         (4U)                                         
#define TIM_CCER_CC2E_Msk         (0x1U << TIM_CCER_CC2E_Pos)                  /*!< 0x00000010 */
#define TIM_CCER_CC2E             TIM_CCER_CC2E_Msk                            /*!<Capture/Compare 2 output enable */
#define TIM_CCER_CC2P_Pos         (5U)                                         
#define TIM_CCER_CC2P_Msk         (0x1U << TIM_CCER_CC2P_Pos)                  /*!< 0x00000020 */
#define TIM_CCER_CC2P             TIM_CCER_CC2P_Msk                            /*!<Capture/Compare 2 output Polarity */
#define TIM_CCER_CC2NP_Pos        (7U)                                         
#define TIM_CCER_CC2NP_Msk        (0x1U << TIM_CCER_CC2NP_Pos)                 /*!< 0x00000080 */
#define TIM_CCER_CC2NP            TIM_CCER_CC2NP_Msk                           /*!<Capture/Compare 2 Complementary output Polarity */
#define TIM_CCER_CC3E_Pos         (8U)                                         
#define TIM_CCER_CC3E_Msk         (0x1U << TIM_CCER_CC3E_Pos)                  /*!< 0x00000100 */
#define TIM_CCER_CC3E             TIM_CCER_CC3E_Msk                            /*!<Capture/Compare 3 output enable */
#define TIM_CCER_CC3P_Pos         (9U)                                         
#define TIM_CCER_CC3P_Msk         (0x1U << TIM_CCER_CC3P_Pos)                  /*!< 0x00000200 */
#define TIM_CCER_CC3P             TIM_CCER_CC3P_Msk                            /*!<Capture/Compare 3 output Polarity */
#define TIM_CCER_CC3NP_Pos        (11U)                                        
#define TIM_CCER_CC3NP_Msk        (0x1U << TIM_CCER_CC3NP_Pos)                 /*!< 0x00000800 */
#define TIM_CCER_CC3NP            TIM_CCER_CC3NP_Msk                           /*!<Capture/Compare 3 Complementary output Polarity */
#define TIM_CCER_CC4E_Pos         (12U)                                        
#define TIM_CCER_CC4E_Msk         (0x1U << TIM_CCER_CC4E_Pos)                  /*!< 0x00001000 */
#define TIM_CCER_CC4E             TIM_CCER_CC4E_Msk                            /*!<Capture/Compare 4 output enable */
#define TIM_CCER_CC4P_Pos         (13U)                                        
#define TIM_CCER_CC4P_Msk         (0x1U << TIM_CCER_CC4P_Pos)                  /*!< 0x00002000 */
#define TIM_CCER_CC4P             TIM_CCER_CC4P_Msk                            /*!<Capture/Compare 4 output Polarity */
#define TIM_CCER_CC4NP_Pos        (15U)                                        
#define TIM_CCER_CC4NP_Msk        (0x1U << TIM_CCER_CC4NP_Pos)                 /*!< 0x00008000 */
#define TIM_CCER_CC4NP            TIM_CCER_CC4NP_Msk                           /*!<Capture/Compare 4 Complementary output Polarity */

/*******************  Bit definition for TIM_CNT register  ********************/
#define TIM_CNT_CNT_Pos           (0U)                                         
#define TIM_CNT_CNT_Msk           (0xFFFFU << TIM_CNT_CNT_Pos)                 /*!< 0x0000FFFF */
#define TIM_CNT_CNT               TIM_CNT_CNT_Msk                              /*!<Counter Value */

/*******************  Bit definition for TIM_PSC register  ********************/
#define TIM_PSC_PSC_Pos           (0U)                                         
#define TIM_PSC_PSC_Msk           (0xFFFFU << TIM_PSC_PSC_Pos)                 /*!< 0x0000FFFF */
#define TIM_PSC_PSC               TIM_PSC_PSC_Msk                              /*!<Prescaler Value */

/*******************  Bit definition for TIM_ARR register  ********************/
#define TIM_ARR_ARR_Pos           (0U)                                         
#define TIM_ARR_ARR_Msk           (0xFFFFU << TIM_ARR_ARR_Pos)                 /*!< 0x0000FFFF */
#define TIM_ARR_ARR               TIM_ARR_ARR_Msk                              /*!<actual auto-reload Value */

/*******************  Bit definition for TIM_CCR1 register  *******************/
#define TIM_CCR1_CCR1_Pos         (0U)                                         
#define TIM_CCR1_CCR1_Msk         (0xFFFFU << TIM_CCR1_CCR1_Pos)               /*!< 0x0000FFFF */
#define TIM_CCR1_CCR1             TIM_CCR1_CCR1_Msk                            /*!<Capture/Compare 1 Value */

/*******************  Bit definition for TIM_CCR2 register  *******************/
#define TIM_CCR2_CCR2_Pos         (0U)                                         
#define TIM_CCR2_CCR2_Msk         (0xFFFFU << TIM_CCR2_CCR2_Pos)               /*!< 0x0000FFFF */
#define TIM_CCR2_CCR2             TIM_CCR2_CCR2_Msk                            /*!<Capture/Compare 2 Value */

/*******************  Bit definition for TIM_CCR3 register  *******************/
#define TIM_CCR3_CCR3_Pos         (0U)                                         
#define TIM_CCR3_CCR3_Msk         (0xFFFFU << TIM_CCR3_CCR3_Pos)               /*!< 0x0000FFFF */
#define TIM_CCR3_CCR3             TIM_CCR3_CCR3_Msk                            /*!<Capture/Compare 3 Value */

/*******************  Bit definition for TIM_CCR4 register  *******************/
#define TIM_CCR4_CCR4_Pos         (0U)                                         
#define TIM_CCR4_CCR4_Msk         (0xFFFFU << TIM_CCR4_CCR4_Pos)               /*!< 0x0000FFFF */
#define TIM_CCR4_CCR4             TIM_CCR4_CCR4_Msk                            /*!<Capture/Compare 4 Value */

/*******************  Bit definition for TIM_DCR register  ********************/
#define TIM_DCR_DBA_Pos           (0U)                                         
#define TIM_DCR_DBA_Msk           (0x1FU << TIM_DCR_DBA_Pos)                   /*!< 0x0000001F */
#define TIM_DCR_DBA               TIM_DCR_DBA_Msk                              /*!<DBA[4:0] bits (DMA Base Address) */
#define TIM_DCR_DBA_0             (0x01U << TIM_DCR_DBA_Pos)                   /*!< 0x00000001 */
#define TIM_DCR_DBA_1             (0x02U << TIM_DCR_DBA_Pos)                   /*!< 0x00000002 */
#define TIM_DCR_DBA_2             (0x04U << TIM_DCR_DBA_Pos)                   /*!< 0x00000004 */
#define TIM_DCR_DBA_3             (0x08U << TIM_DCR_DBA_Pos)                   /*!< 0x00000008 */
#define TIM_DCR_DBA_4             (0x10U << TIM_DCR_DBA_Pos)                   /*!< 0x00000010 */

#define TIM_DCR_DBL_Pos           (8U)                                         
#define TIM_DCR_DBL_Msk           (0x1FU << TIM_DCR_DBL_Pos)                   /*!< 0x00001F00 */
#define TIM_DCR_DBL               TIM_DCR_DBL_Msk                              /*!<DBL[4:0] bits (DMA Burst Length) */
#define TIM_DCR_DBL_0             (0x01U << TIM_DCR_DBL_Pos)                   /*!< 0x00000100 */
#define TIM_DCR_DBL_1             (0x02U << TIM_DCR_DBL_Pos)                   /*!< 0x00000200 */
#define TIM_DCR_DBL_2             (0x04U << TIM_DCR_DBL_Pos)                   /*!< 0x00000400 */
#define TIM_DCR_DBL_3             (0x08U << TIM_DCR_DBL_Pos)                   /*!< 0x00000800 */
#define TIM_DCR_DBL_4             (0x10U << TIM_DCR_DBL_Pos)                   /*!< 0x00001000 */

/*******************  Bit definition for TIM_DMAR register  *******************/
#define TIM_DMAR_DMAB_Pos         (0U)                                         
#define TIM_DMAR_DMAB_Msk         (0xFFFFU << TIM_DMAR_DMAB_Pos)               /*!< 0x0000FFFF */
#define TIM_DMAR_DMAB             TIM_DMAR_DMAB_Msk                            /*!<DMA register for burst accesses */

/*******************  Bit definition for TIM_OR register  *********************/
#define TIM2_OR_ETR_RMP_Pos      (0U)                                          
#define TIM2_OR_ETR_RMP_Msk      (0x7U << TIM2_OR_ETR_RMP_Pos)                 /*!< 0x00000007 */
#define TIM2_OR_ETR_RMP          TIM2_OR_ETR_RMP_Msk                           /*!<ETR_RMP[1:0] bits (TIM2 ETR remap) */
#define TIM2_OR_ETR_RMP_0        (0x1U << TIM2_OR_ETR_RMP_Pos)                 /*!< 0x00000001 */
#define TIM2_OR_ETR_RMP_1        (0x2U << TIM2_OR_ETR_RMP_Pos)                 /*!< 0x00000002 */
#define TIM2_OR_ETR_RMP_2        (0x4U << TIM2_OR_ETR_RMP_Pos)                 /*!< 0x00000004 */
#define TIM2_OR_TI4_RMP_Pos      (3U)                                          
#define TIM2_OR_TI4_RMP_Msk      (0x3U << TIM2_OR_TI4_RMP_Pos)                 /*!< 0x00000018 */
#define TIM2_OR_TI4_RMP          TIM2_OR_TI4_RMP_Msk                           /*!<TI4_RMP[1:0] bits (TIM2 Input 4 remap) */
#define TIM2_OR_TI4_RMP_0        (0x1U << TIM2_OR_TI4_RMP_Pos)                 /*!< 0x00000008 */
#define TIM2_OR_TI4_RMP_1        (0x2U << TIM2_OR_TI4_RMP_Pos)                 /*!< 0x00000010 */

#define TIM21_OR_ETR_RMP_Pos      (0U)                                         
#define TIM21_OR_ETR_RMP_Msk      (0x3U << TIM21_OR_ETR_RMP_Pos)               /*!< 0x00000003 */
#define TIM21_OR_ETR_RMP          TIM21_OR_ETR_RMP_Msk                         /*!<ETR_RMP[1:0] bits (TIM21 ETR remap) */
#define TIM21_OR_ETR_RMP_0        (0x1U << TIM21_OR_ETR_RMP_Pos)               /*!< 0x00000001 */
#define TIM21_OR_ETR_RMP_1        (0x2U << TIM21_OR_ETR_RMP_Pos)               /*!< 0x00000002 */
#define TIM21_OR_TI1_RMP_Pos      (2U)                                         
#define TIM21_OR_TI1_RMP_Msk      (0x7U << TIM21_OR_TI1_RMP_Pos)               /*!< 0x0000001C */
#define TIM21_OR_TI1_RMP          TIM21_OR_TI1_RMP_Msk                         /*!<TI1_RMP[2:0] bits (TIM21 Input 1 remap) */
#define TIM21_OR_TI1_RMP_0        (0x1U << TIM21_OR_TI1_RMP_Pos)               /*!< 0x00000004 */
#define TIM21_OR_TI1_RMP_1        (0x2U << TIM21_OR_TI1_RMP_Pos)               /*!< 0x00000008 */
#define TIM21_OR_TI1_RMP_2        (0x4U << TIM21_OR_TI1_RMP_Pos)               /*!< 0x00000010 */
#define TIM21_OR_TI2_RMP_Pos      (5U)                                         
#define TIM21_OR_TI2_RMP_Msk      (0x1U << TIM21_OR_TI2_RMP_Pos)               /*!< 0x00000020 */
#define TIM21_OR_TI2_RMP          TIM21_OR_TI2_RMP_Msk                         /*!<TI2_RMP bit (TIM21 Input 2 remap) */

#define TIM22_OR_ETR_RMP_Pos      (0U)                                         
#define TIM22_OR_ETR_RMP_Msk      (0x3U << TIM22_OR_ETR_RMP_Pos)               /*!< 0x00000003 */
#define TIM22_OR_ETR_RMP          TIM22_OR_ETR_RMP_Msk                         /*!<ETR_RMP[1:0] bits (TIM22 ETR remap) */
#define TIM22_OR_ETR_RMP_0        (0x1U << TIM22_OR_ETR_RMP_Pos)               /*!< 0x00000001 */
#define TIM22_OR_ETR_RMP_1        (0x2U << TIM22_OR_ETR_RMP_Pos)               /*!< 0x00000002 */
#define TIM22_OR_TI1_RMP_Pos      (2U)                                         
#define TIM22_OR_TI1_RMP_Msk      (0x3U << TIM22_OR_TI1_RMP_Pos)               /*!< 0x0000000C */
#define TIM22_OR_TI1_RMP          TIM22_OR_TI1_RMP_Msk                         /*!<TI1_RMP[2:0] bits (TIM22 Input 1 remap) */
#define TIM22_OR_TI1_RMP_0        (0x1U << TIM22_OR_TI1_RMP_Pos)               /*!< 0x00000004 */
#define TIM22_OR_TI1_RMP_1        (0x2U << TIM22_OR_TI1_RMP_Pos)               /*!< 0x00000008 */


/******************************************************************************/
/*                                                                            */
/*      Universal Synchronous Asynchronous Receiver Transmitter (USART)       */
/*                                                                            */
/******************************************************************************/

/*
 * @brief Specific device feature definitions (not present on all devices in the STM32L0 family)
 */
/* Note: No specific macro feature on this device */

/******************  Bit definition for USART_CR1 register  *******************/
#define USART_CR1_UE_Pos              (0U)                                     
#define USART_CR1_UE_Msk              (0x1U << USART_CR1_UE_Pos)               /*!< 0x00000001 */
#define USART_CR1_UE                  USART_CR1_UE_Msk                         /*!< USART Enable */
#define USART_CR1_UESM_Pos            (1U)                                     
#define USART_CR1_UESM_Msk            (0x1U << USART_CR1_UESM_Pos)             /*!< 0x00000002 */
#define USART_CR1_UESM                USART_CR1_UESM_Msk                       /*!< USART Enable in STOP Mode */
#define USART_CR1_RE_Pos              (2U)                                     
#define USART_CR1_RE_Msk              (0x1U << USART_CR1_RE_Pos)               /*!< 0x00000004 */
#define USART_CR1_RE                  USART_CR1_RE_Msk                         /*!< Receiver Enable */
#define USART_CR1_TE_Pos              (3U)                                     
#define USART_CR1_TE_Msk              (0x1U << USART_CR1_TE_Pos)               /*!< 0x00000008 */
#define USART_CR1_TE                  USART_CR1_TE_Msk                         /*!< Transmitter Enable */
#define USART_CR1_IDLEIE_Pos          (4U)                                     
#define USART_CR1_IDLEIE_Msk          (0x1U << USART_CR1_IDLEIE_Pos)           /*!< 0x00000010 */
#define USART_CR1_IDLEIE              USART_CR1_IDLEIE_Msk                     /*!< IDLE Interrupt Enable */
#define USART_CR1_RXNEIE_Pos          (5U)                                     
#define USART_CR1_RXNEIE_Msk          (0x1U << USART_CR1_RXNEIE_Pos)           /*!< 0x00000020 */
#define USART_CR1_RXNEIE              USART_CR1_RXNEIE_Msk                     /*!< RXNE Interrupt Enable */
#define USART_CR1_TCIE_Pos            (6U)                                     
#define USART_CR1_TCIE_Msk            (0x1U << USART_CR1_TCIE_Pos)             /*!< 0x00000040 */
#define USART_CR1_TCIE                USART_CR1_TCIE_Msk                       /*!< Transmission Complete Interrupt Enable */
#define USART_CR1_TXEIE_Pos           (7U)                                     
#define USART_CR1_TXEIE_Msk           (0x1U << USART_CR1_TXEIE_Pos)            /*!< 0x00000080 */
#define USART_CR1_TXEIE               USART_CR1_TXEIE_Msk                      /*!< TXE Interrupt Enable */
#define USART_CR1_PEIE_Pos            (8U)                                     
#define USART_CR1_PEIE_Msk            (0x1U << USART_CR1_PEIE_Pos)             /*!< 0x00000100 */
#define USART_CR1_PEIE                USART_CR1_PEIE_Msk                       /*!< PE Interrupt Enable */
#define USART_CR1_PS_Pos              (9U)                                     
#define USART_CR1_PS_Msk              (0x1U << USART_CR1_PS_Pos)               /*!< 0x00000200 */
#define USART_CR1_PS                  USART_CR1_PS_Msk                         /*!< Parity Selection */
#define USART_CR1_PCE_Pos             (10U)                                    
#define USART_CR1_PCE_Msk             (0x1U << USART_CR1_PCE_Pos)              /*!< 0x00000400 */
#define USART_CR1_PCE                 USART_CR1_PCE_Msk                        /*!< Parity Control Enable */
#define USART_CR1_WAKE_Pos            (11U)                                    
#define USART_CR1_WAKE_Msk            (0x1U << USART_CR1_WAKE_Pos)             /*!< 0x00000800 */
#define USART_CR1_WAKE                USART_CR1_WAKE_Msk                       /*!< Receiver Wakeup method */
#define USART_CR1_M_Pos               (12U)                                    
#define USART_CR1_M_Msk               (0x10001U << USART_CR1_M_Pos)            /*!< 0x10001000 */
#define USART_CR1_M                   USART_CR1_M_Msk                          /*!< Word length */
#define USART_CR1_M0_Pos              (12U)                                    
#define USART_CR1_M0_Msk              (0x1U << USART_CR1_M0_Pos)               /*!< 0x00001000 */
#define USART_CR1_M0                  USART_CR1_M0_Msk                         /*!< Word length - Bit 0 */
#define USART_CR1_MME_Pos             (13U)                                    
#define USART_CR1_MME_Msk             (0x1U << USART_CR1_MME_Pos)              /*!< 0x00002000 */
#define USART_CR1_MME                 USART_CR1_MME_Msk                        /*!< Mute Mode Enable */
#define USART_CR1_CMIE_Pos            (14U)                                    
#define USART_CR1_CMIE_Msk            (0x1U << USART_CR1_CMIE_Pos)             /*!< 0x00004000 */
#define USART_CR1_CMIE                USART_CR1_CMIE_Msk                       /*!< Character match interrupt enable */
#define USART_CR1_OVER8_Pos           (15U)                                    
#define USART_CR1_OVER8_Msk           (0x1U << USART_CR1_OVER8_Pos)            /*!< 0x00008000 */
#define USART_CR1_OVER8               USART_CR1_OVER8_Msk                      /*!< Oversampling by 8-bit or 16-bit mode */
#define USART_CR1_DEDT_Pos            (16U)                                    
#define USART_CR1_DEDT_Msk            (0x1FU << USART_CR1_DEDT_Pos)            /*!< 0x001F0000 */
#define USART_CR1_DEDT                USART_CR1_DEDT_Msk                       /*!< DEDT[4:0] bits (Driver Enable Deassertion Time) */
#define USART_CR1_DEDT_0              (0x01U << USART_CR1_DEDT_Pos)            /*!< 0x00010000 */
#define USART_CR1_DEDT_1              (0x02U << USART_CR1_DEDT_Pos)            /*!< 0x00020000 */
#define USART_CR1_DEDT_2              (0x04U << USART_CR1_DEDT_Pos)            /*!< 0x00040000 */
#define USART_CR1_DEDT_3              (0x08U << USART_CR1_DEDT_Pos)            /*!< 0x00080000 */
#define USART_CR1_DEDT_4              (0x10U << USART_CR1_DEDT_Pos)            /*!< 0x00100000 */
#define USART_CR1_DEAT_Pos            (21U)                                    
#define USART_CR1_DEAT_Msk            (0x1FU << USART_CR1_DEAT_Pos)            /*!< 0x03E00000 */
#define USART_CR1_DEAT                USART_CR1_DEAT_Msk                       /*!< DEAT[4:0] bits (Driver Enable Assertion Time) */
#define USART_CR1_DEAT_0              (0x01U << USART_CR1_DEAT_Pos)            /*!< 0x00200000 */
#define USART_CR1_DEAT_1              (0x02U << USART_CR1_DEAT_Pos)            /*!< 0x00400000 */
#define USART_CR1_DEAT_2              (0x04U << USART_CR1_DEAT_Pos)            /*!< 0x00800000 */
#define USART_CR1_DEAT_3              (0x08U << USART_CR1_DEAT_Pos)            /*!< 0x01000000 */
#define USART_CR1_DEAT_4              (0x10U << USART_CR1_DEAT_Pos)            /*!< 0x02000000 */
#define USART_CR1_RTOIE_Pos           (26U)                                    
#define USART_CR1_RTOIE_Msk           (0x1U << USART_CR1_RTOIE_Pos)            /*!< 0x04000000 */
#define USART_CR1_RTOIE               USART_CR1_RTOIE_Msk                      /*!< Receive Time Out interrupt enable */
#define USART_CR1_EOBIE_Pos           (27U)                                    
#define USART_CR1_EOBIE_Msk           (0x1U << USART_CR1_EOBIE_Pos)            /*!< 0x08000000 */
#define USART_CR1_EOBIE               USART_CR1_EOBIE_Msk                      /*!< End of Block interrupt enable */
#define USART_CR1_M1_Pos              (28U)                                    
#define USART_CR1_M1_Msk              (0x1U << USART_CR1_M1_Pos)               /*!< 0x10000000 */
#define USART_CR1_M1                  USART_CR1_M1_Msk                         /*!< Word length - Bit 1 */
/******************  Bit definition for USART_CR2 register  *******************/
#define USART_CR2_ADDM7_Pos           (4U)                                     
#define USART_CR2_ADDM7_Msk           (0x1U << USART_CR2_ADDM7_Pos)            /*!< 0x00000010 */
#define USART_CR2_ADDM7               USART_CR2_ADDM7_Msk                      /*!< 7-bit or 4-bit Address Detection */
#define USART_CR2_LBDL_Pos            (5U)                                     
#define USART_CR2_LBDL_Msk            (0x1U << USART_CR2_LBDL_Pos)             /*!< 0x00000020 */
#define USART_CR2_LBDL                USART_CR2_LBDL_Msk                       /*!< LIN Break Detection Length */
#define USART_CR2_LBDIE_Pos           (6U)                                     
#define USART_CR2_LBDIE_Msk           (0x1U << USART_CR2_LBDIE_Pos)            /*!< 0x00000040 */
#define USART_CR2_LBDIE               USART_CR2_LBDIE_Msk                      /*!< LIN Break Detection Interrupt Enable */
#define USART_CR2_LBCL_Pos            (8U)                                     
#define USART_CR2_LBCL_Msk            (0x1U << USART_CR2_LBCL_Pos)             /*!< 0x00000100 */
#define USART_CR2_LBCL                USART_CR2_LBCL_Msk                       /*!< Last Bit Clock pulse */
#define USART_CR2_CPHA_Pos            (9U)                                     
#define USART_CR2_CPHA_Msk            (0x1U << USART_CR2_CPHA_Pos)             /*!< 0x00000200 */
#define USART_CR2_CPHA                USART_CR2_CPHA_Msk                       /*!< Clock Phase */
#define USART_CR2_CPOL_Pos            (10U)                                    
#define USART_CR2_CPOL_Msk            (0x1U << USART_CR2_CPOL_Pos)             /*!< 0x00000400 */
#define USART_CR2_CPOL                USART_CR2_CPOL_Msk                       /*!< Clock Polarity */
#define USART_CR2_CLKEN_Pos           (11U)                                    
#define USART_CR2_CLKEN_Msk           (0x1U << USART_CR2_CLKEN_Pos)            /*!< 0x00000800 */
#define USART_CR2_CLKEN               USART_CR2_CLKEN_Msk                      /*!< Clock Enable */
#define USART_CR2_STOP_Pos            (12U)                                    
#define USART_CR2_STOP_Msk            (0x3U << USART_CR2_STOP_Pos)             /*!< 0x00003000 */
#define USART_CR2_STOP                USART_CR2_STOP_Msk                       /*!< STOP[1:0] bits (STOP bits) */
#define USART_CR2_STOP_0              (0x1U << USART_CR2_STOP_Pos)             /*!< 0x00001000 */
#define USART_CR2_STOP_1              (0x2U << USART_CR2_STOP_Pos)             /*!< 0x00002000 */
#define USART_CR2_LINEN_Pos           (14U)                                    
#define USART_CR2_LINEN_Msk           (0x1U << USART_CR2_LINEN_Pos)            /*!< 0x00004000 */
#define USART_CR2_LINEN               USART_CR2_LINEN_Msk                      /*!< LIN mode enable */
#define USART_CR2_SWAP_Pos            (15U)                                    
#define USART_CR2_SWAP_Msk            (0x1U << USART_CR2_SWAP_Pos)             /*!< 0x00008000 */
#define USART_CR2_SWAP                USART_CR2_SWAP_Msk                       /*!< SWAP TX/RX pins */
#define USART_CR2_RXINV_Pos           (16U)                                    
#define USART_CR2_RXINV_Msk           (0x1U << USART_CR2_RXINV_Pos)            /*!< 0x00010000 */
#define USART_CR2_RXINV               USART_CR2_RXINV_Msk                      /*!< RX pin active level inversion */
#define USART_CR2_TXINV_Pos           (17U)                                    
#define USART_CR2_TXINV_Msk           (0x1U << USART_CR2_TXINV_Pos)            /*!< 0x00020000 */
#define USART_CR2_TXINV               USART_CR2_TXINV_Msk                      /*!< TX pin active level inversion */
#define USART_CR2_DATAINV_Pos         (18U)                                    
#define USART_CR2_DATAINV_Msk         (0x1U << USART_CR2_DATAINV_Pos)          /*!< 0x00040000 */
#define USART_CR2_DATAINV             USART_CR2_DATAINV_Msk                    /*!< Binary data inversion */
#define USART_CR2_MSBFIRST_Pos        (19U)                                    
#define USART_CR2_MSBFIRST_Msk        (0x1U << USART_CR2_MSBFIRST_Pos)         /*!< 0x00080000 */
#define USART_CR2_MSBFIRST            USART_CR2_MSBFIRST_Msk                   /*!< Most Significant Bit First */
#define USART_CR2_ABREN_Pos           (20U)                                    
#define USART_CR2_ABREN_Msk           (0x1U << USART_CR2_ABREN_Pos)            /*!< 0x00100000 */
#define USART_CR2_ABREN               USART_CR2_ABREN_Msk                      /*!< Auto Baud-Rate Enable*/
#define USART_CR2_ABRMODE_Pos         (21U)                                    
#define USART_CR2_ABRMODE_Msk         (0x3U << USART_CR2_ABRMODE_Pos)          /*!< 0x00600000 */
#define USART_CR2_ABRMODE             USART_CR2_ABRMODE_Msk                    /*!< ABRMOD[1:0] bits (Auto Baud-Rate Mode) */
#define USART_CR2_ABRMODE_0           (0x1U << USART_CR2_ABRMODE_Pos)          /*!< 0x00200000 */
#define USART_CR2_ABRMODE_1           (0x2U << USART_CR2_ABRMODE_Pos)          /*!< 0x00400000 */
#define USART_CR2_RTOEN_Pos           (23U)                                    
#define USART_CR2_RTOEN_Msk           (0x1U << USART_CR2_RTOEN_Pos)            /*!< 0x00800000 */
#define USART_CR2_RTOEN               USART_CR2_RTOEN_Msk                      /*!< Receiver Time-Out enable */
#define USART_CR2_ADD_Pos             (24U)                                    
#define USART_CR2_ADD_Msk             (0xFFU << USART_CR2_ADD_Pos)             /*!< 0xFF000000 */
#define USART_CR2_ADD                 USART_CR2_ADD_Msk                        /*!< Address of the USART node */

/******************  Bit definition for USART_CR3 register  *******************/
#define USART_CR3_EIE_Pos             (0U)                                     
#define USART_CR3_EIE_Msk             (0x1U << USART_CR3_EIE_Pos)              /*!< 0x00000001 */
#define USART_CR3_EIE                 USART_CR3_EIE_Msk                        /*!< Error Interrupt Enable */
#define USART_CR3_IREN_Pos            (1U)                                     
#define USART_CR3_IREN_Msk            (0x1U << USART_CR3_IREN_Pos)             /*!< 0x00000002 */
#define USART_CR3_IREN                USART_CR3_IREN_Msk                       /*!< IrDA mode Enable */
#define USART_CR3_IRLP_Pos            (2U)                                     
#define USART_CR3_IRLP_Msk            (0x1U << USART_CR3_IRLP_Pos)             /*!< 0x00000004 */
#define USART_CR3_IRLP                USART_CR3_IRLP_Msk                       /*!< IrDA Low-Power */
#define USART_CR3_HDSEL_Pos           (3U)                                     
#define USART_CR3_HDSEL_Msk           (0x1U << USART_CR3_HDSEL_Pos)            /*!< 0x00000008 */
#define USART_CR3_HDSEL               USART_CR3_HDSEL_Msk                      /*!< Half-Duplex Selection */
#define USART_CR3_NACK_Pos            (4U)                                     
#define USART_CR3_NACK_Msk            (0x1U << USART_CR3_NACK_Pos)             /*!< 0x00000010 */
#define USART_CR3_NACK                USART_CR3_NACK_Msk                       /*!< SmartCard NACK enable */
#define USART_CR3_SCEN_Pos            (5U)                                     
#define USART_CR3_SCEN_Msk            (0x1U << USART_CR3_SCEN_Pos)             /*!< 0x00000020 */
#define USART_CR3_SCEN                USART_CR3_SCEN_Msk                       /*!< SmartCard mode enable */
#define USART_CR3_DMAR_Pos            (6U)                                     
#define USART_CR3_DMAR_Msk            (0x1U << USART_CR3_DMAR_Pos)             /*!< 0x00000040 */
#define USART_CR3_DMAR                USART_CR3_DMAR_Msk                       /*!< DMA Enable Receiver */
#define USART_CR3_DMAT_Pos            (7U)                                     
#define USART_CR3_DMAT_Msk            (0x1U << USART_CR3_DMAT_Pos)             /*!< 0x00000080 */
#define USART_CR3_DMAT                USART_CR3_DMAT_Msk                       /*!< DMA Enable Transmitter */
#define USART_CR3_RTSE_Pos            (8U)                                     
#define USART_CR3_RTSE_Msk            (0x1U << USART_CR3_RTSE_Pos)             /*!< 0x00000100 */
#define USART_CR3_RTSE                USART_CR3_RTSE_Msk                       /*!< RTS Enable */
#define USART_CR3_CTSE_Pos            (9U)                                     
#define USART_CR3_CTSE_Msk            (0x1U << USART_CR3_CTSE_Pos)             /*!< 0x00000200 */
#define USART_CR3_CTSE                USART_CR3_CTSE_Msk                       /*!< CTS Enable */
#define USART_CR3_CTSIE_Pos           (10U)                                    
#define USART_CR3_CTSIE_Msk           (0x1U << USART_CR3_CTSIE_Pos)            /*!< 0x00000400 */
#define USART_CR3_CTSIE               USART_CR3_CTSIE_Msk                      /*!< CTS Interrupt Enable */
#define USART_CR3_ONEBIT_Pos          (11U)                                    
#define USART_CR3_ONEBIT_Msk          (0x1U << USART_CR3_ONEBIT_Pos)           /*!< 0x00000800 */
#define USART_CR3_ONEBIT              USART_CR3_ONEBIT_Msk                     /*!< One sample bit method enable */
#define USART_CR3_OVRDIS_Pos          (12U)                                    
#define USART_CR3_OVRDIS_Msk          (0x1U << USART_CR3_OVRDIS_Pos)           /*!< 0x00001000 */
#define USART_CR3_OVRDIS              USART_CR3_OVRDIS_Msk                     /*!< Overrun Disable */
#define USART_CR3_DDRE_Pos            (13U)                                    
#define USART_CR3_DDRE_Msk            (0x1U << USART_CR3_DDRE_Pos)             /*!< 0x00002000 */
#define USART_CR3_DDRE                USART_CR3_DDRE_Msk                       /*!< DMA Disable on Reception Error */
#define USART_CR3_DEM_Pos             (14U)                                    
#define USART_CR3_DEM_Msk             (0x1U << USART_CR3_DEM_Pos)              /*!< 0x00004000 */
#define USART_CR3_DEM                 USART_CR3_DEM_Msk                        /*!< Driver Enable Mode */
#define USART_CR3_DEP_Pos             (15U)                                    
#define USART_CR3_DEP_Msk             (0x1U << USART_CR3_DEP_Pos)              /*!< 0x00008000 */
#define USART_CR3_DEP                 USART_CR3_DEP_Msk                        /*!< Driver Enable Polarity Selection */
#define USART_CR3_SCARCNT_Pos         (17U)                                    
#define USART_CR3_SCARCNT_Msk         (0x7U << USART_CR3_SCARCNT_Pos)          /*!< 0x000E0000 */
#define USART_CR3_SCARCNT             USART_CR3_SCARCNT_Msk                    /*!< SCARCNT[2:0] bits (SmartCard Auto-Retry Count) */
#define USART_CR3_SCARCNT_0           (0x1U << USART_CR3_SCARCNT_Pos)          /*!< 0x00020000 */
#define USART_CR3_SCARCNT_1           (0x2U << USART_CR3_SCARCNT_Pos)          /*!< 0x00040000 */
#define USART_CR3_SCARCNT_2           (0x4U << USART_CR3_SCARCNT_Pos)          /*!< 0x00080000 */
#define USART_CR3_WUS_Pos             (20U)                                    
#define USART_CR3_WUS_Msk             (0x3U << USART_CR3_WUS_Pos)              /*!< 0x00300000 */
#define USART_CR3_WUS                 USART_CR3_WUS_Msk                        /*!< WUS[1:0] bits (Wake UP Interrupt Flag Selection) */
#define USART_CR3_WUS_0               (0x1U << USART_CR3_WUS_Pos)              /*!< 0x00100000 */
#define USART_CR3_WUS_1               (0x2U << USART_CR3_WUS_Pos)              /*!< 0x00200000 */
#define USART_CR3_WUFIE_Pos           (22U)                                    
#define USART_CR3_WUFIE_Msk           (0x1U << USART_CR3_WUFIE_Pos)            /*!< 0x00400000 */
#define USART_CR3_WUFIE               USART_CR3_WUFIE_Msk                      /*!< Wake Up Interrupt Enable */
#define USART_CR3_UCESM_Pos           (23U)                                    
#define USART_CR3_UCESM_Msk           (0x1U << USART_CR3_UCESM_Pos)            /*!< 0x00800000 */
#define USART_CR3_UCESM               USART_CR3_UCESM_Msk                      /*!< Clock Enable in Stop mode */ 

/******************  Bit definition for USART_BRR register  *******************/
#define USART_BRR_DIV_FRACTION_Pos    (0U)                                     
#define USART_BRR_DIV_FRACTION_Msk    (0xFU << USART_BRR_DIV_FRACTION_Pos)     /*!< 0x0000000F */
#define USART_BRR_DIV_FRACTION        USART_BRR_DIV_FRACTION_Msk               /*!< Fraction of USARTDIV */
#define USART_BRR_DIV_MANTISSA_Pos    (4U)                                     
#define USART_BRR_DIV_MANTISSA_Msk    (0xFFFU << USART_BRR_DIV_MANTISSA_Pos)   /*!< 0x0000FFF0 */
#define USART_BRR_DIV_MANTISSA        USART_BRR_DIV_MANTISSA_Msk               /*!< Mantissa of USARTDIV */

/******************  Bit definition for USART_GTPR register  ******************/
#define USART_GTPR_PSC_Pos            (0U)                                     
#define USART_GTPR_PSC_Msk            (0xFFU << USART_GTPR_PSC_Pos)            /*!< 0x000000FF */
#define USART_GTPR_PSC                USART_GTPR_PSC_Msk                       /*!< PSC[7:0] bits (Prescaler value) */
#define USART_GTPR_GT_Pos             (8U)                                     
#define USART_GTPR_GT_Msk             (0xFFU << USART_GTPR_GT_Pos)             /*!< 0x0000FF00 */
#define USART_GTPR_GT                 USART_GTPR_GT_Msk                        /*!< GT[7:0] bits (Guard time value) */


/*******************  Bit definition for USART_RTOR register  *****************/
#define USART_RTOR_RTO_Pos            (0U)                                     
#define USART_RTOR_RTO_Msk            (0xFFFFFFU << USART_RTOR_RTO_Pos)        /*!< 0x00FFFFFF */
#define USART_RTOR_RTO                USART_RTOR_RTO_Msk                       /*!< Receiver Time Out Value */
#define USART_RTOR_BLEN_Pos           (24U)                                    
#define USART_RTOR_BLEN_Msk           (0xFFU << USART_RTOR_BLEN_Pos)           /*!< 0xFF000000 */
#define USART_RTOR_BLEN               USART_RTOR_BLEN_Msk                      /*!< Block Length */

/*******************  Bit definition for USART_RQR register  ******************/
#define USART_RQR_ABRRQ_Pos           (0U)                                     
#define USART_RQR_ABRRQ_Msk           (0x1U << USART_RQR_ABRRQ_Pos)            /*!< 0x00000001 */
#define USART_RQR_ABRRQ               USART_RQR_ABRRQ_Msk                      /*!< Auto-Baud Rate Request */
#define USART_RQR_SBKRQ_Pos           (1U)                                     
#define USART_RQR_SBKRQ_Msk           (0x1U << USART_RQR_SBKRQ_Pos)            /*!< 0x00000002 */
#define USART_RQR_SBKRQ               USART_RQR_SBKRQ_Msk                      /*!< Send Break Request */
#define USART_RQR_MMRQ_Pos            (2U)                                     
#define USART_RQR_MMRQ_Msk            (0x1U << USART_RQR_MMRQ_Pos)             /*!< 0x00000004 */
#define USART_RQR_MMRQ                USART_RQR_MMRQ_Msk                       /*!< Mute Mode Request */
#define USART_RQR_RXFRQ_Pos           (3U)                                     
#define USART_RQR_RXFRQ_Msk           (0x1U << USART_RQR_RXFRQ_Pos)            /*!< 0x00000008 */
#define USART_RQR_RXFRQ               USART_RQR_RXFRQ_Msk                      /*!< Receive Data flush Request */
#define USART_RQR_TXFRQ_Pos           (4U)                                     
#define USART_RQR_TXFRQ_Msk           (0x1U << USART_RQR_TXFRQ_Pos)            /*!< 0x00000010 */
#define USART_RQR_TXFRQ               USART_RQR_TXFRQ_Msk                      /*!< Transmit data flush Request */

/*******************  Bit definition for USART_ISR register  ******************/
#define USART_ISR_PE_Pos              (0U)                                     
#define USART_ISR_PE_Msk              (0x1U << USART_ISR_PE_Pos)               /*!< 0x00000001 */
#define USART_ISR_PE                  USART_ISR_PE_Msk                         /*!< Parity Error */
#define USART_ISR_FE_Pos              (1U)                                     
#define USART_ISR_FE_Msk              (0x1U << USART_ISR_FE_Pos)               /*!< 0x00000002 */
#define USART_ISR_FE                  USART_ISR_FE_Msk                         /*!< Framing Error */
#define USART_ISR_NE_Pos              (2U)                                     
#define USART_ISR_NE_Msk              (0x1U << USART_ISR_NE_Pos)               /*!< 0x00000004 */
#define USART_ISR_NE                  USART_ISR_NE_Msk                         /*!< Noise detected Flag */
#define USART_ISR_ORE_Pos             (3U)                                     
#define USART_ISR_ORE_Msk             (0x1U << USART_ISR_ORE_Pos)              /*!< 0x00000008 */
#define USART_ISR_ORE                 USART_ISR_ORE_Msk                        /*!< OverRun Error */
#define USART_ISR_IDLE_Pos            (4U)                                     
#define USART_ISR_IDLE_Msk            (0x1U << USART_ISR_IDLE_Pos)             /*!< 0x00000010 */
#define USART_ISR_IDLE                USART_ISR_IDLE_Msk                       /*!< IDLE line detected */
#define USART_ISR_RXNE_Pos            (5U)                                     
#define USART_ISR_RXNE_Msk            (0x1U << USART_ISR_RXNE_Pos)             /*!< 0x00000020 */
#define USART_ISR_RXNE                USART_ISR_RXNE_Msk                       /*!< Read Data Register Not Empty */
#define USART_ISR_TC_Pos              (6U)                                     
#define USART_ISR_TC_Msk              (0x1U << USART_ISR_TC_Pos)               /*!< 0x00000040 */
#define USART_ISR_TC                  USART_ISR_TC_Msk                         /*!< Transmission Complete */
#define USART_ISR_TXE_Pos             (7U)                                     
#define USART_ISR_TXE_Msk             (0x1U << USART_ISR_TXE_Pos)              /*!< 0x00000080 */
#define USART_ISR_TXE                 USART_ISR_TXE_Msk                        /*!< Transmit Data Register Empty */
#define USART_ISR_LBDF_Pos            (8U)                                     
#define USART_ISR_LBDF_Msk            (0x1U << USART_ISR_LBDF_Pos)             /*!< 0x00000100 */
#define USART_ISR_LBDF                USART_ISR_LBDF_Msk                       /*!< LIN Break Detection Flag */
#define USART_ISR_CTSIF_Pos           (9U)                                     
#define USART_ISR_CTSIF_Msk           (0x1U << USART_ISR_CTSIF_Pos)            /*!< 0x00000200 */
#define USART_ISR_CTSIF               USART_ISR_CTSIF_Msk                      /*!< CTS interrupt flag */
#define USART_ISR_CTS_Pos             (10U)                                    
#define USART_ISR_CTS_Msk             (0x1U << USART_ISR_CTS_Pos)              /*!< 0x00000400 */
#define USART_ISR_CTS                 USART_ISR_CTS_Msk                        /*!< CTS flag */
#define USART_ISR_RTOF_Pos            (11U)                                    
#define USART_ISR_RTOF_Msk            (0x1U << USART_ISR_RTOF_Pos)             /*!< 0x00000800 */
#define USART_ISR_RTOF                USART_ISR_RTOF_Msk                       /*!< Receiver Time Out */
#define USART_ISR_EOBF_Pos            (12U)                                    
#define USART_ISR_EOBF_Msk            (0x1U << USART_ISR_EOBF_Pos)             /*!< 0x00001000 */
#define USART_ISR_EOBF                USART_ISR_EOBF_Msk                       /*!< End Of Block Flag */
#define USART_ISR_ABRE_Pos            (14U)                                    
#define USART_ISR_ABRE_Msk            (0x1U << USART_ISR_ABRE_Pos)             /*!< 0x00004000 */
#define USART_ISR_ABRE                USART_ISR_ABRE_Msk                       /*!< Auto-Baud Rate Error */
#define USART_ISR_ABRF_Pos            (15U)                                    
#define USART_ISR_ABRF_Msk            (0x1U << USART_ISR_ABRF_Pos)             /*!< 0x00008000 */
#define USART_ISR_ABRF                USART_ISR_ABRF_Msk                       /*!< Auto-Baud Rate Flag */
#define USART_ISR_BUSY_Pos            (16U)                                    
#define USART_ISR_BUSY_Msk            (0x1U << USART_ISR_BUSY_Pos)             /*!< 0x00010000 */
#define USART_ISR_BUSY                USART_ISR_BUSY_Msk                       /*!< Busy Flag */
#define USART_ISR_CMF_Pos             (17U)                                    
#define USART_ISR_CMF_Msk             (0x1U << USART_ISR_CMF_Pos)              /*!< 0x00020000 */
#define USART_ISR_CMF                 USART_ISR_CMF_Msk                        /*!< Character Match Flag */
#define USART_ISR_SBKF_Pos            (18U)                                    
#define USART_ISR_SBKF_Msk            (0x1U << USART_ISR_SBKF_Pos)             /*!< 0x00040000 */
#define USART_ISR_SBKF                USART_ISR_SBKF_Msk                       /*!< Send Break Flag */
#define USART_ISR_RWU_Pos             (19U)                                    
#define USART_ISR_RWU_Msk             (0x1U << USART_ISR_RWU_Pos)              /*!< 0x00080000 */
#define USART_ISR_RWU                 USART_ISR_RWU_Msk                        /*!< Receive Wake Up from mute mode Flag */
#define USART_ISR_WUF_Pos             (20U)                                    
#define USART_ISR_WUF_Msk             (0x1U << USART_ISR_WUF_Pos)              /*!< 0x00100000 */
#define USART_ISR_WUF                 USART_ISR_WUF_Msk                        /*!< Wake Up from stop mode Flag */
#define USART_ISR_TEACK_Pos           (21U)                                    
#define USART_ISR_TEACK_Msk           (0x1U << USART_ISR_TEACK_Pos)            /*!< 0x00200000 */
#define USART_ISR_TEACK               USART_ISR_TEACK_Msk                      /*!< Transmit Enable Acknowledge Flag */
#define USART_ISR_REACK_Pos           (22U)                                    
#define USART_ISR_REACK_Msk           (0x1U << USART_ISR_REACK_Pos)            /*!< 0x00400000 */
#define USART_ISR_REACK               USART_ISR_REACK_Msk                      /*!< Receive Enable Acknowledge Flag */

/*******************  Bit definition for USART_ICR register  ******************/
#define USART_ICR_PECF_Pos            (0U)                                     
#define USART_ICR_PECF_Msk            (0x1U << USART_ICR_PECF_Pos)             /*!< 0x00000001 */
#define USART_ICR_PECF                USART_ICR_PECF_Msk                       /*!< Parity Error Clear Flag */
#define USART_ICR_FECF_Pos            (1U)                                     
#define USART_ICR_FECF_Msk            (0x1U << USART_ICR_FECF_Pos)             /*!< 0x00000002 */
#define USART_ICR_FECF                USART_ICR_FECF_Msk                       /*!< Framing Error Clear Flag */
#define USART_ICR_NCF_Pos             (2U)                                     
#define USART_ICR_NCF_Msk             (0x1U << USART_ICR_NCF_Pos)              /*!< 0x00000004 */
#define USART_ICR_NCF                 USART_ICR_NCF_Msk                        /*!< Noise detected Clear Flag */
#define USART_ICR_ORECF_Pos           (3U)                                     
#define USART_ICR_ORECF_Msk           (0x1U << USART_ICR_ORECF_Pos)            /*!< 0x00000008 */
#define USART_ICR_ORECF               USART_ICR_ORECF_Msk                      /*!< OverRun Error Clear Flag */
#define USART_ICR_IDLECF_Pos          (4U)                                     
#define USART_ICR_IDLECF_Msk          (0x1U << USART_ICR_IDLECF_Pos)           /*!< 0x00000010 */
#define USART_ICR_IDLECF              USART_ICR_IDLECF_Msk                     /*!< IDLE line detected Clear Flag */
#define USART_ICR_TCCF_Pos            (6U)                                     
#define USART_ICR_TCCF_Msk            (0x1U << USART_ICR_TCCF_Pos)             /*!< 0x00000040 */
#define USART_ICR_TCCF                USART_ICR_TCCF_Msk                       /*!< Transmission Complete Clear Flag */
#define USART_ICR_LBDCF_Pos           (8U)                                     
#define USART_ICR_LBDCF_Msk           (0x1U << USART_ICR_LBDCF_Pos)            /*!< 0x00000100 */
#define USART_ICR_LBDCF               USART_ICR_LBDCF_Msk                      /*!< LIN Break Detection Clear Flag */
#define USART_ICR_CTSCF_Pos           (9U)                                     
#define USART_ICR_CTSCF_Msk           (0x1U << USART_ICR_CTSCF_Pos)            /*!< 0x00000200 */
#define USART_ICR_CTSCF               USART_ICR_CTSCF_Msk                      /*!< CTS Interrupt Clear Flag */
#define USART_ICR_RTOCF_Pos           (11U)                                    
#define USART_ICR_RTOCF_Msk           (0x1U << USART_ICR_RTOCF_Pos)            /*!< 0x00000800 */
#define USART_ICR_RTOCF               USART_ICR_RTOCF_Msk                      /*!< Receiver Time Out Clear Flag */
#define USART_ICR_EOBCF_Pos           (12U)                                    
#define USART_ICR_EOBCF_Msk           (0x1U << USART_ICR_EOBCF_Pos)            /*!< 0x00001000 */
#define USART_ICR_EOBCF               USART_ICR_EOBCF_Msk                      /*!< End Of Block Clear Flag */
#define USART_ICR_CMCF_Pos            (17U)                                    
#define USART_ICR_CMCF_Msk            (0x1U << USART_ICR_CMCF_Pos)             /*!< 0x00020000 */
#define USART_ICR_CMCF                USART_ICR_CMCF_Msk                       /*!< Character Match Clear Flag */
#define USART_ICR_WUCF_Pos            (20U)                                    
#define USART_ICR_WUCF_Msk            (0x1U << USART_ICR_WUCF_Pos)             /*!< 0x00100000 */
#define USART_ICR_WUCF                USART_ICR_WUCF_Msk                       /*!< Wake Up from stop mode Clear Flag */

/*******************  Bit definition for USART_RDR register  ******************/
#define USART_RDR_RDR_Pos             (0U)                                     
#define USART_RDR_RDR_Msk             (0x1FFU << USART_RDR_RDR_Pos)            /*!< 0x000001FF */
#define USART_RDR_RDR                 USART_RDR_RDR_Msk                        /*!< RDR[8:0] bits (Receive Data value) */

/*******************  Bit definition for USART_TDR register  ******************/
#define USART_TDR_TDR_Pos             (0U)                                     
#define USART_TDR_TDR_Msk             (0x1FFU << USART_TDR_TDR_Pos)            /*!< 0x000001FF */
#define USART_TDR_TDR                 USART_TDR_TDR_Msk                        /*!< TDR[8:0] bits (Transmit Data value) */

/******************************************************************************/
/*                                                                            */
/*                         Window WATCHDOG (WWDG)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for WWDG_CR register  ********************/
#define WWDG_CR_T_Pos           (0U)                                           
#define WWDG_CR_T_Msk           (0x7FU << WWDG_CR_T_Pos)                       /*!< 0x0000007F */
#define WWDG_CR_T               WWDG_CR_T_Msk                                  /*!< T[6:0] bits (7-Bit counter (MSB to LSB)) */
#define WWDG_CR_T_0             (0x01U << WWDG_CR_T_Pos)                       /*!< 0x00000001 */
#define WWDG_CR_T_1             (0x02U << WWDG_CR_T_Pos)                       /*!< 0x00000002 */
#define WWDG_CR_T_2             (0x04U << WWDG_CR_T_Pos)                       /*!< 0x00000004 */
#define WWDG_CR_T_3             (0x08U << WWDG_CR_T_Pos)                       /*!< 0x00000008 */
#define WWDG_CR_T_4             (0x10U << WWDG_CR_T_Pos)                       /*!< 0x00000010 */
#define WWDG_CR_T_5             (0x20U << WWDG_CR_T_Pos)                       /*!< 0x00000020 */
#define WWDG_CR_T_6             (0x40U << WWDG_CR_T_Pos)                       /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CR_T0    WWDG_CR_T_0
#define  WWDG_CR_T1    WWDG_CR_T_1
#define  WWDG_CR_T2    WWDG_CR_T_2
#define  WWDG_CR_T3    WWDG_CR_T_3
#define  WWDG_CR_T4    WWDG_CR_T_4
#define  WWDG_CR_T5    WWDG_CR_T_5
#define  WWDG_CR_T6    WWDG_CR_T_6

#define WWDG_CR_WDGA_Pos        (7U)                                           
#define WWDG_CR_WDGA_Msk        (0x1U << WWDG_CR_WDGA_Pos)                     /*!< 0x00000080 */
#define WWDG_CR_WDGA            WWDG_CR_WDGA_Msk                               /*!< Activation bit */

/*******************  Bit definition for WWDG_CFR register  *******************/
#define WWDG_CFR_W_Pos          (0U)                                           
#define WWDG_CFR_W_Msk          (0x7FU << WWDG_CFR_W_Pos)                      /*!< 0x0000007F */
#define WWDG_CFR_W              WWDG_CFR_W_Msk                                 /*!< W[6:0] bits (7-bit window value) */
#define WWDG_CFR_W_0            (0x01U << WWDG_CFR_W_Pos)                      /*!< 0x00000001 */
#define WWDG_CFR_W_1            (0x02U << WWDG_CFR_W_Pos)                      /*!< 0x00000002 */
#define WWDG_CFR_W_2            (0x04U << WWDG_CFR_W_Pos)                      /*!< 0x00000004 */
#define WWDG_CFR_W_3            (0x08U << WWDG_CFR_W_Pos)                      /*!< 0x00000008 */
#define WWDG_CFR_W_4            (0x10U << WWDG_CFR_W_Pos)                      /*!< 0x00000010 */
#define WWDG_CFR_W_5            (0x20U << WWDG_CFR_W_Pos)                      /*!< 0x00000020 */
#define WWDG_CFR_W_6            (0x40U << WWDG_CFR_W_Pos)                      /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CFR_W0    WWDG_CFR_W_0
#define  WWDG_CFR_W1    WWDG_CFR_W_1
#define  WWDG_CFR_W2    WWDG_CFR_W_2
#define  WWDG_CFR_W3    WWDG_CFR_W_3
#define  WWDG_CFR_W4    WWDG_CFR_W_4
#define  WWDG_CFR_W5    WWDG_CFR_W_5
#define  WWDG_CFR_W6    WWDG_CFR_W_6

#define WWDG_CFR_WDGTB_Pos      (7U)                                           
#define WWDG_CFR_WDGTB_Msk      (0x3U << WWDG_CFR_WDGTB_Pos)                   /*!< 0x00000180 */
#define WWDG_CFR_WDGTB          WWDG_CFR_WDGTB_Msk                             /*!< WDGTB[1:0] bits (Timer Base) */
#define WWDG_CFR_WDGTB_0        (0x1U << WWDG_CFR_WDGTB_Pos)                   /*!< 0x00000080 */
#define WWDG_CFR_WDGTB_1        (0x2U << WWDG_CFR_WDGTB_Pos)                   /*!< 0x00000100 */

/* Legacy defines */
#define  WWDG_CFR_WDGTB0    WWDG_CFR_WDGTB_0
#define  WWDG_CFR_WDGTB1    WWDG_CFR_WDGTB_1

#define WWDG_CFR_EWI_Pos        (9U)                                           
#define WWDG_CFR_EWI_Msk        (0x1U << WWDG_CFR_EWI_Pos)                     /*!< 0x00000200 */
#define WWDG_CFR_EWI            WWDG_CFR_EWI_Msk                               /*!< Early Wakeup Interrupt */

/*******************  Bit definition for WWDG_SR register  ********************/
#define WWDG_SR_EWIF_Pos        (0U)                                           
#define WWDG_SR_EWIF_Msk        (0x1U << WWDG_SR_EWIF_Pos)                     /*!< 0x00000001 */
#define WWDG_SR_EWIF            WWDG_SR_EWIF_Msk                               /*!< Early Wakeup Interrupt Flag */

/**
  * @}
  */ 

/**
  * @}
  */

/** @addtogroup Exported_macros
  * @{
  */

/******************************* ADC Instances ********************************/
#define IS_ADC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == ADC1)
#define IS_ADC_COMMON_INSTANCE(INSTANCE) ((INSTANCE) == ADC1_COMMON)

/******************************* COMP Instances *******************************/
#define IS_COMP_ALL_INSTANCE(INSTANCE) (((INSTANCE) == COMP1) || \
                                       ((INSTANCE) == COMP2))

#define IS_COMP_COMMON_INSTANCE(INSTANCE) ((INSTANCE) == COMP12_COMMON)

/******************************* CRC Instances ********************************/
#define IS_CRC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == CRC)

/******************************* DMA Instances *********************************/
#define IS_DMA_ALL_INSTANCE(INSTANCE) (((INSTANCE) == DMA1_Channel1) || \
                                       ((INSTANCE) == DMA1_Channel2) || \
                                       ((INSTANCE) == DMA1_Channel3) || \
                                       ((INSTANCE) == DMA1_Channel4) || \
                                       ((INSTANCE) == DMA1_Channel5) || \
                                       ((INSTANCE) == DMA1_Channel6) || \
                                       ((INSTANCE) == DMA1_Channel7))

/******************************* GPIO Instances *******************************/
#define IS_GPIO_ALL_INSTANCE(INSTANCE) (((INSTANCE) == GPIOA) || \
                                        ((INSTANCE) == GPIOB) || \
                                        ((INSTANCE) == GPIOC) || \
                                        ((INSTANCE) == GPIOD) || \
                                        ((INSTANCE) == GPIOH))

#define IS_GPIO_AF_INSTANCE(INSTANCE)  (((INSTANCE) == GPIOA) || \
                                        ((INSTANCE) == GPIOB) || \
                                        ((INSTANCE) == GPIOC) || \
                                        ((INSTANCE) == GPIOD))

/******************************** I2C Instances *******************************/
#define IS_I2C_ALL_INSTANCE(INSTANCE) (((INSTANCE) == I2C1) || \
                                       ((INSTANCE) == I2C2))

/****************** I2C Instances : wakeup capability from stop modes *********/
#define IS_I2C_WAKEUP_FROMSTOP_INSTANCE(INSTANCE) (((INSTANCE) == I2C1))


/******************************** I2S Instances *******************************/
#define IS_I2S_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == SPI2)


/****************************** RTC Instances *********************************/
#define IS_RTC_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == RTC)

/******************************** SMBUS Instances *****************************/
#define IS_SMBUS_INSTANCE(INSTANCE)  ((INSTANCE) == I2C1)

/******************************** SPI Instances *******************************/
#define IS_SPI_ALL_INSTANCE(INSTANCE) (((INSTANCE) == SPI1) || \
                                       ((INSTANCE) == SPI2))

/****************** LPTIM Instances : All supported instances *****************/
#define IS_LPTIM_INSTANCE(INSTANCE)       ((INSTANCE) == LPTIM1)

/****************** TIM Instances : All supported instances *******************/
#define IS_TIM_INSTANCE(INSTANCE)       (((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM6)   || \
                                         ((INSTANCE) == TIM21)  || \
                                         ((INSTANCE) == TIM22))

/****************** TIM Instances : supporting counting mode selection ********/ 
#define IS_TIM_COUNTER_MODE_SELECT_INSTANCE(INSTANCE)       (((INSTANCE) == TIM2)   || \
                                                             ((INSTANCE) == TIM21)  || \
                                                             ((INSTANCE) == TIM22))

/****************** TIM Instances : supporting clock division *****************/
#define IS_TIM_CLOCK_DIVISION_INSTANCE(INSTANCE)       (((INSTANCE) == TIM2)   || \
                                                        ((INSTANCE) == TIM21)  || \
                                                        ((INSTANCE) == TIM22))

/****** TIM Instances : supporting external clock mode 1 for ETRF input *******/
#define IS_TIM_CLOCKSOURCE_ETRMODE1_INSTANCE(INSTANCE)   (((INSTANCE) == TIM2)   || \
                                                          ((INSTANCE) == TIM21))

/****** TIM Instances : supporting external clock mode 2 for ETRF input *******/
#define IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE(INSTANCE)   (((INSTANCE) == TIM2)   || \
                                                          ((INSTANCE) == TIM21)  || \
                                                          ((INSTANCE) == TIM22))

/****************** TIM Instances : supporting external clock mode 1 for TIX inputs*/
#define IS_TIM_CLOCKSOURCE_TIX_INSTANCE(INSTANCE)        (((INSTANCE) == TIM2)   || \
                                                          ((INSTANCE) == TIM21))

/****************** TIM Instances : supporting internal trigger inputs(ITRX) *******/
#define IS_TIM_CLOCKSOURCE_ITRX_INSTANCE(INSTANCE)       (((INSTANCE) == TIM2)   || \
                                                          ((INSTANCE) == TIM21)  || \
                                                          ((INSTANCE) == TIM22))

/************* TIM Instances : at least 1 capture/compare channel *************/
#define IS_TIM_CC1_INSTANCE(INSTANCE)   (((INSTANCE) == TIM2)  || \
                                         ((INSTANCE) == TIM21) || \
                                         ((INSTANCE) == TIM22))

/************ TIM Instances : at least 2 capture/compare channels *************/
#define IS_TIM_CC2_INSTANCE(INSTANCE)  (((INSTANCE) == TIM2)  || \
                                        ((INSTANCE) == TIM21) || \
                                        ((INSTANCE) == TIM22))

/************ TIM Instances : at least 3 capture/compare channels *************/
#define IS_TIM_CC3_INSTANCE(INSTANCE)   ((INSTANCE) == TIM2)

/************ TIM Instances : at least 4 capture/compare channels *************/
#define IS_TIM_CC4_INSTANCE(INSTANCE)   ((INSTANCE) == TIM2)

/******************** TIM Instances : Advanced-control timers *****************/

/******************* TIM Instances : Timer input XOR function *****************/
#define IS_TIM_XOR_INSTANCE(INSTANCE)   ((INSTANCE) == TIM2)

/****************** TIM Instances : DMA requests generation (UDE) *************/
#define IS_TIM_DMA_INSTANCE(INSTANCE)      (((INSTANCE) == TIM2) || \
                                            ((INSTANCE) == TIM6))

/************ TIM Instances : DMA requests generation (CCxDE) *****************/
#define IS_TIM_DMA_CC_INSTANCE(INSTANCE)   ((INSTANCE) == TIM2)

/************ TIM Instances : DMA requests generation (COMDE) *****************/
#define IS_TIM_CCDMA_INSTANCE(INSTANCE)    ((INSTANCE) == TIM2)

/******************** TIM Instances : DMA burst feature ***********************/
#define IS_TIM_DMABURST_INSTANCE(INSTANCE)  ((INSTANCE) == TIM2)

/****** TIM Instances : master mode available (TIMx_CR2.MMS available )********/
#define IS_TIM_MASTER_INSTANCE(INSTANCE)   (((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM6)  || \
                                            ((INSTANCE) == TIM21) || \
                                            ((INSTANCE) == TIM22))

/*********** TIM Instances : Slave mode available (TIMx_SMCR available )*******/
#define IS_TIM_SLAVE_INSTANCE(INSTANCE)    (((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM21) || \
                                            ((INSTANCE) == TIM22))

/********************** TIM Instances : 32 bit Counter ************************/

/***************** TIM Instances : external trigger input availabe ************/
#define IS_TIM_ETR_INSTANCE(INSTANCE)      (((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM21) || \
                                            ((INSTANCE) == TIM22))

/****************** TIM Instances : remapping capability **********************/
#define IS_TIM_REMAP_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM21)  || \
                                         ((INSTANCE) == TIM22))

/****************** TIM Instances : supporting encoder interface **************/
#define IS_TIM_ENCODER_INTERFACE_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)   || \
                                                     ((INSTANCE) == TIM21)  || \
                                                     ((INSTANCE) == TIM22))

/******************* TIM Instances : output(s) OCXEC register *****************/
#define IS_TIM_OCXREF_CLEAR_INSTANCE(INSTANCE)  ((INSTANCE) == TIM2)
													   
/******************* TIM Instances : output(s) available **********************/
#define IS_TIM_CCX_INSTANCE(INSTANCE, CHANNEL) \
    ((((INSTANCE) == TIM2) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
     ||                                        \
     (((INSTANCE) == TIM21) &&                 \
      (((CHANNEL) == TIM_CHANNEL_1) ||         \
       ((CHANNEL) == TIM_CHANNEL_2)))          \
     ||                                        \
     (((INSTANCE) == TIM22) &&                 \
      (((CHANNEL) == TIM_CHANNEL_1) ||         \
       ((CHANNEL) == TIM_CHANNEL_2))))

/******************** UART Instances : Asynchronous mode **********************/
#define IS_UART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2) || \
                                    ((INSTANCE) == LPUART1))

/******************** USART Instances : Synchronous mode **********************/
#define IS_USART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                     ((INSTANCE) == USART2))

/****************** USART Instances : Auto Baud Rate detection ****************/

#define IS_USART_AUTOBAUDRATE_DETECTION_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                                            ((INSTANCE) == USART2))

/******************** UART Instances : Half-Duplex mode **********************/
#define IS_UART_HALFDUPLEX_INSTANCE(INSTANCE)    (((INSTANCE) == USART1) || \
                                                  ((INSTANCE) == USART2) || \
                                                  ((INSTANCE) == LPUART1))

/******************** UART Instances : LIN mode **********************/
#define IS_UART_LIN_INSTANCE(INSTANCE)    (((INSTANCE) == USART1) || \
                                           ((INSTANCE) == USART2))

/******************** UART Instances : Wake-up from Stop mode **********************/
#define IS_UART_WAKEUP_FROMSTOP_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                                      ((INSTANCE) == USART2) || \
                                                      ((INSTANCE) == LPUART1))
/****************** UART Instances : Hardware Flow control ********************/
#define IS_UART_HWFLOW_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                           ((INSTANCE) == USART2) || \
                                           ((INSTANCE) == LPUART1))

/********************* UART Instances : Smard card mode ***********************/
#define IS_SMARTCARD_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                         ((INSTANCE) == USART2))

/*********************** UART Instances : IRDA mode ***************************/
#define IS_IRDA_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2))

/******************** LPUART Instance *****************************************/
#define IS_LPUART_INSTANCE(INSTANCE)    ((INSTANCE) == LPUART1)

/****************************** IWDG Instances ********************************/
#define IS_IWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == IWDG)

/****************************** WWDG Instances ********************************/
#define IS_WWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == WWDG)

/**
  * @}
  */

/******************************************************************************/
/*  For a painless codes migration between the STM32L0xx device product       */
/*  lines, the aliases defined below are put in place to overcome the         */
/*  differences in the interrupt handlers and IRQn definitions.               */
/*  No need to update developed interrupt code when moving across             */ 
/*  product lines within the same STM32L0 Family                              */
/******************************************************************************/

/* Aliases for __IRQn */

#define RNG_LPUART1_IRQn               LPUART1_IRQn
#define AES_LPUART1_IRQn               LPUART1_IRQn
#define AES_RNG_LPUART1_IRQn           LPUART1_IRQn
#define TIM6_DAC_IRQn                  TIM6_IRQn
#define RCC_CRS_IRQn                   RCC_IRQn

/* Aliases for __IRQHandler */
#define RNG_LPUART1_IRQHandler         LPUART1_IRQHandler
#define AES_LPUART1_IRQHandler         LPUART1_IRQHandler
#define AES_RNG_LPUART1_IRQHandler     LPUART1_IRQHandler
#define TIM6_DAC_IRQHandler            TIM6_IRQHandler
#define RCC_CRS_IRQHandler             RCC_IRQHandler

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32L051xx_H */



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
