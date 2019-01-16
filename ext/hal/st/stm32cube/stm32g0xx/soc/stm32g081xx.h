/**
  ******************************************************************************
  * @file    stm32g081xx.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M0+ Device Peripheral Access Layer Header File.
  *          This file contains all the peripheral register's definitions, bits
  *          definitions and memory mapping for stm32g081xx devices.
  *
  *          This file contains:
  *           - Data structures and the address mapping for all peripherals
  *           - Peripheral's registers declarations and bits definition
  *           - Macros to access peripheral's registers hardware
  *
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; Copyright (c) 2018 STMicroelectronics. 
  * All rights reserved.</center></h2>
  *
  * This software component is licensed by ST under BSD 3-Clause license,
  * the "License"; You may not use this file except in compliance with the 
  * License. You may obtain a copy of the License at:
  *                        opensource.org/licenses/BSD-3-Clause
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS_Device
  * @{
  */

/** @addtogroup stm32g081xx
  * @{
  */

#ifndef STM32G081xx_H
#define STM32G081xx_H

#ifdef __cplusplus
 extern "C" {
#endif /* __cplusplus */

/** @addtogroup Configuration_section_for_CMSIS
  * @{
  */

/**
  * @brief Configuration of the Cortex-M0+ Processor and Core Peripherals
   */
#define __CM0PLUS_REV             0 /*!< Core Revision r0p0                            */
#define __MPU_PRESENT             1 /*!< STM32G0xx  provides an MPU                    */
#define __VTOR_PRESENT            1 /*!< Vector  Table  Register supported             */
#define __NVIC_PRIO_BITS          2 /*!< STM32G0xx uses 2 Bits for the Priority Levels */
#define __Vendor_SysTickConfig    0 /*!< Set to 1 if different SysTick Config is used  */

/**
  * @}
  */

/** @addtogroup Peripheral_interrupt_number_definition
  * @{
  */

/**
 * @brief stm32g081xx Interrupt Number Definition, according to the selected device
 *        in @ref Library_configuration_section
 */

/*!< Interrupt Number Definition */
typedef enum
{
/******  Cortex-M0+ Processor Exceptions Numbers ***************************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                          */
  HardFault_IRQn              = -13,    /*!< 3 Cortex-M Hard Fault Interrupt                                   */
  SVC_IRQn                    = -5,     /*!< 11 Cortex-M SV Call Interrupt                                     */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M Pend SV Interrupt                                     */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M System Tick Interrupt                                 */
/******  STM32G0xxxx specific Interrupt Numbers ****************************************************************/
  WWDG_IRQn                   = 0,      /*!< Window WatchDog Interrupt                                         */
  PVD_IRQn                    = 1,      /*!< PVD through EXTI Line detection Interrupt(EXTI line 16)           */
  RTC_TAMP_IRQn               = 2,      /*!< RTC interrupt through the EXTI line 19 & 21                       */
  FLASH_IRQn                  = 3,      /*!< FLASH global Interrupt                                            */
  RCC_IRQn                    = 4,      /*!< RCC global Interrupt                                              */
  EXTI0_1_IRQn                = 5,      /*!< EXTI 0 and 1 Interrupts                                           */
  EXTI2_3_IRQn                = 6,      /*!< EXTI Line 2 and 3 Interrupts                                      */
  EXTI4_15_IRQn               = 7,      /*!< EXTI Line 4 to 15 Interrupts                                      */
  UCPD1_2_IRQn                = 8,      /*!< UCPD1 and UCPD2 global Interrupt                                  */
  DMA1_Channel1_IRQn          = 9,      /*!< DMA1 Channel 1 Interrupt                                          */
  DMA1_Channel2_3_IRQn        = 10,     /*!< DMA1 Channel 2 and Channel 3 Interrupts                           */
  DMA1_Ch4_7_DMAMUX1_OVR_IRQn = 11,     /*!< DMA1 Channel 4 to Channel 7 and DMAMUX1 Overrun Interrupts        */
  ADC1_COMP_IRQn              = 12,     /*!< ADC1, COMP1 and COMP2 Interrupts (combined with EXTI 17 & 18)     */
  TIM1_BRK_UP_TRG_COM_IRQn    = 13,     /*!< TIM1 Break, Update, Trigger and Commutation Interrupts            */
  TIM1_CC_IRQn                = 14,     /*!< TIM1 Capture Compare Interrupt                                    */
  TIM2_IRQn                   = 15,     /*!< TIM2 Interrupt                                                    */
  TIM3_IRQn                   = 16,     /*!< TIM3 global Interrupt                                             */
  TIM6_DAC_LPTIM1_IRQn        = 17,     /*!< TIM6, DAC and LPTIM1 global Interrupts                            */
  TIM7_LPTIM2_IRQn            = 18,     /*!< TIM7 and LPTIM2 global Interrupt                                  */
  TIM14_IRQn                  = 19,     /*!< TIM14 global Interrupt                                            */
  TIM15_IRQn                  = 20,     /*!< TIM15 global Interrupt                                            */
  TIM16_IRQn                  = 21,     /*!< TIM16 global Interrupt                                            */
  TIM17_IRQn                  = 22,     /*!< TIM17 global Interrupt                                            */
  I2C1_IRQn                   = 23,     /*!< I2C1 Interrupt  (combined with EXTI 23)                           */
  I2C2_IRQn                   = 24,     /*!< I2C2 Interrupt                                                    */
  SPI1_IRQn                   = 25,     /*!< SPI1 Interrupt                                                    */
  SPI2_IRQn                   = 26,     /*!< SPI2 Interrupt                                                    */
  USART1_IRQn                 = 27,     /*!< USART1 Interrupt                                                  */
  USART2_IRQn                 = 28,     /*!< USART2 Interrupt                                                  */
  USART3_4_LPUART1_IRQn       = 29,     /*!< USART3, USART4 and LPUART1 globlal Interrupts (combined with EXTI 28) */
  CEC_IRQn                    = 30,     /*!< CEC Interrupt(combined with EXTI 27)                               */
  AES_RNG_IRQn                = 31,     /*!< AES & RNG Interrupt                                                */
} IRQn_Type;

/**
  * @}
  */

#include "core_cm0plus.h"               /* Cortex-M0+ processor and core peripherals */
#include "system_stm32g0xx.h"
#include <stdint.h>

/** @addtogroup Peripheral_registers_structures
  * @{
  */

/**
  * @brief Analog to Digital Converter
  */
typedef struct
{
  __IO uint32_t ISR;          /*!< ADC interrupt and status register,             Address offset: 0x00 */
  __IO uint32_t IER;          /*!< ADC interrupt enable register,                 Address offset: 0x04 */
  __IO uint32_t CR;           /*!< ADC control register,                          Address offset: 0x08 */
  __IO uint32_t CFGR1;        /*!< ADC configuration register 1,                  Address offset: 0x0C */
  __IO uint32_t CFGR2;        /*!< ADC configuration register 2,                  Address offset: 0x10 */
  __IO uint32_t SMPR;         /*!< ADC sampling time register,                    Address offset: 0x14 */
       uint32_t RESERVED1;    /*!< Reserved,                                                      0x18 */
       uint32_t RESERVED2;    /*!< Reserved,                                                      0x1C */
  __IO uint32_t TR1;          /*!< ADC analog watchdog 1 threshold register,      Address offset: 0x20 */
  __IO uint32_t TR2;          /*!< ADC analog watchdog 2 threshold register,      Address offset: 0x24 */
  __IO uint32_t CHSELR;       /*!< ADC group regular sequencer register,          Address offset: 0x28 */
  __IO uint32_t TR3;          /*!< ADC analog watchdog 3 threshold register,      Address offset: 0x2C */
       uint32_t RESERVED3[4]; /*!< Reserved,                                               0x30 - 0x3C */
  __IO uint32_t DR;           /*!< ADC group regular data register,               Address offset: 0x40 */
       uint32_t RESERVED4[23];/*!< Reserved,                                               0x44 - 0x9C */
  __IO uint32_t AWD2CR;       /*!< ADC analog watchdog 2 configuration register,  Address offset: 0xA0 */
  __IO uint32_t AWD3CR;       /*!< ADC analog watchdog 3 configuration register,  Address offset: 0xA4 */
       uint32_t RESERVED5[3]; /*!< Reserved,                                               0xA8 - 0xB0 */
  __IO uint32_t CALFACT;      /*!< ADC Calibration factor register,               Address offset: 0xB4 */
} ADC_TypeDef;

typedef struct
{
  __IO uint32_t CCR;          /*!< ADC common configuration register,             Address offset: ADC1 base address + 0x308 */
} ADC_Common_TypeDef;

/**
  * @brief HDMI-CEC
  */
typedef struct
{
  __IO uint32_t CR;           /*!< CEC control register,                                       Address offset:0x00 */
  __IO uint32_t CFGR;         /*!< CEC configuration register,                                 Address offset:0x04 */
  __IO uint32_t TXDR;         /*!< CEC Tx data register ,                                      Address offset:0x08 */
  __IO uint32_t RXDR;         /*!< CEC Rx Data Register,                                       Address offset:0x0C */
  __IO uint32_t ISR;          /*!< CEC Interrupt and Status Register,                          Address offset:0x10 */
  __IO uint32_t IER;          /*!< CEC interrupt enable register,                              Address offset:0x14 */
}CEC_TypeDef;

/**
  * @brief Comparator
  */
typedef struct
{
  __IO uint32_t CSR;         /*!< COMP control and status register,                                                 Address offset: 0x00 */
} COMP_TypeDef;

typedef struct
{
  __IO uint32_t CSR_ODD;    /*!< COMP control and status register located in register of comparator instance odd, used for bits common to several COMP instances, Address offset: 0x00 */
  __IO uint32_t CSR_EVEN;   /*!< COMP control and status register located in register of comparator instance even, used for bits common to several COMP instances, Address offset: 0x04 */
} COMP_Common_TypeDef;

/**
  * @brief CRC calculation unit
  */
typedef struct
{
  __IO uint32_t DR;             /*!< CRC Data register,                         Address offset: 0x00 */
  __IO uint32_t IDR;            /*!< CRC Independent data register,             Address offset: 0x04 */
  __IO uint32_t CR;             /*!< CRC Control register,                      Address offset: 0x08 */
       uint32_t RESERVED1;      /*!< Reserved,                                                  0x0C */
  __IO uint32_t INIT;           /*!< Initial CRC value register,                Address offset: 0x10 */
  __IO uint32_t POL;            /*!< CRC polynomial register,                   Address offset: 0x14 */
} CRC_TypeDef;

/**
  * @brief Digital to Analog Converter
  */
typedef struct
{
  __IO uint32_t CR;          /*!< DAC control register,                                    Address offset: 0x00 */
  __IO uint32_t SWTRIGR;     /*!< DAC software trigger register,                           Address offset: 0x04 */
  __IO uint32_t DHR12R1;     /*!< DAC channel1 12-bit right-aligned data holding register, Address offset: 0x08 */
  __IO uint32_t DHR12L1;     /*!< DAC channel1 12-bit left aligned data holding register,  Address offset: 0x0C */
  __IO uint32_t DHR8R1;      /*!< DAC channel1 8-bit right aligned data holding register,  Address offset: 0x10 */
  __IO uint32_t DHR12R2;     /*!< DAC channel2 12-bit right aligned data holding register, Address offset: 0x14 */
  __IO uint32_t DHR12L2;     /*!< DAC channel2 12-bit left aligned data holding register,  Address offset: 0x18 */
  __IO uint32_t DHR8R2;      /*!< DAC channel2 8-bit right-aligned data holding register,  Address offset: 0x1C */
  __IO uint32_t DHR12RD;     /*!< Dual DAC 12-bit right-aligned data holding register,     Address offset: 0x20 */
  __IO uint32_t DHR12LD;     /*!< DUAL DAC 12-bit left aligned data holding register,      Address offset: 0x24 */
  __IO uint32_t DHR8RD;      /*!< DUAL DAC 8-bit right aligned data holding register,      Address offset: 0x28 */
  __IO uint32_t DOR1;        /*!< DAC channel1 data output register,                       Address offset: 0x2C */
  __IO uint32_t DOR2;        /*!< DAC channel2 data output register,                       Address offset: 0x30 */
  __IO uint32_t SR;          /*!< DAC status register,                                     Address offset: 0x34 */
  __IO uint32_t CCR;         /*!< DAC calibration control register,                        Address offset: 0x38 */
  __IO uint32_t MCR;         /*!< DAC mode control register,                               Address offset: 0x3C */
  __IO uint32_t SHSR1;       /*!< DAC Sample and Hold sample time register 1,              Address offset: 0x40 */
  __IO uint32_t SHSR2;       /*!< DAC Sample and Hold sample time register 2,              Address offset: 0x44 */
  __IO uint32_t SHHR;        /*!< DAC Sample and Hold hold time register,                  Address offset: 0x48 */
  __IO uint32_t SHRR;        /*!< DAC Sample and Hold refresh time register,               Address offset: 0x4C */
} DAC_TypeDef;

/**
  * @brief Debug MCU
  */
typedef struct
{
  __IO uint32_t IDCODE;      /*!< MCU device ID code,              Address offset: 0x00 */
  __IO uint32_t CR;          /*!< Debug configuration register,    Address offset: 0x04 */
  __IO uint32_t APBFZ1;      /*!< Debug APB freeze register 1,     Address offset: 0x08 */
  __IO uint32_t APBFZ2;      /*!< Debug APB freeze register 2,     Address offset: 0x0C */
} DBG_TypeDef;

/**
  * @brief DMA Controller
  */
typedef struct
{
  __IO uint32_t CCR;         /*!< DMA channel x configuration register        */
  __IO uint32_t CNDTR;       /*!< DMA channel x number of data register       */
  __IO uint32_t CPAR;        /*!< DMA channel x peripheral address register   */
  __IO uint32_t CMAR;        /*!< DMA channel x memory address register       */
} DMA_Channel_TypeDef;

typedef struct
{
  __IO uint32_t ISR;         /*!< DMA interrupt status register,                 Address offset: 0x00 */
  __IO uint32_t IFCR;        /*!< DMA interrupt flag clear register,             Address offset: 0x04 */
} DMA_TypeDef;

/**
  * @brief DMA Multiplexer
  */
typedef struct
{
  __IO uint32_t   CCR;       /*!< DMA Multiplexer Channel x Control Register    Address offset: 0x0004 * (channel x) */
}DMAMUX_Channel_TypeDef;

typedef struct
{
  __IO uint32_t   CSR;       /*!< DMA Channel Status Register                    Address offset: 0x0080   */
  __IO uint32_t   CFR;       /*!< DMA Channel Clear Flag Register                Address offset: 0x0084   */
}DMAMUX_ChannelStatus_TypeDef;

typedef struct
{
  __IO uint32_t   RGCR;        /*!< DMA Request Generator x Control Register     Address offset: 0x0100 + 0x0004 * (Req Gen x) */
}DMAMUX_RequestGen_TypeDef;

typedef struct
{
  __IO uint32_t   RGSR;        /*!< DMA Request Generator Status Register        Address offset: 0x0140   */
  __IO uint32_t   RGCFR;       /*!< DMA Request Generator Clear Flag Register    Address offset: 0x0144   */
}DMAMUX_RequestGenStatus_TypeDef;

/**
  * @brief Asynch Interrupt/Event Controller (EXTI)
  */
typedef struct
{
  __IO uint32_t RTSR1;          /*!< EXTI Rising Trigger Selection Register 1,        Address offset:   0x00 */
  __IO uint32_t FTSR1;          /*!< EXTI Falling Trigger Selection Register 1,       Address offset:   0x04 */
  __IO uint32_t SWIER1;         /*!< EXTI Software Interrupt event Register 1,        Address offset:   0x08 */
  __IO uint32_t RPR1;           /*!< EXTI Rising Pending Register 1,                  Address offset:   0x0C */
  __IO uint32_t FPR1;           /*!< EXTI Falling Pending Register 1,                 Address offset:   0x10 */
       uint32_t RESERVED1[3];   /*!< Reserved 1,                                                0x14 -- 0x1C */
       uint32_t RESERVED2[5];   /*!< Reserved 2,                                                0x20 -- 0x30 */
       uint32_t RESERVED3[11];  /*!< Reserved 3,                                                0x34 -- 0x5C */
  __IO uint32_t EXTICR[4];      /*!< EXTI External Interrupt Configuration Register,            0x60 -- 0x6C */
       uint32_t RESERVED4[4];   /*!< Reserved 4,                                                0x70 -- 0x7C */
  __IO uint32_t IMR1;           /*!< EXTI Interrupt Mask Register 1,                  Address offset:   0x80 */
  __IO uint32_t EMR1;           /*!< EXTI Event Mask Register 1,                      Address offset:   0x84 */
       uint32_t RESERVED5[2];   /*!< Reserved 5,                                                0x88 -- 0x8C */
  __IO uint32_t IMR2;           /*!< EXTI Interrupt Mask Register 2,                  Address offset:   0x90 */
  __IO uint32_t EMR2;           /*!< EXTI Event Mask Register 2,                      Address offset:   0x94 */
} EXTI_TypeDef;

/**
  * @brief FLASH Registers
  */
typedef struct
{
  __IO uint32_t ACR;          /*!< FLASH Access Control register,                     Address offset: 0x00 */
       uint32_t RESERVED1;    /*!< Reserved1,                                         Address offset: 0x04 */
  __IO uint32_t KEYR;         /*!< FLASH Key register,                                Address offset: 0x08 */
  __IO uint32_t OPTKEYR;      /*!< FLASH Option Key register,                         Address offset: 0x0C */
  __IO uint32_t SR;           /*!< FLASH Status register,                             Address offset: 0x10 */
  __IO uint32_t CR;           /*!< FLASH Control register,                            Address offset: 0x14 */
  __IO uint32_t ECCR;         /*!< FLASH ECC register,                                Address offset: 0x18 */
       uint32_t RESERVED2;    /*!< Reserved2,                                         Address offset: 0x1C */
  __IO uint32_t OPTR;         /*!< FLASH Option register,                             Address offset: 0x20 */
  __IO uint32_t PCROP1ASR;    /*!< FLASH Bank PCROP area A Start address register,    Address offset: 0x24 */
  __IO uint32_t PCROP1AER;    /*!< FLASH Bank PCROP area A End address register,      Address offset: 0x28 */
  __IO uint32_t WRP1AR;       /*!< FLASH Bank WRP area A address register,            Address offset: 0x2C */
  __IO uint32_t WRP1BR;       /*!< FLASH Bank WRP area B address register,            Address offset: 0x30 */
  __IO uint32_t PCROP1BSR;    /*!< FLASH Bank PCROP area B Start address register,    Address offset: 0x34 */
  __IO uint32_t PCROP1BER;    /*!< FLASH Bank PCROP area B End address register,      Address offset: 0x38 */
       uint32_t RESERVED3[17];/*!< Reserved3,                                         Address offset: 0x3C */
  __IO uint32_t SECR;         /*!< FLASH security register ,                          Address offset: 0x80 */
} FLASH_TypeDef;

/**
  * @brief General Purpose I/O
  */
typedef struct
{
  __IO uint32_t MODER;       /*!< GPIO port mode register,               Address offset: 0x00      */
  __IO uint32_t OTYPER;      /*!< GPIO port output type register,        Address offset: 0x04      */
  __IO uint32_t OSPEEDR;     /*!< GPIO port output speed register,       Address offset: 0x08      */
  __IO uint32_t PUPDR;       /*!< GPIO port pull-up/pull-down register,  Address offset: 0x0C      */
  __IO uint32_t IDR;         /*!< GPIO port input data register,         Address offset: 0x10      */
  __IO uint32_t ODR;         /*!< GPIO port output data register,        Address offset: 0x14      */
  __IO uint32_t BSRR;        /*!< GPIO port bit set/reset  register,     Address offset: 0x18      */
  __IO uint32_t LCKR;        /*!< GPIO port configuration lock register, Address offset: 0x1C      */
  __IO uint32_t AFR[2];      /*!< GPIO alternate function registers,     Address offset: 0x20-0x24 */
  __IO uint32_t BRR;         /*!< GPIO Bit Reset register,               Address offset: 0x28      */
} GPIO_TypeDef;


/**
  * @brief Inter-integrated Circuit Interface
  */
typedef struct
{
  __IO uint32_t CR1;         /*!< I2C Control register 1,            Address offset: 0x00 */
  __IO uint32_t CR2;         /*!< I2C Control register 2,            Address offset: 0x04 */
  __IO uint32_t OAR1;        /*!< I2C Own address 1 register,        Address offset: 0x08 */
  __IO uint32_t OAR2;        /*!< I2C Own address 2 register,        Address offset: 0x0C */
  __IO uint32_t TIMINGR;     /*!< I2C Timing register,               Address offset: 0x10 */
  __IO uint32_t TIMEOUTR;    /*!< I2C Timeout register,              Address offset: 0x14 */
  __IO uint32_t ISR;         /*!< I2C Interrupt and status register, Address offset: 0x18 */
  __IO uint32_t ICR;         /*!< I2C Interrupt clear register,      Address offset: 0x1C */
  __IO uint32_t PECR;        /*!< I2C PEC register,                  Address offset: 0x20 */
  __IO uint32_t RXDR;        /*!< I2C Receive data register,         Address offset: 0x24 */
  __IO uint32_t TXDR;        /*!< I2C Transmit data register,        Address offset: 0x28 */
} I2C_TypeDef;

/**
  * @brief Independent WATCHDOG
  */
typedef struct
{
  __IO uint32_t KR;          /*!< IWDG Key register,       Address offset: 0x00 */
  __IO uint32_t PR;          /*!< IWDG Prescaler register, Address offset: 0x04 */
  __IO uint32_t RLR;         /*!< IWDG Reload register,    Address offset: 0x08 */
  __IO uint32_t SR;          /*!< IWDG Status register,    Address offset: 0x0C */
  __IO uint32_t WINR;        /*!< IWDG Window register,    Address offset: 0x10 */
} IWDG_TypeDef;

/**
  * @brief LPTIMER
  */
typedef struct
{
  __IO uint32_t ISR;              /*!< LPTIM Interrupt and Status register,                Address offset: 0x00 */
  __IO uint32_t ICR;              /*!< LPTIM Interrupt Clear register,                     Address offset: 0x04 */
  __IO uint32_t IER;              /*!< LPTIM Interrupt Enable register,                    Address offset: 0x08 */
  __IO uint32_t CFGR;             /*!< LPTIM Configuration register,                       Address offset: 0x0C */
  __IO uint32_t CR;               /*!< LPTIM Control register,                             Address offset: 0x10 */
  __IO uint32_t CMP;              /*!< LPTIM Compare register,                             Address offset: 0x14 */
  __IO uint32_t ARR;              /*!< LPTIM Autoreload register,                          Address offset: 0x18 */
  __IO uint32_t CNT;              /*!< LPTIM Counter register,                             Address offset: 0x1C */
  __IO uint32_t RESERVED1;        /*!< Reserved1,                                          Address offset: 0x20 */
  __IO uint32_t CFGR2;            /*!< LPTIM Option register,                              Address offset: 0x24 */
} LPTIM_TypeDef;


/**
  * @brief Power Control
  */
typedef struct
{
  __IO uint32_t CR1;          /*!< PWR Power Control Register 1,                     Address offset: 0x00 */
  __IO uint32_t CR2;          /*!< PWR Power Control Register 2,                     Address offset: 0x04 */
  __IO uint32_t CR3;          /*!< PWR Power Control Register 3,                     Address offset: 0x08 */
  __IO uint32_t CR4;          /*!< PWR Power Control Register 4,                     Address offset: 0x0C */
  __IO uint32_t SR1;          /*!< PWR Power Status Register 1,                      Address offset: 0x10 */
  __IO uint32_t SR2;          /*!< PWR Power Status Register 2,                      Address offset: 0x14 */
  __IO uint32_t SCR;          /*!< PWR Power Status Clear Register,                  Address offset: 0x18 */
       uint32_t RESERVED1;    /*!< Reserved,                                         Address offset: 0x1C */
  __IO uint32_t PUCRA;        /*!< PWR Pull-Up Control Register of port A,           Address offset: 0x20 */
  __IO uint32_t PDCRA;        /*!< PWR Pull-Down Control Register of port A,         Address offset: 0x24 */
  __IO uint32_t PUCRB;        /*!< PWR Pull-Up Control Register of port B,           Address offset: 0x28 */
  __IO uint32_t PDCRB;        /*!< PWR Pull-Down Control Register of port B,         Address offset: 0x2C */
  __IO uint32_t PUCRC;        /*!< PWR Pull-Up Control Register of port C,           Address offset: 0x30 */
  __IO uint32_t PDCRC;        /*!< PWR Pull-Down Control Register of port C,         Address offset: 0x34 */
  __IO uint32_t PUCRD;        /*!< PWR Pull-Up Control Register of port D,           Address offset: 0x38 */
  __IO uint32_t PDCRD;        /*!< PWR Pull-Down Control Register of port D,         Address offset: 0x3C */
       uint32_t RESERVED2;    /*!< Reserved,                                         Address offset: 0x40 */
       uint32_t RESERVED3;    /*!< Reserved,                                         Address offset: 0x44 */
  __IO uint32_t PUCRF;        /*!< PWR Pull-Up Control Register of port F,           Address offset: 0x48 */
  __IO uint32_t PDCRF;        /*!< PWR Pull-Down Control Register of port F,         Address offset: 0x4C */
} PWR_TypeDef;

/**
  * @brief Reset and Clock Control
  */
typedef struct
{
  __IO uint32_t CR;          /*!< RCC Clock Sources Control Register,                                     Address offset: 0x00 */
  __IO uint32_t ICSCR;       /*!< RCC Internal Clock Sources Calibration Register,                        Address offset: 0x04 */
  __IO uint32_t CFGR;        /*!< RCC Regulated Domain Clocks Configuration Register,                     Address offset: 0x08 */
  __IO uint32_t PLLCFGR;     /*!< RCC System PLL configuration Register,                                  Address offset: 0x0C */
  __IO uint32_t RESERVED0;   /*!< Reserved,                                                               Address offset: 0x10 */
  __IO uint32_t RESERVED1;   /*!< Reserved,                                                               Address offset: 0x14 */
  __IO uint32_t CIER;        /*!< RCC Clock Interrupt Enable Register,                                    Address offset: 0x18 */
  __IO uint32_t CIFR;        /*!< RCC Clock Interrupt Flag Register,                                      Address offset: 0x1C */
  __IO uint32_t CICR;        /*!< RCC Clock Interrupt Clear Register,                                     Address offset: 0x20 */
  __IO uint32_t IOPRSTR;     /*!< RCC IO port reset register,                                             Address offset: 0x24 */
  __IO uint32_t AHBRSTR;     /*!< RCC AHB peripherals reset register,                                     Address offset: 0x28 */
  __IO uint32_t APBRSTR1;    /*!< RCC APB peripherals reset register 1,                                   Address offset: 0x2C */
  __IO uint32_t APBRSTR2;    /*!< RCC APB peripherals reset register 2,                                   Address offset: 0x30 */
  __IO uint32_t IOPENR;      /*!< RCC IO port enable register,                                            Address offset: 0x34 */
  __IO uint32_t AHBENR;      /*!< RCC AHB peripherals clock enable register,                              Address offset: 0x38 */
  __IO uint32_t APBENR1;     /*!< RCC APB peripherals clock enable register1,                             Address offset: 0x3C */
  __IO uint32_t APBENR2;     /*!< RCC APB peripherals clock enable register2,                             Address offset: 0x40 */
  __IO uint32_t IOPSMENR;    /*!< RCC IO port clocks enable in sleep mode register,                       Address offset: 0x44 */
  __IO uint32_t AHBSMENR;    /*!< RCC AHB peripheral clocks enable in sleep mode register,                Address offset: 0x48 */
  __IO uint32_t APBSMENR1;   /*!< RCC APB peripheral clocks enable in sleep mode register1,               Address offset: 0x4C */
  __IO uint32_t APBSMENR2;   /*!< RCC APB peripheral clocks enable in sleep mode register2,               Address offset: 0x50 */
  __IO uint32_t CCIPR;       /*!< RCC Peripherals Independent Clocks Configuration Register,              Address offset: 0x54 */
  __IO uint32_t RESERVED2;   /*!< Reserved,                                                               Address offset: 0x58 */
  __IO uint32_t BDCR;        /*!< RCC Backup Domain Control Register,                                     Address offset: 0x5C */
  __IO uint32_t CSR;         /*!< RCC Unregulated Domain Clock Control and Status Register,               Address offset: 0x60 */
} RCC_TypeDef;

/**
  * @brief Real-Time Clock
  */
typedef struct
{
  __IO uint32_t TR;          /*!< RTC time register,                                         Address offset: 0x00 */
  __IO uint32_t DR;          /*!< RTC date register,                                         Address offset: 0x04 */
  __IO uint32_t SSR;         /*!< RTC sub second register,                                   Address offset: 0x08 */
  __IO uint32_t ICSR;        /*!< RTC initialization control and status register,            Address offset: 0x0C */
  __IO uint32_t PRER;        /*!< RTC prescaler register,                                    Address offset: 0x10 */
  __IO uint32_t WUTR;        /*!< RTC wakeup timer register,                                 Address offset: 0x14 */
  __IO uint32_t CR;          /*!< RTC control register,                                      Address offset: 0x18 */
       uint32_t RESERVED0;   /*!< Reserved                                                   Address offset: 0x1C */
       uint32_t RESERVED1;   /*!< Reserved                                                   Address offset: 0x20 */
  __IO uint32_t WPR;         /*!< RTC write protection register,                             Address offset: 0x24 */
  __IO uint32_t CALR;        /*!< RTC calibration register,                                  Address offset: 0x28 */
  __IO uint32_t SHIFTR;      /*!< RTC shift control register,                                Address offset: 0x2C */
  __IO uint32_t TSTR;        /*!< RTC time stamp time register,                              Address offset: 0x30 */
  __IO uint32_t TSDR;        /*!< RTC time stamp date register,                              Address offset: 0x34 */
  __IO uint32_t TSSSR;       /*!< RTC time-stamp sub second register,                        Address offset: 0x38 */
       uint32_t RESERVED2;   /*!< Reserved                                                   Address offset: 0x1C */
  __IO uint32_t ALRMAR;      /*!< RTC alarm A register,                                      Address offset: 0x40 */
  __IO uint32_t ALRMASSR;    /*!< RTC alarm A sub second register,                           Address offset: 0x44 */
  __IO uint32_t ALRMBR;      /*!< RTC alarm B register,                                      Address offset: 0x48 */
  __IO uint32_t ALRMBSSR;    /*!< RTC alarm B sub second register,                           Address offset: 0x4C */
  __IO uint32_t SR;          /*!< RTC Status register,                                       Address offset: 0x50 */
  __IO uint32_t MISR;        /*!< RTC Masked Interrupt Status register,                      Address offset: 0x54 */
       uint32_t RESERVED3;   /*!< Reserved                                                   Address offset: 0x58 */
  __IO uint32_t SCR;         /*!< RTC Status Clear register,                                 Address offset: 0x5C */
  __IO uint32_t OR;          /*!< RTC option register,                                       Address offset: 0x60 */
} RTC_TypeDef;

/**
  * @brief Tamper and backup registers
  */
typedef struct
{
  __IO uint32_t CR1;            /*!< TAMP configuration register 1,                             Address offset: 0x00 */
  __IO uint32_t CR2;            /*!< TAMP configuration register 2,                             Address offset: 0x04 */
       uint32_t RESERVED0;      /*!< Reserved                                                   Address offset: 0x08 */
  __IO uint32_t FLTCR;          /*!< Reserved                                                   Address offset: 0x0C */
       uint32_t RESERVED1[7];   /*!< Reserved                                                   Address offset: 0x10 -- 0x28 */
  __IO uint32_t IER;            /*!< TAMP Interrupt enable register,                            Address offset: 0x2C */
  __IO uint32_t SR;             /*!< TAMP Status register,                                      Address offset: 0x30 */
  __IO uint32_t MISR;           /*!< TAMP Masked Interrupt Status register,                     Address offset: 0x34 */
       uint32_t RESERVED2;      /*!< Reserved                                                   Address offset: 0x38 */
  __IO uint32_t SCR;            /*!< TAMP Status clear register,                                Address offset: 0x3C */
       uint32_t RESERVED3[48];  /*!< Reserved                                                   Address offset: 0x54 -- 0xFC */
  __IO uint32_t BKP0R;          /*!< TAMP backup register 0,                                    Address offset: 0x100 */
  __IO uint32_t BKP1R;          /*!< TAMP backup register 1,                                    Address offset: 0x104 */
  __IO uint32_t BKP2R;          /*!< TAMP backup register 2,                                    Address offset: 0x108 */
  __IO uint32_t BKP3R;          /*!< TAMP backup register 3,                                    Address offset: 0x10C */
  __IO uint32_t BKP4R;          /*!< TAMP backup register 4,                                    Address offset: 0x110 */
} TAMP_TypeDef;

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
  * @brief System configuration controller
  */
typedef struct
{
  __IO uint32_t CFGR1;          /*!< SYSCFG configuration register 1,                   Address offset: 0x00 */
       uint32_t RESERVED0[5];   /*!< Reserved,                                                   0x04 --0x14 */
  __IO uint32_t CFGR2;          /*!< SYSCFG configuration register 2,                   Address offset: 0x18 */
       uint32_t RESERVED1[25];  /*!< Reserved                                                           0x1C */
  __IO uint32_t IT_LINE_SR[32]; /*!< SYSCFG configuration IT_LINE register,             Address offset: 0x80 */
} SYSCFG_TypeDef;

/**
  * @brief TIM
  */
typedef struct
{
  __IO uint32_t CR1;         /*!< TIM control register 1,                   Address offset: 0x00 */
  __IO uint32_t CR2;         /*!< TIM control register 2,                   Address offset: 0x04 */
  __IO uint32_t SMCR;        /*!< TIM slave mode control register,          Address offset: 0x08 */
  __IO uint32_t DIER;        /*!< TIM DMA/interrupt enable register,        Address offset: 0x0C */
  __IO uint32_t SR;          /*!< TIM status register,                      Address offset: 0x10 */
  __IO uint32_t EGR;         /*!< TIM event generation register,            Address offset: 0x14 */
  __IO uint32_t CCMR1;       /*!< TIM capture/compare mode register 1,      Address offset: 0x18 */
  __IO uint32_t CCMR2;       /*!< TIM capture/compare mode register 2,      Address offset: 0x1C */
  __IO uint32_t CCER;        /*!< TIM capture/compare enable register,      Address offset: 0x20 */
  __IO uint32_t CNT;         /*!< TIM counter register,                     Address offset: 0x24 */
  __IO uint32_t PSC;         /*!< TIM prescaler register,                   Address offset: 0x28 */
  __IO uint32_t ARR;         /*!< TIM auto-reload register,                 Address offset: 0x2C */
  __IO uint32_t RCR;         /*!< TIM repetition counter register,          Address offset: 0x30 */
  __IO uint32_t CCR1;        /*!< TIM capture/compare register 1,           Address offset: 0x34 */
  __IO uint32_t CCR2;        /*!< TIM capture/compare register 2,           Address offset: 0x38 */
  __IO uint32_t CCR3;        /*!< TIM capture/compare register 3,           Address offset: 0x3C */
  __IO uint32_t CCR4;        /*!< TIM capture/compare register 4,           Address offset: 0x40 */
  __IO uint32_t BDTR;        /*!< TIM break and dead-time register,         Address offset: 0x44 */
  __IO uint32_t DCR;         /*!< TIM DMA control register,                 Address offset: 0x48 */
  __IO uint32_t DMAR;        /*!< TIM DMA address for full transfer,        Address offset: 0x4C */
  __IO uint32_t OR1;         /*!< TIM option register,                      Address offset: 0x50 */
  __IO uint32_t CCMR3;       /*!< TIM capture/compare mode register 3,      Address offset: 0x54 */
  __IO uint32_t CCR5;        /*!< TIM capture/compare register5,            Address offset: 0x58 */
  __IO uint32_t CCR6;        /*!< TIM capture/compare register6,            Address offset: 0x5C */
  __IO uint32_t AF1;         /*!< TIM alternate function register 1,        Address offset: 0x60 */
  __IO uint32_t AF2;         /*!< TIM alternate function register 2,        Address offset: 0x64 */
  __IO uint32_t TISEL;       /*!< TIM Input Selection register,             Address offset: 0x68 */
} TIM_TypeDef;

/**
  * @brief Universal Synchronous Asynchronous Receiver Transmitter
  */
typedef struct
{
  __IO uint32_t CR1;         /*!< USART Control register 1,                 Address offset: 0x00  */
  __IO uint32_t CR2;         /*!< USART Control register 2,                 Address offset: 0x04  */
  __IO uint32_t CR3;         /*!< USART Control register 3,                 Address offset: 0x08  */
  __IO uint32_t BRR;         /*!< USART Baud rate register,                 Address offset: 0x0C  */
  __IO uint32_t GTPR;        /*!< USART Guard time and prescaler register,  Address offset: 0x10  */
  __IO uint32_t RTOR;        /*!< USART Receiver Time Out register,         Address offset: 0x14  */
  __IO uint32_t RQR;         /*!< USART Request register,                   Address offset: 0x18  */
  __IO uint32_t ISR;         /*!< USART Interrupt and status register,      Address offset: 0x1C  */
  __IO uint32_t ICR;         /*!< USART Interrupt flag Clear register,      Address offset: 0x20  */
  __IO uint32_t RDR;         /*!< USART Receive Data register,              Address offset: 0x24  */
  __IO uint32_t TDR;         /*!< USART Transmit Data register,             Address offset: 0x28  */
  __IO uint32_t PRESC;       /*!< USART Prescaler register,                 Address offset: 0x2C  */
} USART_TypeDef;

/**
  * @brief VREFBUF
  */
typedef struct
{
  __IO uint32_t CSR;         /*!< VREFBUF control and status register,         Address offset: 0x00 */
  __IO uint32_t CCR;         /*!< VREFBUF calibration and control register,    Address offset: 0x04 */
} VREFBUF_TypeDef;

/**
  * @brief Window WATCHDOG
  */
typedef struct
{
  __IO uint32_t CR;          /*!< WWDG Control register,       Address offset: 0x00 */
  __IO uint32_t CFR;         /*!< WWDG Configuration register, Address offset: 0x04 */
  __IO uint32_t SR;          /*!< WWDG Status register,        Address offset: 0x08 */
} WWDG_TypeDef;

/**
  * @brief AES hardware accelerator
  */
typedef struct
{
  __IO uint32_t CR;          /*!< AES control register,                        Address offset: 0x00 */
  __IO uint32_t SR;          /*!< AES status register,                         Address offset: 0x04 */
  __IO uint32_t DINR;        /*!< AES data input register,                     Address offset: 0x08 */
  __IO uint32_t DOUTR;       /*!< AES data output register,                    Address offset: 0x0C */
  __IO uint32_t KEYR0;       /*!< AES key register 0,                          Address offset: 0x10 */
  __IO uint32_t KEYR1;       /*!< AES key register 1,                          Address offset: 0x14 */
  __IO uint32_t KEYR2;       /*!< AES key register 2,                          Address offset: 0x18 */
  __IO uint32_t KEYR3;       /*!< AES key register 3,                          Address offset: 0x1C */
  __IO uint32_t IVR0;        /*!< AES initialization vector register 0,        Address offset: 0x20 */
  __IO uint32_t IVR1;        /*!< AES initialization vector register 1,        Address offset: 0x24 */
  __IO uint32_t IVR2;        /*!< AES initialization vector register 2,        Address offset: 0x28 */
  __IO uint32_t IVR3;        /*!< AES initialization vector register 3,        Address offset: 0x2C */
  __IO uint32_t KEYR4;       /*!< AES key register 4,                          Address offset: 0x30 */
  __IO uint32_t KEYR5;       /*!< AES key register 5,                          Address offset: 0x34 */
  __IO uint32_t KEYR6;       /*!< AES key register 6,                          Address offset: 0x38 */
  __IO uint32_t KEYR7;       /*!< AES key register 7,                          Address offset: 0x3C */
  __IO uint32_t SUSP0R;      /*!< AES Suspend register 0,                      Address offset: 0x40 */
  __IO uint32_t SUSP1R;      /*!< AES Suspend register 1,                      Address offset: 0x44 */
  __IO uint32_t SUSP2R;      /*!< AES Suspend register 2,                      Address offset: 0x48 */
  __IO uint32_t SUSP3R;      /*!< AES Suspend register 3,                      Address offset: 0x4C */
  __IO uint32_t SUSP4R;      /*!< AES Suspend register 4,                      Address offset: 0x50 */
  __IO uint32_t SUSP5R;      /*!< AES Suspend register 5,                      Address offset: 0x54 */
  __IO uint32_t SUSP6R;      /*!< AES Suspend register 6,                      Address offset: 0x58 */
  __IO uint32_t SUSP7R;      /*!< AES Suspend register 7,                      Address offset: 0x5C */
} AES_TypeDef;

/**
  * @brief RNG
  */
typedef struct
{
  __IO uint32_t CR;  /*!< RNG control register, Address offset: 0x00 */
  __IO uint32_t SR;  /*!< RNG status register,  Address offset: 0x04 */
  __IO uint32_t DR;  /*!< RNG data register,    Address offset: 0x08 */
} RNG_TypeDef;


/**
  * @brief UCPD
  */
typedef struct
{
  __IO uint32_t CFG1;          /*!< UCPD configuration register 1,             Address offset: 0x00 */
  __IO uint32_t CFG2;          /*!< UCPD configuration register 2,             Address offset: 0x04 */
  __IO uint32_t RESERVED0;     /*!< UCPD reserved register,                    Address offset: 0x08 */
  __IO uint32_t CR;            /*!< UCPD control register,                     Address offset: 0x0C */
  __IO uint32_t IMR;           /*!< UCPD interrupt mask register,              Address offset: 0x10 */
  __IO uint32_t SR;            /*!< UCPD status register,                      Address offset: 0x14 */
  __IO uint32_t ICR;           /*!< UCPD interrupt flag clear register         Address offset: 0x18 */
  __IO uint32_t TX_ORDSET;     /*!< UCPD Tx ordered set type register,         Address offset: 0x1C */
  __IO uint32_t TX_PAYSZ;      /*!< UCPD Tx payload size register,             Address offset: 0x20 */
  __IO uint32_t TXDR;          /*!< UCPD Tx data register,                     Address offset: 0x24 */
  __IO uint32_t RX_ORDSET;     /*!< UCPD Rx ordered set type register,         Address offset: 0x28 */
  __IO uint32_t RX_PAYSZ;      /*!< UCPD Rx payload size register,             Address offset: 0x2C */
  __IO uint32_t RXDR;          /*!< UCPD Rx data register,                     Address offset: 0x30 */
  __IO uint32_t RX_ORDEXT1;    /*!< UCPD Rx ordered set extension 1 register,  Address offset: 0x34 */
  __IO uint32_t RX_ORDEXT2;    /*!< UCPD Rx ordered set extension 2 register,  Address offset: 0x38 */

} UCPD_TypeDef;

/**
  * @}
  */

/** @addtogroup Peripheral_memory_map
  * @{
  */
#define FLASH_BASE            (0x08000000UL)  /*!< FLASH base address */
#define SRAM_BASE             (0x20000000UL)  /*!< SRAM base address */
#define PERIPH_BASE           (0x40000000UL)  /*!< Peripheral base address */
#define IOPORT_BASE           (0x50000000UL)  /*!< IOPORT base address */

#define SRAM_SIZE_MAX         (0x00008000UL)  /*!< maximum SRAM size (up to 32 KBytes) */

/*!< Peripheral memory map */
#define APBPERIPH_BASE        (PERIPH_BASE)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000UL)

/*!< APB peripherals */

#define TIM2_BASE             (APBPERIPH_BASE + 0UL)
#define TIM3_BASE             (APBPERIPH_BASE + 0x00000400UL)
#define TIM6_BASE             (APBPERIPH_BASE + 0x00001000UL)
#define TIM7_BASE             (APBPERIPH_BASE + 0x00001400UL)
#define TIM14_BASE            (APBPERIPH_BASE + 0x00002000UL)
#define RTC_BASE              (APBPERIPH_BASE + 0x00002800UL)
#define WWDG_BASE             (APBPERIPH_BASE + 0x00002C00UL)
#define IWDG_BASE             (APBPERIPH_BASE + 0x00003000UL)
#define SPI2_BASE             (APBPERIPH_BASE + 0x00003800UL)
#define USART2_BASE           (APBPERIPH_BASE + 0x00004400UL)
#define USART3_BASE           (APBPERIPH_BASE + 0x00004800UL)
#define USART4_BASE           (APBPERIPH_BASE + 0x00004C00UL)
#define I2C1_BASE             (APBPERIPH_BASE + 0x00005400UL)
#define I2C2_BASE             (APBPERIPH_BASE + 0x00005800UL)
#define PWR_BASE              (APBPERIPH_BASE + 0x00007000UL)
#define DAC1_BASE             (APBPERIPH_BASE + 0x00007400UL)
#define DAC_BASE              (APBPERIPH_BASE + 0x00007400UL) /* Kept for legacy purpose */
#define CEC_BASE              (APBPERIPH_BASE + 0x00007800UL)
#define LPTIM1_BASE           (APBPERIPH_BASE + 0x00007C00UL)
#define LPUART1_BASE          (APBPERIPH_BASE + 0x00008000UL)
#define LPTIM2_BASE           (APBPERIPH_BASE + 0x00009400UL)
#define UCPD1_BASE            (APBPERIPH_BASE + 0x0000A000UL)
#define UCPD2_BASE            (APBPERIPH_BASE + 0x0000A400UL)
#define TAMP_BASE             (APBPERIPH_BASE + 0x0000B000UL)
#define SYSCFG_BASE           (APBPERIPH_BASE + 0x00010000UL)
#define VREFBUF_BASE          (APBPERIPH_BASE + 0x00010030UL)
#define COMP1_BASE            (SYSCFG_BASE + 0x0200UL)
#define COMP2_BASE            (SYSCFG_BASE + 0x0204UL)
#define ADC1_BASE             (APBPERIPH_BASE + 0x00012400UL)
#define ADC1_COMMON_BASE      (APBPERIPH_BASE + 0x00012708UL)
#define ADC_BASE              (ADC1_COMMON_BASE) /* Kept for legacy purpose */
#define TIM1_BASE             (APBPERIPH_BASE + 0x00012C00UL)
#define SPI1_BASE             (APBPERIPH_BASE + 0x00013000UL)
#define USART1_BASE           (APBPERIPH_BASE + 0x00013800UL)
#define TIM15_BASE            (APBPERIPH_BASE + 0x00014000UL)
#define TIM16_BASE            (APBPERIPH_BASE + 0x00014400UL)
#define TIM17_BASE            (APBPERIPH_BASE + 0x00014800UL)
#define DBG_BASE              (APBPERIPH_BASE + 0x00015800UL)


/*!< AHB peripherals */
#define DMA1_BASE             (AHBPERIPH_BASE)
#define DMAMUX1_BASE          (AHBPERIPH_BASE + 0x00000800UL)
#define RCC_BASE              (AHBPERIPH_BASE + 0x00001000UL)
#define EXTI_BASE             (AHBPERIPH_BASE + 0x00001800UL)
#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x00002000UL)
#define CRC_BASE              (AHBPERIPH_BASE + 0x00003000UL)

#define RNG_BASE              (AHBPERIPH_BASE + 0x00005000UL)
#define AES_BASE              (AHBPERIPH_BASE + 0x00006000UL)

#define DMA1_Channel1_BASE    (DMA1_BASE + 0x00000008UL)
#define DMA1_Channel2_BASE    (DMA1_BASE + 0x0000001CUL)
#define DMA1_Channel3_BASE    (DMA1_BASE + 0x00000030UL)
#define DMA1_Channel4_BASE    (DMA1_BASE + 0x00000044UL)
#define DMA1_Channel5_BASE    (DMA1_BASE + 0x00000058UL)
#define DMA1_Channel6_BASE    (DMA1_BASE + 0x0000006CUL)
#define DMA1_Channel7_BASE    (DMA1_BASE + 0x00000080UL)

#define DMAMUX1_Channel0_BASE    (DMAMUX1_BASE)
#define DMAMUX1_Channel1_BASE    (DMAMUX1_BASE + 0x00000004UL)
#define DMAMUX1_Channel2_BASE    (DMAMUX1_BASE + 0x00000008UL)
#define DMAMUX1_Channel3_BASE    (DMAMUX1_BASE + 0x0000000CUL)
#define DMAMUX1_Channel4_BASE    (DMAMUX1_BASE + 0x00000010UL)
#define DMAMUX1_Channel5_BASE    (DMAMUX1_BASE + 0x00000014UL)
#define DMAMUX1_Channel6_BASE    (DMAMUX1_BASE + 0x00000018UL)

#define DMAMUX1_RequestGenerator0_BASE  (DMAMUX1_BASE + 0x00000100UL)
#define DMAMUX1_RequestGenerator1_BASE  (DMAMUX1_BASE + 0x00000104UL)
#define DMAMUX1_RequestGenerator2_BASE  (DMAMUX1_BASE + 0x00000108UL)
#define DMAMUX1_RequestGenerator3_BASE  (DMAMUX1_BASE + 0x0000010CUL)

#define DMAMUX1_ChannelStatus_BASE      (DMAMUX1_BASE + 0x00000080UL)
#define DMAMUX1_RequestGenStatus_BASE   (DMAMUX1_BASE + 0x00000140UL)
#define DMAMUX1_IdRegisters_BASE        (DMAMUX1_BASE + 0x000003EC)

/*!< IOPORT */
#define GPIOA_BASE            (IOPORT_BASE + 0x00000000UL)
#define GPIOB_BASE            (IOPORT_BASE + 0x00000400UL)
#define GPIOC_BASE            (IOPORT_BASE + 0x00000800UL)
#define GPIOD_BASE            (IOPORT_BASE + 0x00000C00UL)
#define GPIOF_BASE            (IOPORT_BASE + 0x00001400UL)

/*!< Device Electronic Signature */
#define PACKAGE_BASE          (0x1FFF7500UL)        /*!< Package data register base address     */
#define UID_BASE              (0x1FFF7590UL)        /*!< Unique device ID register base address */
#define FLASHSIZE_BASE        (0x1FFF75E0UL)        /*!< Flash size data register base address  */

/**
  * @}
  */

/** @addtogroup Peripheral_declaration
  * @{
  */
#define TIM2                ((TIM_TypeDef *) TIM2_BASE)
#define TIM3                ((TIM_TypeDef *) TIM3_BASE)
#define TIM6                ((TIM_TypeDef *) TIM6_BASE)
#define TIM7                ((TIM_TypeDef *) TIM7_BASE)
#define TIM14               ((TIM_TypeDef *) TIM14_BASE)
#define RTC                 ((RTC_TypeDef *) RTC_BASE)
#define TAMP                ((TAMP_TypeDef *) TAMP_BASE)
#define WWDG                ((WWDG_TypeDef *) WWDG_BASE)
#define IWDG                ((IWDG_TypeDef *) IWDG_BASE)
#define SPI2                ((SPI_TypeDef *) SPI2_BASE)
#define USART2              ((USART_TypeDef *) USART2_BASE)
#define USART3              ((USART_TypeDef *) USART3_BASE)
#define USART4              ((USART_TypeDef *) USART4_BASE)
#define I2C1                ((I2C_TypeDef *) I2C1_BASE)
#define I2C2                ((I2C_TypeDef *) I2C2_BASE)
#define LPTIM1              ((LPTIM_TypeDef *) LPTIM1_BASE)
#define PWR                 ((PWR_TypeDef *) PWR_BASE)
#define RCC                 ((RCC_TypeDef *) RCC_BASE)
#define EXTI                ((EXTI_TypeDef *) EXTI_BASE)
#define DAC1                ((DAC_TypeDef *) DAC1_BASE)
#define DAC                 ((DAC_TypeDef *) DAC_BASE) /* Kept for legacy purpose */
#define LPUART1             ((USART_TypeDef *) LPUART1_BASE)
#define LPTIM2              ((LPTIM_TypeDef *) LPTIM2_BASE)
#define CEC                 ((CEC_TypeDef *) CEC_BASE)
#define SYSCFG              ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define VREFBUF             ((VREFBUF_TypeDef *) VREFBUF_BASE)
#define COMP1               ((COMP_TypeDef *) COMP1_BASE)
#define COMP2               ((COMP_TypeDef *) COMP2_BASE)
#define COMP12_COMMON       ((COMP_Common_TypeDef *) COMP1_BASE)
#define TIM1                ((TIM_TypeDef *) TIM1_BASE)
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)
#define USART1              ((USART_TypeDef *) USART1_BASE)
#define TIM15               ((TIM_TypeDef *) TIM15_BASE)
#define TIM16               ((TIM_TypeDef *) TIM16_BASE)
#define TIM17               ((TIM_TypeDef *) TIM17_BASE)
#define DMA1                ((DMA_TypeDef *) DMA1_BASE)

#define FLASH               ((FLASH_TypeDef *) FLASH_R_BASE)
#define CRC                 ((CRC_TypeDef *) CRC_BASE)
#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOF               ((GPIO_TypeDef *) GPIOF_BASE)
#define ADC1                ((ADC_TypeDef *) ADC1_BASE)
#define ADC1_COMMON         ((ADC_Common_TypeDef *) ADC1_COMMON_BASE)
#define ADC                 (ADC1_COMMON) /* Kept for legacy purpose */

#define AES                 ((AES_TypeDef *) AES_BASE)
#define AES1                ((AES_TypeDef *) AES_BASE)
#define RNG                 ((RNG_TypeDef *) RNG_BASE)

#define UCPD1               ((UCPD_TypeDef *) UCPD1_BASE)
#define UCPD2               ((UCPD_TypeDef *) UCPD2_BASE)

#define DMA1_Channel1       ((DMA_Channel_TypeDef *) DMA1_Channel1_BASE)
#define DMA1_Channel2       ((DMA_Channel_TypeDef *) DMA1_Channel2_BASE)
#define DMA1_Channel3       ((DMA_Channel_TypeDef *) DMA1_Channel3_BASE)
#define DMA1_Channel4       ((DMA_Channel_TypeDef *) DMA1_Channel4_BASE)
#define DMA1_Channel5       ((DMA_Channel_TypeDef *) DMA1_Channel5_BASE)
#define DMA1_Channel6       ((DMA_Channel_TypeDef *) DMA1_Channel6_BASE)
#define DMA1_Channel7       ((DMA_Channel_TypeDef *) DMA1_Channel7_BASE)

#define DMAMUX1                ((DMAMUX_Channel_TypeDef *) DMAMUX1_BASE)
#define DMAMUX1_Channel0       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel0_BASE)
#define DMAMUX1_Channel1       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel1_BASE)
#define DMAMUX1_Channel2       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel2_BASE)
#define DMAMUX1_Channel3       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel3_BASE)
#define DMAMUX1_Channel4       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel4_BASE)
#define DMAMUX1_Channel5       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel5_BASE)
#define DMAMUX1_Channel6       ((DMAMUX_Channel_TypeDef *) DMAMUX1_Channel6_BASE)

#define DMAMUX1_RequestGenerator0  ((DMAMUX_RequestGen_TypeDef *) DMAMUX1_RequestGenerator0_BASE)
#define DMAMUX1_RequestGenerator1  ((DMAMUX_RequestGen_TypeDef *) DMAMUX1_RequestGenerator1_BASE)
#define DMAMUX1_RequestGenerator2  ((DMAMUX_RequestGen_TypeDef *) DMAMUX1_RequestGenerator2_BASE)
#define DMAMUX1_RequestGenerator3  ((DMAMUX_RequestGen_TypeDef *) DMAMUX1_RequestGenerator3_BASE)

#define DMAMUX1_ChannelStatus      ((DMAMUX_ChannelStatus_TypeDef *) DMAMUX1_ChannelStatus_BASE)
#define DMAMUX1_RequestGenStatus   ((DMAMUX_RequestGenStatus_TypeDef *) DMAMUX1_RequestGenStatus_BASE)
#define DMAMUX1_IdRegisters        ((DMAMUX_IdRegisters_TypeDef *) DMAMUX1_IdRegisters_BASE)

#define DBG              ((DBG_TypeDef *) DBG_BASE)

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
/********************  Bit definition for ADC_ISR register  *******************/
#define ADC_ISR_ADRDY_Pos              (0U)
#define ADC_ISR_ADRDY_Msk              (0x1UL << ADC_ISR_ADRDY_Pos)            /*!< 0x00000001 */
#define ADC_ISR_ADRDY                  ADC_ISR_ADRDY_Msk                       /*!< ADC ready flag */
#define ADC_ISR_EOSMP_Pos              (1U)
#define ADC_ISR_EOSMP_Msk              (0x1UL << ADC_ISR_EOSMP_Pos)            /*!< 0x00000002 */
#define ADC_ISR_EOSMP                  ADC_ISR_EOSMP_Msk                       /*!< ADC group regular end of sampling flag */
#define ADC_ISR_EOC_Pos                (2U)
#define ADC_ISR_EOC_Msk                (0x1UL << ADC_ISR_EOC_Pos)              /*!< 0x00000004 */
#define ADC_ISR_EOC                    ADC_ISR_EOC_Msk                         /*!< ADC group regular end of unitary conversion flag */
#define ADC_ISR_EOS_Pos                (3U)
#define ADC_ISR_EOS_Msk                (0x1UL << ADC_ISR_EOS_Pos)              /*!< 0x00000008 */
#define ADC_ISR_EOS                    ADC_ISR_EOS_Msk                         /*!< ADC group regular end of sequence conversions flag */
#define ADC_ISR_OVR_Pos                (4U)
#define ADC_ISR_OVR_Msk                (0x1UL << ADC_ISR_OVR_Pos)              /*!< 0x00000010 */
#define ADC_ISR_OVR                    ADC_ISR_OVR_Msk                         /*!< ADC group regular overrun flag */
#define ADC_ISR_AWD1_Pos               (7U)
#define ADC_ISR_AWD1_Msk               (0x1UL << ADC_ISR_AWD1_Pos)             /*!< 0x00000080 */
#define ADC_ISR_AWD1                   ADC_ISR_AWD1_Msk                        /*!< ADC analog watchdog 1 flag */
#define ADC_ISR_AWD2_Pos               (8U)
#define ADC_ISR_AWD2_Msk               (0x1UL << ADC_ISR_AWD2_Pos)             /*!< 0x00000100 */
#define ADC_ISR_AWD2                   ADC_ISR_AWD2_Msk                        /*!< ADC analog watchdog 2 flag */
#define ADC_ISR_AWD3_Pos               (9U)
#define ADC_ISR_AWD3_Msk               (0x1UL << ADC_ISR_AWD3_Pos)             /*!< 0x00000200 */
#define ADC_ISR_AWD3                   ADC_ISR_AWD3_Msk                        /*!< ADC analog watchdog 3 flag */
#define ADC_ISR_EOCAL_Pos              (11U)
#define ADC_ISR_EOCAL_Msk              (0x1UL << ADC_ISR_EOCAL_Pos)            /*!< 0x00000800 */
#define ADC_ISR_EOCAL                  ADC_ISR_EOCAL_Msk                       /*!< ADC end of calibration flag */
#define ADC_ISR_CCRDY_Pos              (13U)
#define ADC_ISR_CCRDY_Msk              (0x1UL << ADC_ISR_CCRDY_Pos)            /*!< 0x00002000 */
#define ADC_ISR_CCRDY                  ADC_ISR_CCRDY_Msk                       /*!< ADC channel configuration ready flag */

/* Legacy defines */
#define ADC_ISR_EOSEQ           (ADC_ISR_EOS)

/********************  Bit definition for ADC_IER register  *******************/
#define ADC_IER_ADRDYIE_Pos            (0U)
#define ADC_IER_ADRDYIE_Msk            (0x1UL << ADC_IER_ADRDYIE_Pos)          /*!< 0x00000001 */
#define ADC_IER_ADRDYIE                ADC_IER_ADRDYIE_Msk                     /*!< ADC ready interrupt */
#define ADC_IER_EOSMPIE_Pos            (1U)
#define ADC_IER_EOSMPIE_Msk            (0x1UL << ADC_IER_EOSMPIE_Pos)          /*!< 0x00000002 */
#define ADC_IER_EOSMPIE                ADC_IER_EOSMPIE_Msk                     /*!< ADC group regular end of sampling interrupt */
#define ADC_IER_EOCIE_Pos              (2U)
#define ADC_IER_EOCIE_Msk              (0x1UL << ADC_IER_EOCIE_Pos)            /*!< 0x00000004 */
#define ADC_IER_EOCIE                  ADC_IER_EOCIE_Msk                       /*!< ADC group regular end of unitary conversion interrupt */
#define ADC_IER_EOSIE_Pos              (3U)
#define ADC_IER_EOSIE_Msk              (0x1UL << ADC_IER_EOSIE_Pos)            /*!< 0x00000008 */
#define ADC_IER_EOSIE                  ADC_IER_EOSIE_Msk                       /*!< ADC group regular end of sequence conversions interrupt */
#define ADC_IER_OVRIE_Pos              (4U)
#define ADC_IER_OVRIE_Msk              (0x1UL << ADC_IER_OVRIE_Pos)            /*!< 0x00000010 */
#define ADC_IER_OVRIE                  ADC_IER_OVRIE_Msk                       /*!< ADC group regular overrun interrupt */
#define ADC_IER_AWD1IE_Pos             (7U)
#define ADC_IER_AWD1IE_Msk             (0x1UL << ADC_IER_AWD1IE_Pos)           /*!< 0x00000080 */
#define ADC_IER_AWD1IE                 ADC_IER_AWD1IE_Msk                      /*!< ADC analog watchdog 1 interrupt */
#define ADC_IER_AWD2IE_Pos             (8U)
#define ADC_IER_AWD2IE_Msk             (0x1UL << ADC_IER_AWD2IE_Pos)           /*!< 0x00000100 */
#define ADC_IER_AWD2IE                 ADC_IER_AWD2IE_Msk                      /*!< ADC analog watchdog 2 interrupt */
#define ADC_IER_AWD3IE_Pos             (9U)
#define ADC_IER_AWD3IE_Msk             (0x1UL << ADC_IER_AWD3IE_Pos)           /*!< 0x00000200 */
#define ADC_IER_AWD3IE                 ADC_IER_AWD3IE_Msk                      /*!< ADC analog watchdog 3 interrupt */
#define ADC_IER_EOCALIE_Pos            (11U)
#define ADC_IER_EOCALIE_Msk            (0x1UL << ADC_IER_EOCALIE_Pos)          /*!< 0x00000800 */
#define ADC_IER_EOCALIE                ADC_IER_EOCALIE_Msk                     /*!< ADC end of calibration interrupt */
#define ADC_IER_CCRDYIE_Pos            (13U)
#define ADC_IER_CCRDYIE_Msk            (0x1UL << ADC_IER_CCRDYIE_Pos)          /*!< 0x00002000 */
#define ADC_IER_CCRDYIE                ADC_IER_CCRDYIE_Msk                     /*!< ADC channel configuration ready interrupt */

/* Legacy defines */
#define ADC_IER_EOSEQIE           (ADC_IER_EOSIE)

/********************  Bit definition for ADC_CR register  ********************/
#define ADC_CR_ADEN_Pos                (0U)
#define ADC_CR_ADEN_Msk                (0x1UL << ADC_CR_ADEN_Pos)              /*!< 0x00000001 */
#define ADC_CR_ADEN                    ADC_CR_ADEN_Msk                         /*!< ADC enable */
#define ADC_CR_ADDIS_Pos               (1U)
#define ADC_CR_ADDIS_Msk               (0x1UL << ADC_CR_ADDIS_Pos)             /*!< 0x00000002 */
#define ADC_CR_ADDIS                   ADC_CR_ADDIS_Msk                        /*!< ADC disable */
#define ADC_CR_ADSTART_Pos             (2U)
#define ADC_CR_ADSTART_Msk             (0x1UL << ADC_CR_ADSTART_Pos)           /*!< 0x00000004 */
#define ADC_CR_ADSTART                 ADC_CR_ADSTART_Msk                      /*!< ADC group regular conversion start */
#define ADC_CR_ADSTP_Pos               (4U)
#define ADC_CR_ADSTP_Msk               (0x1UL << ADC_CR_ADSTP_Pos)             /*!< 0x00000010 */
#define ADC_CR_ADSTP                   ADC_CR_ADSTP_Msk                        /*!< ADC group regular conversion stop */
#define ADC_CR_ADVREGEN_Pos            (28U)
#define ADC_CR_ADVREGEN_Msk            (0x1UL << ADC_CR_ADVREGEN_Pos)          /*!< 0x10000000 */
#define ADC_CR_ADVREGEN                ADC_CR_ADVREGEN_Msk                     /*!< ADC voltage regulator enable */
#define ADC_CR_ADCAL_Pos               (31U)
#define ADC_CR_ADCAL_Msk               (0x1UL << ADC_CR_ADCAL_Pos)             /*!< 0x80000000 */
#define ADC_CR_ADCAL                   ADC_CR_ADCAL_Msk                        /*!< ADC calibration */

/********************  Bit definition for ADC_CFGR1 register  *****************/
#define ADC_CFGR1_DMAEN_Pos            (0U)
#define ADC_CFGR1_DMAEN_Msk            (0x1UL << ADC_CFGR1_DMAEN_Pos)          /*!< 0x00000001 */
#define ADC_CFGR1_DMAEN                ADC_CFGR1_DMAEN_Msk                     /*!< ADC DMA transfer enable */
#define ADC_CFGR1_DMACFG_Pos           (1U)
#define ADC_CFGR1_DMACFG_Msk           (0x1UL << ADC_CFGR1_DMACFG_Pos)         /*!< 0x00000002 */
#define ADC_CFGR1_DMACFG               ADC_CFGR1_DMACFG_Msk                    /*!< ADC DMA transfer configuration */

#define ADC_CFGR1_SCANDIR_Pos          (2U)
#define ADC_CFGR1_SCANDIR_Msk          (0x1UL << ADC_CFGR1_SCANDIR_Pos)        /*!< 0x00000004 */
#define ADC_CFGR1_SCANDIR              ADC_CFGR1_SCANDIR_Msk                   /*!< ADC group regular sequencer scan direction */

#define ADC_CFGR1_RES_Pos              (3U)
#define ADC_CFGR1_RES_Msk              (0x3UL << ADC_CFGR1_RES_Pos)            /*!< 0x00000018 */
#define ADC_CFGR1_RES                  ADC_CFGR1_RES_Msk                       /*!< ADC data resolution */
#define ADC_CFGR1_RES_0                (0x1U << ADC_CFGR1_RES_Pos)             /*!< 0x00000008 */
#define ADC_CFGR1_RES_1                (0x2U << ADC_CFGR1_RES_Pos)             /*!< 0x00000010 */

#define ADC_CFGR1_ALIGN_Pos            (5U)
#define ADC_CFGR1_ALIGN_Msk            (0x1UL << ADC_CFGR1_ALIGN_Pos)          /*!< 0x00000020 */
#define ADC_CFGR1_ALIGN                ADC_CFGR1_ALIGN_Msk                     /*!< ADC data alignement */

#define ADC_CFGR1_EXTSEL_Pos           (6U)
#define ADC_CFGR1_EXTSEL_Msk           (0x7UL << ADC_CFGR1_EXTSEL_Pos)         /*!< 0x000001C0 */
#define ADC_CFGR1_EXTSEL               ADC_CFGR1_EXTSEL_Msk                    /*!< ADC group regular external trigger source */
#define ADC_CFGR1_EXTSEL_0             (0x1UL << ADC_CFGR1_EXTSEL_Pos)         /*!< 0x00000040 */
#define ADC_CFGR1_EXTSEL_1             (0x2UL << ADC_CFGR1_EXTSEL_Pos)         /*!< 0x00000080 */
#define ADC_CFGR1_EXTSEL_2             (0x4UL << ADC_CFGR1_EXTSEL_Pos)         /*!< 0x00000100 */

#define ADC_CFGR1_EXTEN_Pos            (10U)
#define ADC_CFGR1_EXTEN_Msk            (0x3UL << ADC_CFGR1_EXTEN_Pos)          /*!< 0x00000C00 */
#define ADC_CFGR1_EXTEN                ADC_CFGR1_EXTEN_Msk                     /*!< ADC group regular external trigger polarity */
#define ADC_CFGR1_EXTEN_0              (0x1UL << ADC_CFGR1_EXTEN_Pos)          /*!< 0x00000400 */
#define ADC_CFGR1_EXTEN_1              (0x2UL << ADC_CFGR1_EXTEN_Pos)          /*!< 0x00000800 */

#define ADC_CFGR1_OVRMOD_Pos           (12U)
#define ADC_CFGR1_OVRMOD_Msk           (0x1UL << ADC_CFGR1_OVRMOD_Pos)         /*!< 0x00001000 */
#define ADC_CFGR1_OVRMOD               ADC_CFGR1_OVRMOD_Msk                    /*!< ADC group regular overrun configuration */
#define ADC_CFGR1_CONT_Pos             (13U)
#define ADC_CFGR1_CONT_Msk             (0x1UL << ADC_CFGR1_CONT_Pos)           /*!< 0x00002000 */
#define ADC_CFGR1_CONT                 ADC_CFGR1_CONT_Msk                      /*!< ADC group regular continuous conversion mode */
#define ADC_CFGR1_WAIT_Pos             (14U)
#define ADC_CFGR1_WAIT_Msk             (0x1UL << ADC_CFGR1_WAIT_Pos)           /*!< 0x00004000 */
#define ADC_CFGR1_WAIT                 ADC_CFGR1_WAIT_Msk                      /*!< ADC low power auto wait */
#define ADC_CFGR1_AUTOFF_Pos           (15U)
#define ADC_CFGR1_AUTOFF_Msk           (0x1UL << ADC_CFGR1_AUTOFF_Pos)         /*!< 0x00008000 */
#define ADC_CFGR1_AUTOFF               ADC_CFGR1_AUTOFF_Msk                    /*!< ADC low power auto power off */
#define ADC_CFGR1_DISCEN_Pos           (16U)
#define ADC_CFGR1_DISCEN_Msk           (0x1UL << ADC_CFGR1_DISCEN_Pos)         /*!< 0x00010000 */
#define ADC_CFGR1_DISCEN               ADC_CFGR1_DISCEN_Msk                    /*!< ADC group regular sequencer discontinuous mode */
#define ADC_CFGR1_CHSELRMOD_Pos        (21U)
#define ADC_CFGR1_CHSELRMOD_Msk        (0x1UL << ADC_CFGR1_CHSELRMOD_Pos)      /*!< 0x00200000 */
#define ADC_CFGR1_CHSELRMOD            ADC_CFGR1_CHSELRMOD_Msk                 /*!< ADC group regular sequencer mode */

#define ADC_CFGR1_AWD1SGL_Pos          (22U)
#define ADC_CFGR1_AWD1SGL_Msk          (0x1UL << ADC_CFGR1_AWD1SGL_Pos)        /*!< 0x00400000 */
#define ADC_CFGR1_AWD1SGL              ADC_CFGR1_AWD1SGL_Msk                   /*!< ADC analog watchdog 1 monitoring a single channel or all channels */
#define ADC_CFGR1_AWD1EN_Pos           (23U)
#define ADC_CFGR1_AWD1EN_Msk           (0x1UL << ADC_CFGR1_AWD1EN_Pos)         /*!< 0x00800000 */
#define ADC_CFGR1_AWD1EN               ADC_CFGR1_AWD1EN_Msk                    /*!< ADC analog watchdog 1 enable on scope ADC group regular */

#define ADC_CFGR1_AWD1CH_Pos           (26U)
#define ADC_CFGR1_AWD1CH_Msk           (0x1FUL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x7C000000 */
#define ADC_CFGR1_AWD1CH               ADC_CFGR1_AWD1CH_Msk                    /*!< ADC analog watchdog 1 monitored channel selection */
#define ADC_CFGR1_AWD1CH_0             (0x01UL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x04000000 */
#define ADC_CFGR1_AWD1CH_1             (0x02UL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x08000000 */
#define ADC_CFGR1_AWD1CH_2             (0x04UL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x10000000 */
#define ADC_CFGR1_AWD1CH_3             (0x08UL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x20000000 */
#define ADC_CFGR1_AWD1CH_4             (0x10UL << ADC_CFGR1_AWD1CH_Pos)        /*!< 0x40000000 */

/* Legacy defines */
#define ADC_CFGR1_AUTDLY          (ADC_CFGR1_WAIT)

/********************  Bit definition for ADC_CFGR2 register  *****************/
#define ADC_CFGR2_OVSE_Pos             (0U)
#define ADC_CFGR2_OVSE_Msk             (0x1UL << ADC_CFGR2_OVSE_Pos)           /*!< 0x00000001 */
#define ADC_CFGR2_OVSE                 ADC_CFGR2_OVSE_Msk                      /*!< ADC oversampler enable on scope ADC group regular */

#define ADC_CFGR2_OVSR_Pos             (2U)
#define ADC_CFGR2_OVSR_Msk             (0x7UL << ADC_CFGR2_OVSR_Pos)           /*!< 0x0000001C */
#define ADC_CFGR2_OVSR                 ADC_CFGR2_OVSR_Msk                      /*!< ADC oversampling ratio */
#define ADC_CFGR2_OVSR_0               (0x1UL << ADC_CFGR2_OVSR_Pos)           /*!< 0x00000004 */
#define ADC_CFGR2_OVSR_1               (0x2UL << ADC_CFGR2_OVSR_Pos)           /*!< 0x00000008 */
#define ADC_CFGR2_OVSR_2               (0x4UL << ADC_CFGR2_OVSR_Pos)           /*!< 0x00000010 */

#define ADC_CFGR2_OVSS_Pos             (5U)
#define ADC_CFGR2_OVSS_Msk             (0xFUL << ADC_CFGR2_OVSS_Pos)           /*!< 0x000001E0 */
#define ADC_CFGR2_OVSS                 ADC_CFGR2_OVSS_Msk                      /*!< ADC oversampling shift */
#define ADC_CFGR2_OVSS_0               (0x1UL << ADC_CFGR2_OVSS_Pos)           /*!< 0x00000020 */
#define ADC_CFGR2_OVSS_1               (0x2UL << ADC_CFGR2_OVSS_Pos)           /*!< 0x00000040 */
#define ADC_CFGR2_OVSS_2               (0x4UL << ADC_CFGR2_OVSS_Pos)           /*!< 0x00000080 */
#define ADC_CFGR2_OVSS_3               (0x8UL << ADC_CFGR2_OVSS_Pos)           /*!< 0x00000100 */

#define ADC_CFGR2_TOVS_Pos             (9U)
#define ADC_CFGR2_TOVS_Msk             (0x1UL << ADC_CFGR2_TOVS_Pos)           /*!< 0x00000200 */
#define ADC_CFGR2_TOVS                 ADC_CFGR2_TOVS_Msk                      /*!< ADC oversampling discontinuous mode (triggered mode) for ADC group regular */

#define ADC_CFGR2_LFTRIG_Pos           (29U)
#define ADC_CFGR2_LFTRIG_Msk           (0x1UL << ADC_CFGR2_LFTRIG_Pos)         /*!< 0x20000000 */
#define ADC_CFGR2_LFTRIG               ADC_CFGR2_LFTRIG_Msk                    /*!< ADC low frequency trigger mode */

#define ADC_CFGR2_CKMODE_Pos           (30U)
#define ADC_CFGR2_CKMODE_Msk           (0x3UL << ADC_CFGR2_CKMODE_Pos)         /*!< 0xC0000000 */
#define ADC_CFGR2_CKMODE               ADC_CFGR2_CKMODE_Msk                    /*!< ADC clock source and prescaler (prescaler only for clock source synchronous) */
#define ADC_CFGR2_CKMODE_1             (0x2UL << ADC_CFGR2_CKMODE_Pos)         /*!< 0x80000000 */
#define ADC_CFGR2_CKMODE_0             (0x1UL << ADC_CFGR2_CKMODE_Pos)         /*!< 0x40000000 */

/********************  Bit definition for ADC_SMPR register  ******************/
#define ADC_SMPR_SMP1_Pos              (0U)
#define ADC_SMPR_SMP1_Msk              (0x7UL << ADC_SMPR_SMP1_Pos)            /*!< 0x00000007 */
#define ADC_SMPR_SMP1                  ADC_SMPR_SMP1_Msk                       /*!< ADC group of channels sampling time 1 */
#define ADC_SMPR_SMP1_0                (0x1UL << ADC_SMPR_SMP1_Pos)            /*!< 0x00000001 */
#define ADC_SMPR_SMP1_1                (0x2UL << ADC_SMPR_SMP1_Pos)            /*!< 0x00000002 */
#define ADC_SMPR_SMP1_2                (0x4UL << ADC_SMPR_SMP1_Pos)            /*!< 0x00000004 */

#define ADC_SMPR_SMP2_Pos              (4U)
#define ADC_SMPR_SMP2_Msk              (0x7UL << ADC_SMPR_SMP2_Pos)            /*!< 0x00000070 */
#define ADC_SMPR_SMP2                  ADC_SMPR_SMP2_Msk                       /*!< ADC group of channels sampling time 2 */
#define ADC_SMPR_SMP2_0                (0x1UL << ADC_SMPR_SMP2_Pos)            /*!< 0x00000010 */
#define ADC_SMPR_SMP2_1                (0x2UL << ADC_SMPR_SMP2_Pos)            /*!< 0x00000020 */
#define ADC_SMPR_SMP2_2                (0x4UL << ADC_SMPR_SMP2_Pos)            /*!< 0x00000040 */

#define ADC_SMPR_SMPSEL_Pos            (8U)
#define ADC_SMPR_SMPSEL_Msk            (0x7FFFFUL << ADC_SMPR_SMPSEL_Pos)      /*!< 0x07FFFF00 */
#define ADC_SMPR_SMPSEL                ADC_SMPR_SMPSEL_Msk                     /*!< ADC all channels sampling time selection */
#define ADC_SMPR_SMPSEL0_Pos           (8U)
#define ADC_SMPR_SMPSEL0_Msk           (0x1UL << ADC_SMPR_SMPSEL0_Pos)         /*!< 0x00000100 */
#define ADC_SMPR_SMPSEL0               ADC_SMPR_SMPSEL0_Msk                    /*!< ADC channel 0 sampling time selection */
#define ADC_SMPR_SMPSEL1_Pos           (9U)
#define ADC_SMPR_SMPSEL1_Msk           (0x1UL << ADC_SMPR_SMPSEL1_Pos)         /*!< 0x00000200 */
#define ADC_SMPR_SMPSEL1               ADC_SMPR_SMPSEL1_Msk                    /*!< ADC channel 1 sampling time selection */
#define ADC_SMPR_SMPSEL2_Pos           (10U)
#define ADC_SMPR_SMPSEL2_Msk           (0x1UL << ADC_SMPR_SMPSEL2_Pos)         /*!< 0x00000400 */
#define ADC_SMPR_SMPSEL2               ADC_SMPR_SMPSEL2_Msk                    /*!< ADC channel 2 sampling time selection */
#define ADC_SMPR_SMPSEL3_Pos           (11U)
#define ADC_SMPR_SMPSEL3_Msk           (0x1UL << ADC_SMPR_SMPSEL3_Pos)         /*!< 0x00000800 */
#define ADC_SMPR_SMPSEL3               ADC_SMPR_SMPSEL3_Msk                    /*!< ADC channel 3 sampling time selection */
#define ADC_SMPR_SMPSEL4_Pos           (12U)
#define ADC_SMPR_SMPSEL4_Msk           (0x1UL << ADC_SMPR_SMPSEL4_Pos)         /*!< 0x00001000 */
#define ADC_SMPR_SMPSEL4               ADC_SMPR_SMPSEL4_Msk                    /*!< ADC channel 4 sampling time selection */
#define ADC_SMPR_SMPSEL5_Pos           (13U)
#define ADC_SMPR_SMPSEL5_Msk           (0x1UL << ADC_SMPR_SMPSEL5_Pos)         /*!< 0x00002000 */
#define ADC_SMPR_SMPSEL5               ADC_SMPR_SMPSEL5_Msk                    /*!< ADC channel 5 sampling time selection */
#define ADC_SMPR_SMPSEL6_Pos           (14U)
#define ADC_SMPR_SMPSEL6_Msk           (0x1UL << ADC_SMPR_SMPSEL6_Pos)         /*!< 0x00004000 */
#define ADC_SMPR_SMPSEL6               ADC_SMPR_SMPSEL6_Msk                    /*!< ADC channel 6 sampling time selection */
#define ADC_SMPR_SMPSEL7_Pos           (15U)
#define ADC_SMPR_SMPSEL7_Msk           (0x1UL << ADC_SMPR_SMPSEL7_Pos)         /*!< 0x00008000 */
#define ADC_SMPR_SMPSEL7               ADC_SMPR_SMPSEL7_Msk                    /*!< ADC channel 7 sampling time selection */
#define ADC_SMPR_SMPSEL8_Pos           (16U)
#define ADC_SMPR_SMPSEL8_Msk           (0x1UL << ADC_SMPR_SMPSEL8_Pos)         /*!< 0x00010000 */
#define ADC_SMPR_SMPSEL8               ADC_SMPR_SMPSEL8_Msk                    /*!< ADC channel 8 sampling time selection */
#define ADC_SMPR_SMPSEL9_Pos           (17U)
#define ADC_SMPR_SMPSEL9_Msk           (0x1UL << ADC_SMPR_SMPSEL9_Pos)         /*!< 0x00020000 */
#define ADC_SMPR_SMPSEL9               ADC_SMPR_SMPSEL9_Msk                    /*!< ADC channel 9 sampling time selection */
#define ADC_SMPR_SMPSEL10_Pos          (18U)
#define ADC_SMPR_SMPSEL10_Msk          (0x1UL << ADC_SMPR_SMPSEL10_Pos)        /*!< 0x00040000 */
#define ADC_SMPR_SMPSEL10              ADC_SMPR_SMPSEL10_Msk                   /*!< ADC channel 10 sampling time selection */
#define ADC_SMPR_SMPSEL11_Pos          (19U)
#define ADC_SMPR_SMPSEL11_Msk          (0x1UL << ADC_SMPR_SMPSEL11_Pos)        /*!< 0x00080000 */
#define ADC_SMPR_SMPSEL11              ADC_SMPR_SMPSEL11_Msk                   /*!< ADC channel 11 sampling time selection */
#define ADC_SMPR_SMPSEL12_Pos          (20U)
#define ADC_SMPR_SMPSEL12_Msk          (0x1UL << ADC_SMPR_SMPSEL12_Pos)        /*!< 0x00100000 */
#define ADC_SMPR_SMPSEL12              ADC_SMPR_SMPSEL12_Msk                   /*!< ADC channel 12 sampling time selection */
#define ADC_SMPR_SMPSEL13_Pos          (21U)
#define ADC_SMPR_SMPSEL13_Msk          (0x1UL << ADC_SMPR_SMPSEL13_Pos)        /*!< 0x00200000 */
#define ADC_SMPR_SMPSEL13              ADC_SMPR_SMPSEL13_Msk                   /*!< ADC channel 13 sampling time selection */
#define ADC_SMPR_SMPSEL14_Pos          (22U)
#define ADC_SMPR_SMPSEL14_Msk          (0x1UL << ADC_SMPR_SMPSEL14_Pos)        /*!< 0x00400000 */
#define ADC_SMPR_SMPSEL14              ADC_SMPR_SMPSEL14_Msk                   /*!< ADC channel 14 sampling time selection */
#define ADC_SMPR_SMPSEL15_Pos          (23U)
#define ADC_SMPR_SMPSEL15_Msk          (0x1UL << ADC_SMPR_SMPSEL15_Pos)        /*!< 0x00800000 */
#define ADC_SMPR_SMPSEL15              ADC_SMPR_SMPSEL15_Msk                   /*!< ADC channel 15 sampling time selection */
#define ADC_SMPR_SMPSEL16_Pos          (24U)
#define ADC_SMPR_SMPSEL16_Msk          (0x1UL << ADC_SMPR_SMPSEL16_Pos)        /*!< 0x01000000 */
#define ADC_SMPR_SMPSEL16              ADC_SMPR_SMPSEL16_Msk                   /*!< ADC channel 16 sampling time selection */
#define ADC_SMPR_SMPSEL17_Pos          (25U)
#define ADC_SMPR_SMPSEL17_Msk          (0x1UL << ADC_SMPR_SMPSEL17_Pos)        /*!< 0x02000000 */
#define ADC_SMPR_SMPSEL17              ADC_SMPR_SMPSEL17_Msk                   /*!< ADC channel 17 sampling time selection */
#define ADC_SMPR_SMPSEL18_Pos          (26U)
#define ADC_SMPR_SMPSEL18_Msk          (0x1UL << ADC_SMPR_SMPSEL18_Pos)        /*!< 0x04000000 */
#define ADC_SMPR_SMPSEL18              ADC_SMPR_SMPSEL18_Msk                   /*!< ADC channel 18 sampling time selection */

/********************  Bit definition for ADC_TR1 register  *******************/
#define ADC_TR1_LT1_Pos                (0U)
#define ADC_TR1_LT1_Msk                (0xFFFUL << ADC_TR1_LT1_Pos)            /*!< 0x00000FFF */
#define ADC_TR1_LT1                    ADC_TR1_LT1_Msk                         /*!< ADC analog watchdog 1 threshold low */
#define ADC_TR1_LT1_0                  (0x001UL << ADC_TR1_LT1_Pos)            /*!< 0x00000001 */
#define ADC_TR1_LT1_1                  (0x002UL << ADC_TR1_LT1_Pos)            /*!< 0x00000002 */
#define ADC_TR1_LT1_2                  (0x004UL << ADC_TR1_LT1_Pos)            /*!< 0x00000004 */
#define ADC_TR1_LT1_3                  (0x008UL << ADC_TR1_LT1_Pos)            /*!< 0x00000008 */
#define ADC_TR1_LT1_4                  (0x010UL << ADC_TR1_LT1_Pos)            /*!< 0x00000010 */
#define ADC_TR1_LT1_5                  (0x020UL << ADC_TR1_LT1_Pos)            /*!< 0x00000020 */
#define ADC_TR1_LT1_6                  (0x040UL << ADC_TR1_LT1_Pos)            /*!< 0x00000040 */
#define ADC_TR1_LT1_7                  (0x080UL << ADC_TR1_LT1_Pos)            /*!< 0x00000080 */
#define ADC_TR1_LT1_8                  (0x100UL << ADC_TR1_LT1_Pos)            /*!< 0x00000100 */
#define ADC_TR1_LT1_9                  (0x200UL << ADC_TR1_LT1_Pos)            /*!< 0x00000200 */
#define ADC_TR1_LT1_10                 (0x400UL << ADC_TR1_LT1_Pos)            /*!< 0x00000400 */
#define ADC_TR1_LT1_11                 (0x800UL << ADC_TR1_LT1_Pos)            /*!< 0x00000800 */

#define ADC_TR1_HT1_Pos                (16U)
#define ADC_TR1_HT1_Msk                (0xFFFUL << ADC_TR1_HT1_Pos)            /*!< 0x0FFF0000 */
#define ADC_TR1_HT1                    ADC_TR1_HT1_Msk                         /*!< ADC Analog watchdog 1 threshold high */
#define ADC_TR1_HT1_0                  (0x001UL << ADC_TR1_HT1_Pos)            /*!< 0x00010000 */
#define ADC_TR1_HT1_1                  (0x002UL << ADC_TR1_HT1_Pos)            /*!< 0x00020000 */
#define ADC_TR1_HT1_2                  (0x004UL << ADC_TR1_HT1_Pos)            /*!< 0x00040000 */
#define ADC_TR1_HT1_3                  (0x008UL << ADC_TR1_HT1_Pos)            /*!< 0x00080000 */
#define ADC_TR1_HT1_4                  (0x010UL << ADC_TR1_HT1_Pos)            /*!< 0x00100000 */
#define ADC_TR1_HT1_5                  (0x020UL << ADC_TR1_HT1_Pos)            /*!< 0x00200000 */
#define ADC_TR1_HT1_6                  (0x040UL << ADC_TR1_HT1_Pos)            /*!< 0x00400000 */
#define ADC_TR1_HT1_7                  (0x080UL << ADC_TR1_HT1_Pos)            /*!< 0x00800000 */
#define ADC_TR1_HT1_8                  (0x100UL << ADC_TR1_HT1_Pos)            /*!< 0x01000000 */
#define ADC_TR1_HT1_9                  (0x200UL << ADC_TR1_HT1_Pos)            /*!< 0x02000000 */
#define ADC_TR1_HT1_10                 (0x400UL << ADC_TR1_HT1_Pos)            /*!< 0x04000000 */
#define ADC_TR1_HT1_11                 (0x800UL << ADC_TR1_HT1_Pos)            /*!< 0x08000000 */

/********************  Bit definition for ADC_TR2 register  *******************/
#define ADC_TR2_LT2_Pos                (0U)
#define ADC_TR2_LT2_Msk                (0xFFFUL << ADC_TR2_LT2_Pos)            /*!< 0x00000FFF */
#define ADC_TR2_LT2                    ADC_TR2_LT2_Msk                         /*!< ADC analog watchdog 2 threshold low */
#define ADC_TR2_LT2_0                  (0x001UL << ADC_TR2_LT2_Pos)            /*!< 0x00000001 */
#define ADC_TR2_LT2_1                  (0x002UL << ADC_TR2_LT2_Pos)            /*!< 0x00000002 */
#define ADC_TR2_LT2_2                  (0x004UL << ADC_TR2_LT2_Pos)            /*!< 0x00000004 */
#define ADC_TR2_LT2_3                  (0x008UL << ADC_TR2_LT2_Pos)            /*!< 0x00000008 */
#define ADC_TR2_LT2_4                  (0x010UL << ADC_TR2_LT2_Pos)            /*!< 0x00000010 */
#define ADC_TR2_LT2_5                  (0x020UL << ADC_TR2_LT2_Pos)            /*!< 0x00000020 */
#define ADC_TR2_LT2_6                  (0x040UL << ADC_TR2_LT2_Pos)            /*!< 0x00000040 */
#define ADC_TR2_LT2_7                  (0x080UL << ADC_TR2_LT2_Pos)            /*!< 0x00000080 */
#define ADC_TR2_LT2_8                  (0x100UL << ADC_TR2_LT2_Pos)            /*!< 0x00000100 */
#define ADC_TR2_LT2_9                  (0x200UL << ADC_TR2_LT2_Pos)            /*!< 0x00000200 */
#define ADC_TR2_LT2_10                 (0x400UL << ADC_TR2_LT2_Pos)            /*!< 0x00000400 */
#define ADC_TR2_LT2_11                 (0x800UL << ADC_TR2_LT2_Pos)            /*!< 0x00000800 */

#define ADC_TR2_HT2_Pos                (16U)
#define ADC_TR2_HT2_Msk                (0xFFFUL << ADC_TR2_HT2_Pos)            /*!< 0x0FFF0000 */
#define ADC_TR2_HT2                    ADC_TR2_HT2_Msk                         /*!< ADC analog watchdog 2 threshold high */
#define ADC_TR2_HT2_0                  (0x001UL << ADC_TR2_HT2_Pos)            /*!< 0x00010000 */
#define ADC_TR2_HT2_1                  (0x002UL << ADC_TR2_HT2_Pos)            /*!< 0x00020000 */
#define ADC_TR2_HT2_2                  (0x004UL << ADC_TR2_HT2_Pos)            /*!< 0x00040000 */
#define ADC_TR2_HT2_3                  (0x008UL << ADC_TR2_HT2_Pos)            /*!< 0x00080000 */
#define ADC_TR2_HT2_4                  (0x010UL << ADC_TR2_HT2_Pos)            /*!< 0x00100000 */
#define ADC_TR2_HT2_5                  (0x020UL << ADC_TR2_HT2_Pos)            /*!< 0x00200000 */
#define ADC_TR2_HT2_6                  (0x040UL << ADC_TR2_HT2_Pos)            /*!< 0x00400000 */
#define ADC_TR2_HT2_7                  (0x080UL << ADC_TR2_HT2_Pos)            /*!< 0x00800000 */
#define ADC_TR2_HT2_8                  (0x100UL << ADC_TR2_HT2_Pos)            /*!< 0x01000000 */
#define ADC_TR2_HT2_9                  (0x200UL << ADC_TR2_HT2_Pos)            /*!< 0x02000000 */
#define ADC_TR2_HT2_10                 (0x400UL << ADC_TR2_HT2_Pos)            /*!< 0x04000000 */
#define ADC_TR2_HT2_11                 (0x800UL << ADC_TR2_HT2_Pos)            /*!< 0x08000000 */

/********************  Bit definition for ADC_CHSELR register  ****************/
#define ADC_CHSELR_CHSEL_Pos           (0U)
#define ADC_CHSELR_CHSEL_Msk           (0x7FFFFUL << ADC_CHSELR_CHSEL_Pos)     /*!< 0x0007FFFF */
#define ADC_CHSELR_CHSEL               ADC_CHSELR_CHSEL_Msk                    /*!< ADC group regular sequencer channels, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL18_Pos         (18U)
#define ADC_CHSELR_CHSEL18_Msk         (0x1UL << ADC_CHSELR_CHSEL18_Pos)       /*!< 0x00040000 */
#define ADC_CHSELR_CHSEL18             ADC_CHSELR_CHSEL18_Msk                  /*!< ADC group regular sequencer channel 18, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL17_Pos         (17U)
#define ADC_CHSELR_CHSEL17_Msk         (0x1UL << ADC_CHSELR_CHSEL17_Pos)       /*!< 0x00020000 */
#define ADC_CHSELR_CHSEL17             ADC_CHSELR_CHSEL17_Msk                  /*!< ADC group regular sequencer channel 17, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL16_Pos         (16U)
#define ADC_CHSELR_CHSEL16_Msk         (0x1UL << ADC_CHSELR_CHSEL16_Pos)       /*!< 0x00010000 */
#define ADC_CHSELR_CHSEL16             ADC_CHSELR_CHSEL16_Msk                  /*!< ADC group regular sequencer channel 16, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL15_Pos         (15U)
#define ADC_CHSELR_CHSEL15_Msk         (0x1UL << ADC_CHSELR_CHSEL15_Pos)       /*!< 0x00008000 */
#define ADC_CHSELR_CHSEL15             ADC_CHSELR_CHSEL15_Msk                  /*!< ADC group regular sequencer channel 15, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL14_Pos         (14U)
#define ADC_CHSELR_CHSEL14_Msk         (0x1UL << ADC_CHSELR_CHSEL14_Pos)       /*!< 0x00004000 */
#define ADC_CHSELR_CHSEL14             ADC_CHSELR_CHSEL14_Msk                  /*!< ADC group regular sequencer channel 14, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL13_Pos         (13U)
#define ADC_CHSELR_CHSEL13_Msk         (0x1UL << ADC_CHSELR_CHSEL13_Pos)       /*!< 0x00002000 */
#define ADC_CHSELR_CHSEL13             ADC_CHSELR_CHSEL13_Msk                  /*!< ADC group regular sequencer channel 13, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL12_Pos         (12U)
#define ADC_CHSELR_CHSEL12_Msk         (0x1UL << ADC_CHSELR_CHSEL12_Pos)       /*!< 0x00001000 */
#define ADC_CHSELR_CHSEL12             ADC_CHSELR_CHSEL12_Msk                  /*!< ADC group regular sequencer channel 12, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL11_Pos         (11U)
#define ADC_CHSELR_CHSEL11_Msk         (0x1UL << ADC_CHSELR_CHSEL11_Pos)       /*!< 0x00000800 */
#define ADC_CHSELR_CHSEL11             ADC_CHSELR_CHSEL11_Msk                  /*!< ADC group regular sequencer channel 11, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL10_Pos         (10U)
#define ADC_CHSELR_CHSEL10_Msk         (0x1UL << ADC_CHSELR_CHSEL10_Pos)       /*!< 0x00000400 */
#define ADC_CHSELR_CHSEL10             ADC_CHSELR_CHSEL10_Msk                  /*!< ADC group regular sequencer channel 10, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL9_Pos          (9U)
#define ADC_CHSELR_CHSEL9_Msk          (0x1UL << ADC_CHSELR_CHSEL9_Pos)        /*!< 0x00000200 */
#define ADC_CHSELR_CHSEL9              ADC_CHSELR_CHSEL9_Msk                   /*!< ADC group regular sequencer channel 9, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL8_Pos          (8U)
#define ADC_CHSELR_CHSEL8_Msk          (0x1UL << ADC_CHSELR_CHSEL8_Pos)        /*!< 0x00000100 */
#define ADC_CHSELR_CHSEL8              ADC_CHSELR_CHSEL8_Msk                   /*!< ADC group regular sequencer channel 8, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL7_Pos          (7U)
#define ADC_CHSELR_CHSEL7_Msk          (0x1UL << ADC_CHSELR_CHSEL7_Pos)        /*!< 0x00000080 */
#define ADC_CHSELR_CHSEL7              ADC_CHSELR_CHSEL7_Msk                   /*!< ADC group regular sequencer channel 7, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL6_Pos          (6U)
#define ADC_CHSELR_CHSEL6_Msk          (0x1UL << ADC_CHSELR_CHSEL6_Pos)        /*!< 0x00000040 */
#define ADC_CHSELR_CHSEL6              ADC_CHSELR_CHSEL6_Msk                   /*!< ADC group regular sequencer channel 6, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL5_Pos          (5U)
#define ADC_CHSELR_CHSEL5_Msk          (0x1UL << ADC_CHSELR_CHSEL5_Pos)        /*!< 0x00000020 */
#define ADC_CHSELR_CHSEL5              ADC_CHSELR_CHSEL5_Msk                   /*!< ADC group regular sequencer channel 5, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL4_Pos          (4U)
#define ADC_CHSELR_CHSEL4_Msk          (0x1UL << ADC_CHSELR_CHSEL4_Pos)        /*!< 0x00000010 */
#define ADC_CHSELR_CHSEL4              ADC_CHSELR_CHSEL4_Msk                   /*!< ADC group regular sequencer channel 4, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL3_Pos          (3U)
#define ADC_CHSELR_CHSEL3_Msk          (0x1UL << ADC_CHSELR_CHSEL3_Pos)        /*!< 0x00000008 */
#define ADC_CHSELR_CHSEL3              ADC_CHSELR_CHSEL3_Msk                   /*!< ADC group regular sequencer channel 3, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL2_Pos          (2U)
#define ADC_CHSELR_CHSEL2_Msk          (0x1UL << ADC_CHSELR_CHSEL2_Pos)        /*!< 0x00000004 */
#define ADC_CHSELR_CHSEL2              ADC_CHSELR_CHSEL2_Msk                   /*!< ADC group regular sequencer channel 2, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL1_Pos          (1U)
#define ADC_CHSELR_CHSEL1_Msk          (0x1UL << ADC_CHSELR_CHSEL1_Pos)        /*!< 0x00000002 */
#define ADC_CHSELR_CHSEL1              ADC_CHSELR_CHSEL1_Msk                   /*!< ADC group regular sequencer channel 1, available when ADC_CFGR1_CHSELRMOD is reset */
#define ADC_CHSELR_CHSEL0_Pos          (0U)
#define ADC_CHSELR_CHSEL0_Msk          (0x1UL << ADC_CHSELR_CHSEL0_Pos)        /*!< 0x00000001 */
#define ADC_CHSELR_CHSEL0              ADC_CHSELR_CHSEL0_Msk                   /*!< ADC group regular sequencer channel 0, available when ADC_CFGR1_CHSELRMOD is reset */

#define ADC_CHSELR_SQ_ALL_Pos          (0U)
#define ADC_CHSELR_SQ_ALL_Msk          (0xFFFFFFFFUL << ADC_CHSELR_SQ_ALL_Pos) /*!< 0xFFFFFFFF */
#define ADC_CHSELR_SQ_ALL              ADC_CHSELR_SQ_ALL_Msk                   /*!< ADC group regular sequencer all ranks, available when ADC_CFGR1_CHSELRMOD is set */

#define ADC_CHSELR_SQ8_Pos             (28U)
#define ADC_CHSELR_SQ8_Msk             (0xFUL << ADC_CHSELR_SQ8_Pos)           /*!< 0xF0000000 */
#define ADC_CHSELR_SQ8                 ADC_CHSELR_SQ8_Msk                      /*!< ADC group regular sequencer rank 8, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ8_0               (0x1UL << ADC_CHSELR_SQ8_Pos)           /*!< 0x10000000 */
#define ADC_CHSELR_SQ8_1               (0x2UL << ADC_CHSELR_SQ8_Pos)           /*!< 0x20000000 */
#define ADC_CHSELR_SQ8_2               (0x4UL << ADC_CHSELR_SQ8_Pos)           /*!< 0x40000000 */
#define ADC_CHSELR_SQ8_3               (0x8UL << ADC_CHSELR_SQ8_Pos)           /*!< 0x80000000 */

#define ADC_CHSELR_SQ7_Pos             (24U)
#define ADC_CHSELR_SQ7_Msk             (0xFUL << ADC_CHSELR_SQ7_Pos)           /*!< 0x0F000000 */
#define ADC_CHSELR_SQ7                 ADC_CHSELR_SQ7_Msk                      /*!< ADC group regular sequencer rank 7, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ7_0               (0x1UL << ADC_CHSELR_SQ7_Pos)           /*!< 0x01000000 */
#define ADC_CHSELR_SQ7_1               (0x2UL << ADC_CHSELR_SQ7_Pos)           /*!< 0x02000000 */
#define ADC_CHSELR_SQ7_2               (0x4UL << ADC_CHSELR_SQ7_Pos)           /*!< 0x04000000 */
#define ADC_CHSELR_SQ7_3               (0x8UL << ADC_CHSELR_SQ7_Pos)           /*!< 0x08000000 */

#define ADC_CHSELR_SQ6_Pos             (20U)
#define ADC_CHSELR_SQ6_Msk             (0xFUL << ADC_CHSELR_SQ6_Pos)           /*!< 0x00F00000 */
#define ADC_CHSELR_SQ6                 ADC_CHSELR_SQ6_Msk                      /*!< ADC group regular sequencer rank 6, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ6_0               (0x1UL << ADC_CHSELR_SQ6_Pos)           /*!< 0x00100000 */
#define ADC_CHSELR_SQ6_1               (0x2UL << ADC_CHSELR_SQ6_Pos)           /*!< 0x00200000 */
#define ADC_CHSELR_SQ6_2               (0x4UL << ADC_CHSELR_SQ6_Pos)           /*!< 0x00400000 */
#define ADC_CHSELR_SQ6_3               (0x8UL << ADC_CHSELR_SQ6_Pos)           /*!< 0x00800000 */

#define ADC_CHSELR_SQ5_Pos             (16U)
#define ADC_CHSELR_SQ5_Msk             (0xFUL << ADC_CHSELR_SQ5_Pos)           /*!< 0x000F0000 */
#define ADC_CHSELR_SQ5                 ADC_CHSELR_SQ5_Msk                      /*!< ADC group regular sequencer rank 5, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ5_0               (0x1UL << ADC_CHSELR_SQ5_Pos)           /*!< 0x00010000 */
#define ADC_CHSELR_SQ5_1               (0x2UL << ADC_CHSELR_SQ5_Pos)           /*!< 0x00020000 */
#define ADC_CHSELR_SQ5_2               (0x4UL << ADC_CHSELR_SQ5_Pos)           /*!< 0x00040000 */
#define ADC_CHSELR_SQ5_3               (0x8UL << ADC_CHSELR_SQ5_Pos)           /*!< 0x00080000 */

#define ADC_CHSELR_SQ4_Pos             (12U)
#define ADC_CHSELR_SQ4_Msk             (0xFUL << ADC_CHSELR_SQ4_Pos)           /*!< 0x0000F000 */
#define ADC_CHSELR_SQ4                 ADC_CHSELR_SQ4_Msk                      /*!< ADC group regular sequencer rank 4, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ4_0               (0x1UL << ADC_CHSELR_SQ4_Pos)           /*!< 0x00001000 */
#define ADC_CHSELR_SQ4_1               (0x2UL << ADC_CHSELR_SQ4_Pos)           /*!< 0x00002000 */
#define ADC_CHSELR_SQ4_2               (0x4UL << ADC_CHSELR_SQ4_Pos)           /*!< 0x00004000 */
#define ADC_CHSELR_SQ4_3               (0x8UL << ADC_CHSELR_SQ4_Pos)           /*!< 0x00008000 */

#define ADC_CHSELR_SQ3_Pos             (8U)
#define ADC_CHSELR_SQ3_Msk             (0xFUL << ADC_CHSELR_SQ3_Pos)           /*!< 0x00000F00 */
#define ADC_CHSELR_SQ3                 ADC_CHSELR_SQ3_Msk                      /*!< ADC group regular sequencer rank 3, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ3_0               (0x1UL << ADC_CHSELR_SQ3_Pos)           /*!< 0x00000100 */
#define ADC_CHSELR_SQ3_1               (0x2UL << ADC_CHSELR_SQ3_Pos)           /*!< 0x00000200 */
#define ADC_CHSELR_SQ3_2               (0x4UL << ADC_CHSELR_SQ3_Pos)           /*!< 0x00000400 */
#define ADC_CHSELR_SQ3_3               (0x8UL << ADC_CHSELR_SQ3_Pos)           /*!< 0x00000800 */

#define ADC_CHSELR_SQ2_Pos             (4U)
#define ADC_CHSELR_SQ2_Msk             (0xFUL << ADC_CHSELR_SQ2_Pos)           /*!< 0x000000F0 */
#define ADC_CHSELR_SQ2                 ADC_CHSELR_SQ2_Msk                      /*!< ADC group regular sequencer rank 2, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ2_0               (0x1UL << ADC_CHSELR_SQ2_Pos)           /*!< 0x00000010 */
#define ADC_CHSELR_SQ2_1               (0x2UL << ADC_CHSELR_SQ2_Pos)           /*!< 0x00000020 */
#define ADC_CHSELR_SQ2_2               (0x4UL << ADC_CHSELR_SQ2_Pos)           /*!< 0x00000040 */
#define ADC_CHSELR_SQ2_3               (0x8UL << ADC_CHSELR_SQ2_Pos)           /*!< 0x00000080 */

#define ADC_CHSELR_SQ1_Pos             (0U)
#define ADC_CHSELR_SQ1_Msk             (0xFUL << ADC_CHSELR_SQ1_Pos)           /*!< 0x0000000F */
#define ADC_CHSELR_SQ1                 ADC_CHSELR_SQ1_Msk                      /*!< ADC group regular sequencer rank 1, available when ADC_CFGR1_CHSELRMOD is set */
#define ADC_CHSELR_SQ1_0               (0x1UL << ADC_CHSELR_SQ1_Pos)           /*!< 0x00000001 */
#define ADC_CHSELR_SQ1_1               (0x2UL << ADC_CHSELR_SQ1_Pos)           /*!< 0x00000002 */
#define ADC_CHSELR_SQ1_2               (0x4UL << ADC_CHSELR_SQ1_Pos)           /*!< 0x00000004 */
#define ADC_CHSELR_SQ1_3               (0x8UL << ADC_CHSELR_SQ1_Pos)           /*!< 0x00000008 */

/********************  Bit definition for ADC_TR3 register  *******************/
#define ADC_TR3_LT3_Pos                (0U)
#define ADC_TR3_LT3_Msk                (0xFFFUL << ADC_TR3_LT3_Pos)            /*!< 0x00000FFF */
#define ADC_TR3_LT3                    ADC_TR3_LT3_Msk                         /*!< ADC analog watchdog 3 threshold low */
#define ADC_TR3_LT3_0                  (0x001UL << ADC_TR3_LT3_Pos)            /*!< 0x00000001 */
#define ADC_TR3_LT3_1                  (0x002UL << ADC_TR3_LT3_Pos)            /*!< 0x00000002 */
#define ADC_TR3_LT3_2                  (0x004UL << ADC_TR3_LT3_Pos)            /*!< 0x00000004 */
#define ADC_TR3_LT3_3                  (0x008UL << ADC_TR3_LT3_Pos)            /*!< 0x00000008 */
#define ADC_TR3_LT3_4                  (0x010UL << ADC_TR3_LT3_Pos)            /*!< 0x00000010 */
#define ADC_TR3_LT3_5                  (0x020UL << ADC_TR3_LT3_Pos)            /*!< 0x00000020 */
#define ADC_TR3_LT3_6                  (0x040UL << ADC_TR3_LT3_Pos)            /*!< 0x00000040 */
#define ADC_TR3_LT3_7                  (0x080UL << ADC_TR3_LT3_Pos)            /*!< 0x00000080 */
#define ADC_TR3_LT3_8                  (0x100UL << ADC_TR3_LT3_Pos)            /*!< 0x00000100 */
#define ADC_TR3_LT3_9                  (0x200UL << ADC_TR3_LT3_Pos)            /*!< 0x00000200 */
#define ADC_TR3_LT3_10                 (0x400UL << ADC_TR3_LT3_Pos)            /*!< 0x00000400 */
#define ADC_TR3_LT3_11                 (0x800UL << ADC_TR3_LT3_Pos)            /*!< 0x00000800 */

#define ADC_TR3_HT3_Pos                (16U)
#define ADC_TR3_HT3_Msk                (0xFFFUL << ADC_TR3_HT3_Pos)            /*!< 0x0FFF0000 */
#define ADC_TR3_HT3                    ADC_TR3_HT3_Msk                         /*!< ADC analog watchdog 3 threshold high */
#define ADC_TR3_HT3_0                  (0x001UL << ADC_TR3_HT3_Pos)            /*!< 0x00010000 */
#define ADC_TR3_HT3_1                  (0x002UL << ADC_TR3_HT3_Pos)            /*!< 0x00020000 */
#define ADC_TR3_HT3_2                  (0x004UL << ADC_TR3_HT3_Pos)            /*!< 0x00040000 */
#define ADC_TR3_HT3_3                  (0x008UL << ADC_TR3_HT3_Pos)            /*!< 0x00080000 */
#define ADC_TR3_HT3_4                  (0x010UL << ADC_TR3_HT3_Pos)            /*!< 0x00100000 */
#define ADC_TR3_HT3_5                  (0x020UL << ADC_TR3_HT3_Pos)            /*!< 0x00200000 */
#define ADC_TR3_HT3_6                  (0x040UL << ADC_TR3_HT3_Pos)            /*!< 0x00400000 */
#define ADC_TR3_HT3_7                  (0x080UL << ADC_TR3_HT3_Pos)            /*!< 0x00800000 */
#define ADC_TR3_HT3_8                  (0x100UL << ADC_TR3_HT3_Pos)            /*!< 0x01000000 */
#define ADC_TR3_HT3_9                  (0x200UL << ADC_TR3_HT3_Pos)            /*!< 0x02000000 */
#define ADC_TR3_HT3_10                 (0x400UL << ADC_TR3_HT3_Pos)            /*!< 0x04000000 */
#define ADC_TR3_HT3_11                 (0x800UL << ADC_TR3_HT3_Pos)            /*!< 0x08000000 */

/********************  Bit definition for ADC_DR register  ********************/
#define ADC_DR_DATA_Pos                (0U)
#define ADC_DR_DATA_Msk                (0xFFFFUL << ADC_DR_DATA_Pos)           /*!< 0x0000FFFF */
#define ADC_DR_DATA                    ADC_DR_DATA_Msk                         /*!< ADC group regular conversion data */
#define ADC_DR_DATA_0                  (0x0001UL << ADC_DR_DATA_Pos)           /*!< 0x00000001 */
#define ADC_DR_DATA_1                  (0x0002UL << ADC_DR_DATA_Pos)           /*!< 0x00000002 */
#define ADC_DR_DATA_2                  (0x0004UL << ADC_DR_DATA_Pos)           /*!< 0x00000004 */
#define ADC_DR_DATA_3                  (0x0008UL << ADC_DR_DATA_Pos)           /*!< 0x00000008 */
#define ADC_DR_DATA_4                  (0x0010UL << ADC_DR_DATA_Pos)           /*!< 0x00000010 */
#define ADC_DR_DATA_5                  (0x0020UL << ADC_DR_DATA_Pos)           /*!< 0x00000020 */
#define ADC_DR_DATA_6                  (0x0040UL << ADC_DR_DATA_Pos)           /*!< 0x00000040 */
#define ADC_DR_DATA_7                  (0x0080UL << ADC_DR_DATA_Pos)           /*!< 0x00000080 */
#define ADC_DR_DATA_8                  (0x0100UL << ADC_DR_DATA_Pos)           /*!< 0x00000100 */
#define ADC_DR_DATA_9                  (0x0200UL << ADC_DR_DATA_Pos)           /*!< 0x00000200 */
#define ADC_DR_DATA_10                 (0x0400UL << ADC_DR_DATA_Pos)           /*!< 0x00000400 */
#define ADC_DR_DATA_11                 (0x0800UL << ADC_DR_DATA_Pos)           /*!< 0x00000800 */
#define ADC_DR_DATA_12                 (0x1000UL << ADC_DR_DATA_Pos)           /*!< 0x00001000 */
#define ADC_DR_DATA_13                 (0x2000UL << ADC_DR_DATA_Pos)           /*!< 0x00002000 */
#define ADC_DR_DATA_14                 (0x4000UL << ADC_DR_DATA_Pos)           /*!< 0x00004000 */
#define ADC_DR_DATA_15                 (0x8000UL << ADC_DR_DATA_Pos)           /*!< 0x00008000 */

/********************  Bit definition for ADC_AWD2CR register  ****************/
#define ADC_AWD2CR_AWD2CH_Pos          (0U)
#define ADC_AWD2CR_AWD2CH_Msk          (0x7FFFFUL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x0007FFFF */
#define ADC_AWD2CR_AWD2CH              ADC_AWD2CR_AWD2CH_Msk                   /*!< ADC analog watchdog 2 monitored channel selection */
#define ADC_AWD2CR_AWD2CH_0            (0x00001UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000001 */
#define ADC_AWD2CR_AWD2CH_1            (0x00002UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000002 */
#define ADC_AWD2CR_AWD2CH_2            (0x00004UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000004 */
#define ADC_AWD2CR_AWD2CH_3            (0x00008UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000008 */
#define ADC_AWD2CR_AWD2CH_4            (0x00010UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000010 */
#define ADC_AWD2CR_AWD2CH_5            (0x00020UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000020 */
#define ADC_AWD2CR_AWD2CH_6            (0x00040UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000040 */
#define ADC_AWD2CR_AWD2CH_7            (0x00080UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000080 */
#define ADC_AWD2CR_AWD2CH_8            (0x00100UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000100 */
#define ADC_AWD2CR_AWD2CH_9            (0x00200UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000200 */
#define ADC_AWD2CR_AWD2CH_10           (0x00400UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000400 */
#define ADC_AWD2CR_AWD2CH_11           (0x00800UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00000800 */
#define ADC_AWD2CR_AWD2CH_12           (0x01000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00001000 */
#define ADC_AWD2CR_AWD2CH_13           (0x02000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00002000 */
#define ADC_AWD2CR_AWD2CH_14           (0x04000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00004000 */
#define ADC_AWD2CR_AWD2CH_15           (0x08000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00008000 */
#define ADC_AWD2CR_AWD2CH_16           (0x10000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00010000 */
#define ADC_AWD2CR_AWD2CH_17           (0x20000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00020000 */
#define ADC_AWD2CR_AWD2CH_18           (0x40000UL << ADC_AWD2CR_AWD2CH_Pos)    /*!< 0x00040000 */

/********************  Bit definition for ADC_AWD3CR register  ****************/
#define ADC_AWD3CR_AWD3CH_Pos          (0U)
#define ADC_AWD3CR_AWD3CH_Msk          (0x7FFFFUL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x0007FFFF */
#define ADC_AWD3CR_AWD3CH              ADC_AWD3CR_AWD3CH_Msk                   /*!< ADC analog watchdog 3 monitored channel selection */
#define ADC_AWD3CR_AWD3CH_0            (0x00001UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000001 */
#define ADC_AWD3CR_AWD3CH_1            (0x00002UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000002 */
#define ADC_AWD3CR_AWD3CH_2            (0x00004UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000004 */
#define ADC_AWD3CR_AWD3CH_3            (0x00008UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000008 */
#define ADC_AWD3CR_AWD3CH_4            (0x00010UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000010 */
#define ADC_AWD3CR_AWD3CH_5            (0x00020UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000020 */
#define ADC_AWD3CR_AWD3CH_6            (0x00040UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000040 */
#define ADC_AWD3CR_AWD3CH_7            (0x00080UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000080 */
#define ADC_AWD3CR_AWD3CH_8            (0x00100UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000100 */
#define ADC_AWD3CR_AWD3CH_9            (0x00200UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000200 */
#define ADC_AWD3CR_AWD3CH_10           (0x00400UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000400 */
#define ADC_AWD3CR_AWD3CH_11           (0x00800UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00000800 */
#define ADC_AWD3CR_AWD3CH_12           (0x01000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00001000 */
#define ADC_AWD3CR_AWD3CH_13           (0x02000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00002000 */
#define ADC_AWD3CR_AWD3CH_14           (0x04000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00004000 */
#define ADC_AWD3CR_AWD3CH_15           (0x08000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00008000 */
#define ADC_AWD3CR_AWD3CH_16           (0x10000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00010000 */
#define ADC_AWD3CR_AWD3CH_17           (0x20000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00020000 */
#define ADC_AWD3CR_AWD3CH_18           (0x40000UL << ADC_AWD3CR_AWD3CH_Pos)    /*!< 0x00040000 */

/********************  Bit definition for ADC_CALFACT register  ***************/
#define ADC_CALFACT_CALFACT_Pos        (0U)
#define ADC_CALFACT_CALFACT_Msk        (0x7FUL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x0000007F */
#define ADC_CALFACT_CALFACT            ADC_CALFACT_CALFACT_Msk                 /*!< ADC calibration factor in single-ended mode */
#define ADC_CALFACT_CALFACT_0          (0x01UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000001 */
#define ADC_CALFACT_CALFACT_1          (0x02UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000002 */
#define ADC_CALFACT_CALFACT_2          (0x04UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000004 */
#define ADC_CALFACT_CALFACT_3          (0x08UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000008 */
#define ADC_CALFACT_CALFACT_4          (0x10UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000010 */
#define ADC_CALFACT_CALFACT_5          (0x20UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000020 */
#define ADC_CALFACT_CALFACT_6          (0x40UL << ADC_CALFACT_CALFACT_Pos)     /*!< 0x00000040 */

/*************************  ADC Common registers  *****************************/
/********************  Bit definition for ADC_CCR register  *******************/
#define ADC_CCR_PRESC_Pos              (18U)
#define ADC_CCR_PRESC_Msk              (0xFUL << ADC_CCR_PRESC_Pos)            /*!< 0x003C0000 */
#define ADC_CCR_PRESC                  ADC_CCR_PRESC_Msk                       /*!< ADC common clock prescaler, only for clock source asynchronous */
#define ADC_CCR_PRESC_0                (0x1UL << ADC_CCR_PRESC_Pos)            /*!< 0x00040000 */
#define ADC_CCR_PRESC_1                (0x2UL << ADC_CCR_PRESC_Pos)            /*!< 0x00080000 */
#define ADC_CCR_PRESC_2                (0x4UL << ADC_CCR_PRESC_Pos)            /*!< 0x00100000 */
#define ADC_CCR_PRESC_3                (0x8UL << ADC_CCR_PRESC_Pos)            /*!< 0x00200000 */

#define ADC_CCR_VREFEN_Pos             (22U)
#define ADC_CCR_VREFEN_Msk             (0x1UL << ADC_CCR_VREFEN_Pos)           /*!< 0x00400000 */
#define ADC_CCR_VREFEN                 ADC_CCR_VREFEN_Msk                      /*!< ADC internal path to VrefInt enable */
#define ADC_CCR_TSEN_Pos               (23U)
#define ADC_CCR_TSEN_Msk               (0x1UL << ADC_CCR_TSEN_Pos)             /*!< 0x00800000 */
#define ADC_CCR_TSEN                   ADC_CCR_TSEN_Msk                        /*!< ADC internal path to temperature sensor enable */
#define ADC_CCR_VBATEN_Pos             (24U)
#define ADC_CCR_VBATEN_Msk             (0x1UL << ADC_CCR_VBATEN_Pos)           /*!< 0x01000000 */
#define ADC_CCR_VBATEN                 ADC_CCR_VBATEN_Msk                      /*!< ADC internal path to battery voltage enable */

#define ADC_CCR_LFMEN_Pos              (25U)
#define ADC_CCR_LFMEN_Msk              (0x1UL << ADC_CCR_LFMEN_Pos)            /*!< 0x02000000 */
#define ADC_CCR_LFMEN                  ADC_CCR_LFMEN_Msk                       /*!< ADC common clock low frequency mode */

/******************************************************************************/
/*                                                                            */
/*                                 HDMI-CEC (CEC)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for CEC_CR register  *********************/
#define CEC_CR_CECEN_Pos         (0U)
#define CEC_CR_CECEN_Msk         (0x1UL << CEC_CR_CECEN_Pos)                    /*!< 0x00000001 */
#define CEC_CR_CECEN             CEC_CR_CECEN_Msk                               /*!< CEC Enable                         */
#define CEC_CR_TXSOM_Pos         (1U)
#define CEC_CR_TXSOM_Msk         (0x1UL << CEC_CR_TXSOM_Pos)                    /*!< 0x00000002 */
#define CEC_CR_TXSOM             CEC_CR_TXSOM_Msk                               /*!< CEC Tx Start Of Message            */
#define CEC_CR_TXEOM_Pos         (2U)
#define CEC_CR_TXEOM_Msk         (0x1UL << CEC_CR_TXEOM_Pos)                    /*!< 0x00000004 */
#define CEC_CR_TXEOM             CEC_CR_TXEOM_Msk                               /*!< CEC Tx End Of Message              */

/*******************  Bit definition for CEC_CFGR register  *******************/
#define CEC_CFGR_SFT_Pos         (0U)
#define CEC_CFGR_SFT_Msk         (0x7UL << CEC_CFGR_SFT_Pos)                    /*!< 0x00000007 */
#define CEC_CFGR_SFT             CEC_CFGR_SFT_Msk                               /*!< CEC Signal Free Time               */
#define CEC_CFGR_RXTOL_Pos       (3U)
#define CEC_CFGR_RXTOL_Msk       (0x1UL << CEC_CFGR_RXTOL_Pos)                  /*!< 0x00000008 */
#define CEC_CFGR_RXTOL           CEC_CFGR_RXTOL_Msk                             /*!< CEC Tolerance                      */
#define CEC_CFGR_BRESTP_Pos      (4U)
#define CEC_CFGR_BRESTP_Msk      (0x1UL << CEC_CFGR_BRESTP_Pos)                 /*!< 0x00000010 */
#define CEC_CFGR_BRESTP          CEC_CFGR_BRESTP_Msk                            /*!< CEC Rx Stop                        */
#define CEC_CFGR_BREGEN_Pos      (5U)
#define CEC_CFGR_BREGEN_Msk      (0x1UL << CEC_CFGR_BREGEN_Pos)                 /*!< 0x00000020 */
#define CEC_CFGR_BREGEN          CEC_CFGR_BREGEN_Msk                            /*!< CEC Bit Rising Error generation    */
#define CEC_CFGR_LBPEGEN_Pos     (6U)
#define CEC_CFGR_LBPEGEN_Msk     (0x1UL << CEC_CFGR_LBPEGEN_Pos)                /*!< 0x00000040 */
#define CEC_CFGR_LBPEGEN         CEC_CFGR_LBPEGEN_Msk                           /*!< CEC Long Bit Period Error gener.   */
#define CEC_CFGR_BRDNOGEN_Pos    (7U)
#define CEC_CFGR_BRDNOGEN_Msk    (0x1UL << CEC_CFGR_BRDNOGEN_Pos)               /*!< 0x00000080 */
#define CEC_CFGR_BRDNOGEN        CEC_CFGR_BRDNOGEN_Msk                          /*!< CEC Broadcast No Error generation  */
#define CEC_CFGR_SFTOPT_Pos      (8U)
#define CEC_CFGR_SFTOPT_Msk      (0x1UL << CEC_CFGR_SFTOPT_Pos)                 /*!< 0x00000100 */
#define CEC_CFGR_SFTOPT          CEC_CFGR_SFTOPT_Msk                            /*!< CEC Signal Free Time optional      */
#define CEC_CFGR_OAR_Pos         (16U)
#define CEC_CFGR_OAR_Msk         (0x7FFFUL << CEC_CFGR_OAR_Pos)                 /*!< 0x7FFF0000 */
#define CEC_CFGR_OAR             CEC_CFGR_OAR_Msk                               /*!< CEC Own Address                    */
#define CEC_CFGR_LSTN_Pos        (31U)
#define CEC_CFGR_LSTN_Msk        (0x1UL << CEC_CFGR_LSTN_Pos)                   /*!< 0x80000000 */
#define CEC_CFGR_LSTN            CEC_CFGR_LSTN_Msk                              /*!< CEC Listen mode                    */

/*******************  Bit definition for CEC_TXDR register  *******************/
#define CEC_TXDR_TXD_Pos         (0U)
#define CEC_TXDR_TXD_Msk         (0xFFUL << CEC_TXDR_TXD_Pos)                   /*!< 0x000000FF */
#define CEC_TXDR_TXD             CEC_TXDR_TXD_Msk                               /*!< CEC Tx Data                        */

/*******************  Bit definition for CEC_RXDR register  *******************/
#define CEC_RXDR_RXD_Pos         (0U)
#define CEC_RXDR_RXD_Msk         (0xFFUL << CEC_RXDR_RXD_Pos)                   /*!< 0x000000FF */
#define CEC_RXDR_RXD             CEC_RXDR_RXD_Msk                               /*!< CEC Rx Data                        */

/*******************  Bit definition for CEC_ISR register  ********************/
#define CEC_ISR_RXBR_Pos         (0U)
#define CEC_ISR_RXBR_Msk         (0x1UL << CEC_ISR_RXBR_Pos)                    /*!< 0x00000001 */
#define CEC_ISR_RXBR             CEC_ISR_RXBR_Msk                               /*!< CEC Rx-Byte Received                   */
#define CEC_ISR_RXEND_Pos        (1U)
#define CEC_ISR_RXEND_Msk        (0x1UL << CEC_ISR_RXEND_Pos)                   /*!< 0x00000002 */
#define CEC_ISR_RXEND            CEC_ISR_RXEND_Msk                              /*!< CEC End Of Reception                   */
#define CEC_ISR_RXOVR_Pos        (2U)
#define CEC_ISR_RXOVR_Msk        (0x1UL << CEC_ISR_RXOVR_Pos)                   /*!< 0x00000004 */
#define CEC_ISR_RXOVR            CEC_ISR_RXOVR_Msk                              /*!< CEC Rx-Overrun                         */
#define CEC_ISR_BRE_Pos          (3U)
#define CEC_ISR_BRE_Msk          (0x1UL << CEC_ISR_BRE_Pos)                     /*!< 0x00000008 */
#define CEC_ISR_BRE              CEC_ISR_BRE_Msk                                /*!< CEC Rx Bit Rising Error                */
#define CEC_ISR_SBPE_Pos         (4U)
#define CEC_ISR_SBPE_Msk         (0x1UL << CEC_ISR_SBPE_Pos)                    /*!< 0x00000010 */
#define CEC_ISR_SBPE             CEC_ISR_SBPE_Msk                               /*!< CEC Rx Short Bit period Error          */
#define CEC_ISR_LBPE_Pos         (5U)
#define CEC_ISR_LBPE_Msk         (0x1UL << CEC_ISR_LBPE_Pos)                    /*!< 0x00000020 */
#define CEC_ISR_LBPE             CEC_ISR_LBPE_Msk                               /*!< CEC Rx Long Bit period Error           */
#define CEC_ISR_RXACKE_Pos       (6U)
#define CEC_ISR_RXACKE_Msk       (0x1UL << CEC_ISR_RXACKE_Pos)                  /*!< 0x00000040 */
#define CEC_ISR_RXACKE           CEC_ISR_RXACKE_Msk                             /*!< CEC Rx Missing Acknowledge             */
#define CEC_ISR_ARBLST_Pos       (7U)
#define CEC_ISR_ARBLST_Msk       (0x1UL << CEC_ISR_ARBLST_Pos)                  /*!< 0x00000080 */
#define CEC_ISR_ARBLST           CEC_ISR_ARBLST_Msk                             /*!< CEC Arbitration Lost                   */
#define CEC_ISR_TXBR_Pos         (8U)
#define CEC_ISR_TXBR_Msk         (0x1UL << CEC_ISR_TXBR_Pos)                    /*!< 0x00000100 */
#define CEC_ISR_TXBR             CEC_ISR_TXBR_Msk                               /*!< CEC Tx Byte Request                    */
#define CEC_ISR_TXEND_Pos        (9U)
#define CEC_ISR_TXEND_Msk        (0x1UL << CEC_ISR_TXEND_Pos)                   /*!< 0x00000200 */
#define CEC_ISR_TXEND            CEC_ISR_TXEND_Msk                              /*!< CEC End of Transmission                */
#define CEC_ISR_TXUDR_Pos        (10U)
#define CEC_ISR_TXUDR_Msk        (0x1UL << CEC_ISR_TXUDR_Pos)                   /*!< 0x00000400 */
#define CEC_ISR_TXUDR            CEC_ISR_TXUDR_Msk                              /*!< CEC Tx-Buffer Underrun                 */
#define CEC_ISR_TXERR_Pos        (11U)
#define CEC_ISR_TXERR_Msk        (0x1UL << CEC_ISR_TXERR_Pos)                   /*!< 0x00000800 */
#define CEC_ISR_TXERR            CEC_ISR_TXERR_Msk                              /*!< CEC Tx-Error                           */
#define CEC_ISR_TXACKE_Pos       (12U)
#define CEC_ISR_TXACKE_Msk       (0x1UL << CEC_ISR_TXACKE_Pos)                  /*!< 0x00001000 */
#define CEC_ISR_TXACKE           CEC_ISR_TXACKE_Msk                             /*!< CEC Tx Missing Acknowledge             */

/*******************  Bit definition for CEC_IER register  ********************/
#define CEC_IER_RXBRIE_Pos       (0U)
#define CEC_IER_RXBRIE_Msk       (0x1UL << CEC_IER_RXBRIE_Pos)                  /*!< 0x00000001 */
#define CEC_IER_RXBRIE           CEC_IER_RXBRIE_Msk                             /*!< CEC Rx-Byte Received IT Enable         */
#define CEC_IER_RXENDIE_Pos      (1U)
#define CEC_IER_RXENDIE_Msk      (0x1UL << CEC_IER_RXENDIE_Pos)                 /*!< 0x00000002 */
#define CEC_IER_RXENDIE          CEC_IER_RXENDIE_Msk                            /*!< CEC End Of Reception IT Enable         */
#define CEC_IER_RXOVRIE_Pos      (2U)
#define CEC_IER_RXOVRIE_Msk      (0x1UL << CEC_IER_RXOVRIE_Pos)                 /*!< 0x00000004 */
#define CEC_IER_RXOVRIE          CEC_IER_RXOVRIE_Msk                            /*!< CEC Rx-Overrun IT Enable               */
#define CEC_IER_BREIE_Pos        (3U)
#define CEC_IER_BREIE_Msk        (0x1UL << CEC_IER_BREIE_Pos)                   /*!< 0x00000008 */
#define CEC_IER_BREIE            CEC_IER_BREIE_Msk                              /*!< CEC Rx Bit Rising Error IT Enable      */
#define CEC_IER_SBPEIE_Pos       (4U)
#define CEC_IER_SBPEIE_Msk       (0x1UL << CEC_IER_SBPEIE_Pos)                  /*!< 0x00000010 */
#define CEC_IER_SBPEIE           CEC_IER_SBPEIE_Msk                             /*!< CEC Rx Short Bit period Error IT Enable*/
#define CEC_IER_LBPEIE_Pos       (5U)
#define CEC_IER_LBPEIE_Msk       (0x1UL << CEC_IER_LBPEIE_Pos)                  /*!< 0x00000020 */
#define CEC_IER_LBPEIE           CEC_IER_LBPEIE_Msk                             /*!< CEC Rx Long Bit period Error IT Enable */
#define CEC_IER_RXACKEIE_Pos     (6U)
#define CEC_IER_RXACKEIE_Msk     (0x1UL << CEC_IER_RXACKEIE_Pos)                /*!< 0x00000040 */
#define CEC_IER_RXACKEIE         CEC_IER_RXACKEIE_Msk                           /*!< CEC Rx Missing Acknowledge IT Enable   */
#define CEC_IER_ARBLSTIE_Pos     (7U)
#define CEC_IER_ARBLSTIE_Msk     (0x1UL << CEC_IER_ARBLSTIE_Pos)                /*!< 0x00000080 */
#define CEC_IER_ARBLSTIE         CEC_IER_ARBLSTIE_Msk                           /*!< CEC Arbitration Lost IT Enable         */
#define CEC_IER_TXBRIE_Pos       (8U)
#define CEC_IER_TXBRIE_Msk       (0x1UL << CEC_IER_TXBRIE_Pos)                  /*!< 0x00000100 */
#define CEC_IER_TXBRIE           CEC_IER_TXBRIE_Msk                             /*!< CEC Tx Byte Request  IT Enable         */
#define CEC_IER_TXENDIE_Pos      (9U)
#define CEC_IER_TXENDIE_Msk      (0x1UL << CEC_IER_TXENDIE_Pos)                 /*!< 0x00000200 */
#define CEC_IER_TXENDIE          CEC_IER_TXENDIE_Msk                            /*!< CEC End of Transmission IT Enable      */
#define CEC_IER_TXUDRIE_Pos      (10U)
#define CEC_IER_TXUDRIE_Msk      (0x1UL << CEC_IER_TXUDRIE_Pos)                 /*!< 0x00000400 */
#define CEC_IER_TXUDRIE          CEC_IER_TXUDRIE_Msk                            /*!< CEC Tx-Buffer Underrun IT Enable       */
#define CEC_IER_TXERRIE_Pos      (11U)
#define CEC_IER_TXERRIE_Msk      (0x1UL << CEC_IER_TXERRIE_Pos)                 /*!< 0x00000800 */
#define CEC_IER_TXERRIE          CEC_IER_TXERRIE_Msk                            /*!< CEC Tx-Error IT Enable                 */
#define CEC_IER_TXACKEIE_Pos     (12U)
#define CEC_IER_TXACKEIE_Msk     (0x1UL << CEC_IER_TXACKEIE_Pos)                /*!< 0x00001000 */
#define CEC_IER_TXACKEIE         CEC_IER_TXACKEIE_Msk                           /*!< CEC Tx Missing Acknowledge IT Enable   */

/******************************************************************************/
/*                                                                            */
/*                          CRC calculation unit                              */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for CRC_DR register  *********************/
#define CRC_DR_DR_Pos            (0U)
#define CRC_DR_DR_Msk            (0xFFFFFFFFUL << CRC_DR_DR_Pos)                /*!< 0xFFFFFFFF */
#define CRC_DR_DR                CRC_DR_DR_Msk                                  /*!< Data register bits */

/*******************  Bit definition for CRC_IDR register  ********************/
#define CRC_IDR_IDR_Pos          (0U)
#define CRC_IDR_IDR_Msk          (0xFFFFFFFFUL << CRC_IDR_IDR_Pos)              /*!< 0xFFFFFFFF */
#define CRC_IDR_IDR              CRC_IDR_IDR_Msk                                /*!< General-purpose 32-bits data register bits */

/********************  Bit definition for CRC_CR register  ********************/
#define CRC_CR_RESET_Pos         (0U)
#define CRC_CR_RESET_Msk         (0x1UL << CRC_CR_RESET_Pos)                    /*!< 0x00000001 */
#define CRC_CR_RESET             CRC_CR_RESET_Msk                               /*!< RESET the CRC computation unit bit */
#define CRC_CR_POLYSIZE_Pos      (3U)
#define CRC_CR_POLYSIZE_Msk      (0x3UL << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000018 */
#define CRC_CR_POLYSIZE          CRC_CR_POLYSIZE_Msk                            /*!< Polynomial size bits */
#define CRC_CR_POLYSIZE_0        (0x1UL << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000008 */
#define CRC_CR_POLYSIZE_1        (0x2UL << CRC_CR_POLYSIZE_Pos)                 /*!< 0x00000010 */
#define CRC_CR_REV_IN_Pos        (5U)
#define CRC_CR_REV_IN_Msk        (0x3UL << CRC_CR_REV_IN_Pos)                   /*!< 0x00000060 */
#define CRC_CR_REV_IN            CRC_CR_REV_IN_Msk                              /*!< REV_IN Reverse Input Data bits */
#define CRC_CR_REV_IN_0          (0x1UL << CRC_CR_REV_IN_Pos)                   /*!< 0x00000020 */
#define CRC_CR_REV_IN_1          (0x2UL << CRC_CR_REV_IN_Pos)                   /*!< 0x00000040 */
#define CRC_CR_REV_OUT_Pos       (7U)
#define CRC_CR_REV_OUT_Msk       (0x1UL << CRC_CR_REV_OUT_Pos)                  /*!< 0x00000080 */
#define CRC_CR_REV_OUT           CRC_CR_REV_OUT_Msk                             /*!< REV_OUT Reverse Output Data bits */

/*******************  Bit definition for CRC_INIT register  *******************/
#define CRC_INIT_INIT_Pos        (0U)
#define CRC_INIT_INIT_Msk        (0xFFFFFFFFUL << CRC_INIT_INIT_Pos)            /*!< 0xFFFFFFFF */
#define CRC_INIT_INIT            CRC_INIT_INIT_Msk                              /*!< Initial CRC value bits */

/*******************  Bit definition for CRC_POL register  ********************/
#define CRC_POL_POL_Pos          (0U)
#define CRC_POL_POL_Msk          (0xFFFFFFFFUL << CRC_POL_POL_Pos)              /*!< 0xFFFFFFFF */
#define CRC_POL_POL              CRC_POL_POL_Msk                                /*!< Coefficients of the polynomial */

/******************************************************************************/
/*                                                                            */
/*                       Advanced Encryption Standard (AES)                   */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for AES_CR register  *********************/
#define AES_CR_EN_Pos            (0U)
#define AES_CR_EN_Msk            (0x1UL << AES_CR_EN_Pos)                       /*!< 0x00000001 */
#define AES_CR_EN                AES_CR_EN_Msk                                  /*!< AES Enable */
#define AES_CR_DATATYPE_Pos      (1U)
#define AES_CR_DATATYPE_Msk      (0x3UL << AES_CR_DATATYPE_Pos)                 /*!< 0x00000006 */
#define AES_CR_DATATYPE          AES_CR_DATATYPE_Msk                            /*!< Data type selection */
#define AES_CR_DATATYPE_0        (0x1UL << AES_CR_DATATYPE_Pos)                 /*!< 0x00000002 */
#define AES_CR_DATATYPE_1        (0x2UL << AES_CR_DATATYPE_Pos)                 /*!< 0x00000004 */

#define AES_CR_MODE_Pos          (3U)
#define AES_CR_MODE_Msk          (0x3UL << AES_CR_MODE_Pos)                     /*!< 0x00000018 */
#define AES_CR_MODE              AES_CR_MODE_Msk                                /*!< AES Mode Of Operation */
#define AES_CR_MODE_0            (0x1UL << AES_CR_MODE_Pos)                     /*!< 0x00000008 */
#define AES_CR_MODE_1            (0x2UL << AES_CR_MODE_Pos)                     /*!< 0x00000010 */

#define AES_CR_CHMOD_Pos         (5U)
#define AES_CR_CHMOD_Msk         (0x803UL << AES_CR_CHMOD_Pos)                  /*!< 0x00010060 */
#define AES_CR_CHMOD             AES_CR_CHMOD_Msk                               /*!< AES Chaining Mode */
#define AES_CR_CHMOD_0           (0x001UL << AES_CR_CHMOD_Pos)                  /*!< 0x00000020 */
#define AES_CR_CHMOD_1           (0x002UL << AES_CR_CHMOD_Pos)                  /*!< 0x00000040 */
#define AES_CR_CHMOD_2           (0x800UL << AES_CR_CHMOD_Pos)                  /*!< 0x00010000 */

#define AES_CR_CCFC_Pos          (7U)
#define AES_CR_CCFC_Msk          (0x1UL << AES_CR_CCFC_Pos)                     /*!< 0x00000080 */
#define AES_CR_CCFC              AES_CR_CCFC_Msk                                /*!< Computation Complete Flag Clear */
#define AES_CR_ERRC_Pos          (8U)
#define AES_CR_ERRC_Msk          (0x1UL << AES_CR_ERRC_Pos)                     /*!< 0x00000100 */
#define AES_CR_ERRC              AES_CR_ERRC_Msk                                /*!< Error Clear */
#define AES_CR_CCFIE_Pos         (9U)
#define AES_CR_CCFIE_Msk         (0x1UL << AES_CR_CCFIE_Pos)                    /*!< 0x00000200 */
#define AES_CR_CCFIE             AES_CR_CCFIE_Msk                               /*!< Computation Complete Flag Interrupt Enable */
#define AES_CR_ERRIE_Pos         (10U)
#define AES_CR_ERRIE_Msk         (0x1UL << AES_CR_ERRIE_Pos)                    /*!< 0x00000400 */
#define AES_CR_ERRIE             AES_CR_ERRIE_Msk                               /*!< Error Interrupt Enable */
#define AES_CR_DMAINEN_Pos       (11U)
#define AES_CR_DMAINEN_Msk       (0x1UL << AES_CR_DMAINEN_Pos)                  /*!< 0x00000800 */
#define AES_CR_DMAINEN           AES_CR_DMAINEN_Msk                             /*!< Enable data input phase DMA management  */
#define AES_CR_DMAOUTEN_Pos      (12U)
#define AES_CR_DMAOUTEN_Msk      (0x1UL << AES_CR_DMAOUTEN_Pos)                 /*!< 0x00001000 */
#define AES_CR_DMAOUTEN          AES_CR_DMAOUTEN_Msk                            /*!< Enable data output phase DMA management */

#define AES_CR_NPBLB_Pos         (20U)
#define AES_CR_NPBLB_Msk         (0xFUL << AES_CR_NPBLB_Pos)                    /*!< 0x00F00000 */
#define AES_CR_NPBLB             AES_CR_NPBLB_Msk                               /*!< Number of padding bytes in last block of payload. */
#define AES_CR_NPBLB_0           (0x1UL << AES_CR_NPBLB_Pos)                    /*!< 0x00100000 */
#define AES_CR_NPBLB_1           (0x2UL << AES_CR_NPBLB_Pos)                    /*!< 0x00200000 */
#define AES_CR_NPBLB_2           (0x4UL << AES_CR_NPBLB_Pos)                    /*!< 0x00400000 */
#define AES_CR_NPBLB_3           (0x8UL << AES_CR_NPBLB_Pos)                    /*!< 0x00800000 */

#define AES_CR_GCMPH_Pos         (13U)
#define AES_CR_GCMPH_Msk         (0x3UL << AES_CR_GCMPH_Pos)                    /*!< 0x00006000 */
#define AES_CR_GCMPH             AES_CR_GCMPH_Msk                               /*!< GCM Phase */
#define AES_CR_GCMPH_0           (0x1UL << AES_CR_GCMPH_Pos)                    /*!< 0x00002000 */
#define AES_CR_GCMPH_1           (0x2UL << AES_CR_GCMPH_Pos)                    /*!< 0x00004000 */

#define AES_CR_KEYSIZE_Pos       (18U)
#define AES_CR_KEYSIZE_Msk       (0x1UL << AES_CR_KEYSIZE_Pos)                  /*!< 0x00040000 */
#define AES_CR_KEYSIZE           AES_CR_KEYSIZE_Msk                             /*!< Key size selection */

/*******************  Bit definition for AES_SR register  *********************/
#define AES_SR_CCF_Pos           (0U)
#define AES_SR_CCF_Msk           (0x1UL << AES_SR_CCF_Pos)                      /*!< 0x00000001 */
#define AES_SR_CCF               AES_SR_CCF_Msk                                 /*!< Computation Complete Flag */
#define AES_SR_RDERR_Pos         (1U)
#define AES_SR_RDERR_Msk         (0x1UL << AES_SR_RDERR_Pos)                    /*!< 0x00000002 */
#define AES_SR_RDERR             AES_SR_RDERR_Msk                               /*!< Read Error Flag */
#define AES_SR_WRERR_Pos         (2U)
#define AES_SR_WRERR_Msk         (0x1UL << AES_SR_WRERR_Pos)                    /*!< 0x00000004 */
#define AES_SR_WRERR             AES_SR_WRERR_Msk                               /*!< Write Error Flag */
#define AES_SR_BUSY_Pos          (3U)
#define AES_SR_BUSY_Msk          (0x1UL << AES_SR_BUSY_Pos)                     /*!< 0x00000008 */
#define AES_SR_BUSY              AES_SR_BUSY_Msk                                /*!< Busy Flag */

/*******************  Bit definition for AES_DINR register  *******************/
#define AES_DINR_Pos             (0U)
#define AES_DINR_Msk             (0xFFFFFFFFUL << AES_DINR_Pos)                 /*!< 0xFFFFFFFF */
#define AES_DINR                 AES_DINR_Msk                                   /*!< AES Data Input Register */

/*******************  Bit definition for AES_DOUTR register  ******************/
#define AES_DOUTR_Pos            (0U)
#define AES_DOUTR_Msk            (0xFFFFFFFFUL << AES_DOUTR_Pos)                /*!< 0xFFFFFFFF */
#define AES_DOUTR                AES_DOUTR_Msk                                  /*!< AES Data Output Register */

/*******************  Bit definition for AES_KEYR0 register  ******************/
#define AES_KEYR0_Pos            (0U)
#define AES_KEYR0_Msk            (0xFFFFFFFFUL << AES_KEYR0_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR0                AES_KEYR0_Msk                                  /*!< AES Key Register 0 */

/*******************  Bit definition for AES_KEYR1 register  ******************/
#define AES_KEYR1_Pos            (0U)
#define AES_KEYR1_Msk            (0xFFFFFFFFUL << AES_KEYR1_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR1                AES_KEYR1_Msk                                  /*!< AES Key Register 1 */

/*******************  Bit definition for AES_KEYR2 register  ******************/
#define AES_KEYR2_Pos            (0U)
#define AES_KEYR2_Msk            (0xFFFFFFFFUL << AES_KEYR2_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR2                AES_KEYR2_Msk                                  /*!< AES Key Register 2 */

/*******************  Bit definition for AES_KEYR3 register  ******************/
#define AES_KEYR3_Pos            (0U)
#define AES_KEYR3_Msk            (0xFFFFFFFFUL << AES_KEYR3_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR3                AES_KEYR3_Msk                                  /*!< AES Key Register 3 */

/*******************  Bit definition for AES_KEYR4 register  ******************/
#define AES_KEYR4_Pos            (0U)
#define AES_KEYR4_Msk            (0xFFFFFFFFUL << AES_KEYR4_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR4                AES_KEYR4_Msk                                  /*!< AES Key Register 4 */

/*******************  Bit definition for AES_KEYR5 register  ******************/
#define AES_KEYR5_Pos            (0U)
#define AES_KEYR5_Msk            (0xFFFFFFFFUL << AES_KEYR5_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR5                AES_KEYR5_Msk                                  /*!< AES Key Register 5 */

/*******************  Bit definition for AES_KEYR6 register  ******************/
#define AES_KEYR6_Pos            (0U)
#define AES_KEYR6_Msk            (0xFFFFFFFFUL << AES_KEYR6_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR6                AES_KEYR6_Msk                                  /*!< AES Key Register 6 */

/*******************  Bit definition for AES_KEYR7 register  ******************/
#define AES_KEYR7_Pos            (0U)
#define AES_KEYR7_Msk            (0xFFFFFFFFUL << AES_KEYR7_Pos)                /*!< 0xFFFFFFFF */
#define AES_KEYR7                AES_KEYR7_Msk                                  /*!< AES Key Register 7 */

/*******************  Bit definition for AES_IVR0 register   ******************/
#define AES_IVR0_Pos             (0U)
#define AES_IVR0_Msk             (0xFFFFFFFFUL << AES_IVR0_Pos)                 /*!< 0xFFFFFFFF */
#define AES_IVR0                 AES_IVR0_Msk                                   /*!< AES Initialization Vector Register 0 */

/*******************  Bit definition for AES_IVR1 register   ******************/
#define AES_IVR1_Pos             (0U)
#define AES_IVR1_Msk             (0xFFFFFFFFUL << AES_IVR1_Pos)                 /*!< 0xFFFFFFFF */
#define AES_IVR1                 AES_IVR1_Msk                                   /*!< AES Initialization Vector Register 1 */

/*******************  Bit definition for AES_IVR2 register   ******************/
#define AES_IVR2_Pos             (0U)
#define AES_IVR2_Msk             (0xFFFFFFFFUL << AES_IVR2_Pos)                /*!< 0xFFFFFFFF */
#define AES_IVR2                 AES_IVR2_Msk                                  /*!< AES Initialization Vector Register 2 */

/*******************  Bit definition for AES_IVR3 register   ******************/
#define AES_IVR3_Pos             (0U)
#define AES_IVR3_Msk             (0xFFFFFFFFUL << AES_IVR3_Pos)                /*!< 0xFFFFFFFF */
#define AES_IVR3                 AES_IVR3_Msk                                  /*!< AES Initialization Vector Register 3 */

/*******************  Bit definition for AES_SUSP0R register  ******************/
#define AES_SUSP0R_Pos           (0U)
#define AES_SUSP0R_Msk           (0xFFFFFFFFUL << AES_SUSP0R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP0R               AES_SUSP0R_Msk                                /*!< AES Suspend registers 0 */

/*******************  Bit definition for AES_SUSP1R register  ******************/
#define AES_SUSP1R_Pos           (0U)
#define AES_SUSP1R_Msk           (0xFFFFFFFFUL << AES_SUSP1R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP1R               AES_SUSP1R_Msk                                /*!< AES Suspend registers 1 */

/*******************  Bit definition for AES_SUSP2R register  ******************/
#define AES_SUSP2R_Pos           (0U)
#define AES_SUSP2R_Msk           (0xFFFFFFFFUL << AES_SUSP2R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP2R               AES_SUSP2R_Msk                                /*!< AES Suspend registers 2 */

/*******************  Bit definition for AES_SUSP3R register  ******************/
#define AES_SUSP3R_Pos           (0U)
#define AES_SUSP3R_Msk           (0xFFFFFFFFUL << AES_SUSP3R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP3R               AES_SUSP3R_Msk                                /*!< AES Suspend registers 3 */

/*******************  Bit definition for AES_SUSP4R register  ******************/
#define AES_SUSP4R_Pos           (0U)
#define AES_SUSP4R_Msk           (0xFFFFFFFFUL << AES_SUSP4R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP4R               AES_SUSP4R_Msk                                /*!< AES Suspend registers 4 */

/*******************  Bit definition for AES_SUSP5R register  ******************/
#define AES_SUSP5R_Pos           (0U)
#define AES_SUSP5R_Msk           (0xFFFFFFFFUL << AES_SUSP5R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP5R               AES_SUSP5R_Msk                                /*!< AES Suspend registers 5 */

/*******************  Bit definition for AES_SUSP6R register  ******************/
#define AES_SUSP6R_Pos           (0U)
#define AES_SUSP6R_Msk           (0xFFFFFFFFUL << AES_SUSP6R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP6R               AES_SUSP6R_Msk                                /*!< AES Suspend registers 6 */

/*******************  Bit definition for AES_SUSP7R register  ******************/
#define AES_SUSP7R_Pos           (0U)
#define AES_SUSP7R_Msk           (0xFFFFFFFFUL << AES_SUSP7R_Pos)              /*!< 0xFFFFFFFF */
#define AES_SUSP7R               AES_SUSP7R_Msk                                /*!< AES Suspend registers 7 */


/******************************************************************************/
/*                                                                            */
/*                      Digital to Analog Converter                           */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions
*/
#define DAC_ADDITIONAL_TRIGGERS_SUPPORT

/********************  Bit definition for DAC_CR register  ********************/
#define DAC_CR_EN1_Pos              (0U)
#define DAC_CR_EN1_Msk              (0x1UL << DAC_CR_EN1_Pos)                  /*!< 0x00000001 */
#define DAC_CR_EN1                  DAC_CR_EN1_Msk                             /*!<DAC channel1 enable */
#define DAC_CR_TEN1_Pos             (1U)
#define DAC_CR_TEN1_Msk             (0x1UL << DAC_CR_TEN1_Pos)                 /*!< 0x00000002 */
#define DAC_CR_TEN1                 DAC_CR_TEN1_Msk                            /*!<DAC channel1 Trigger enable */

#define DAC_CR_TSEL1_Pos            (2U)
#define DAC_CR_TSEL1_Msk            (0xFUL << DAC_CR_TSEL1_Pos)                /*!< 0x0000003C */
#define DAC_CR_TSEL1                DAC_CR_TSEL1_Msk                           /*!<TSEL1[3:0] (DAC channel1 Trigger selection) */
#define DAC_CR_TSEL1_0              (0x1UL << DAC_CR_TSEL1_Pos)                /*!< 0x00000004 */
#define DAC_CR_TSEL1_1              (0x2UL << DAC_CR_TSEL1_Pos)                /*!< 0x00000008 */
#define DAC_CR_TSEL1_2              (0x4UL << DAC_CR_TSEL1_Pos)                /*!< 0x00000010 */
#define DAC_CR_TSEL1_3              (0x8UL << DAC_CR_TSEL1_Pos)                /*!< 0x00000020 */

#define DAC_CR_WAVE1_Pos            (6U)
#define DAC_CR_WAVE1_Msk            (0x3UL << DAC_CR_WAVE1_Pos)                /*!< 0x000000C0 */
#define DAC_CR_WAVE1                DAC_CR_WAVE1_Msk                           /*!<WAVE1[1:0] (DAC channel1 noise/triangle wave generation enable) */
#define DAC_CR_WAVE1_0              (0x1UL << DAC_CR_WAVE1_Pos)                /*!< 0x00000040 */
#define DAC_CR_WAVE1_1              (0x2UL << DAC_CR_WAVE1_Pos)                /*!< 0x00000080 */

#define DAC_CR_MAMP1_Pos            (8U)
#define DAC_CR_MAMP1_Msk            (0xFUL << DAC_CR_MAMP1_Pos)                /*!< 0x00000F00 */
#define DAC_CR_MAMP1                DAC_CR_MAMP1_Msk                           /*!<MAMP1[3:0] (DAC channel1 Mask/Amplitude selector) */
#define DAC_CR_MAMP1_0              (0x1UL << DAC_CR_MAMP1_Pos)                /*!< 0x00000100 */
#define DAC_CR_MAMP1_1              (0x2UL << DAC_CR_MAMP1_Pos)                /*!< 0x00000200 */
#define DAC_CR_MAMP1_2              (0x4UL << DAC_CR_MAMP1_Pos)                /*!< 0x00000400 */
#define DAC_CR_MAMP1_3              (0x8UL << DAC_CR_MAMP1_Pos)                /*!< 0x00000800 */

#define DAC_CR_DMAEN1_Pos           (12U)
#define DAC_CR_DMAEN1_Msk           (0x1UL << DAC_CR_DMAEN1_Pos)               /*!< 0x00001000 */
#define DAC_CR_DMAEN1               DAC_CR_DMAEN1_Msk                          /*!<DAC channel1 DMA enable */
#define DAC_CR_DMAUDRIE1_Pos        (13U)
#define DAC_CR_DMAUDRIE1_Msk        (0x1UL << DAC_CR_DMAUDRIE1_Pos)            /*!< 0x00002000 */
#define DAC_CR_DMAUDRIE1            DAC_CR_DMAUDRIE1_Msk                       /*!<DAC channel 1 DMA underrun interrupt enable  >*/
#define DAC_CR_CEN1_Pos             (14U)
#define DAC_CR_CEN1_Msk             (0x1UL << DAC_CR_CEN1_Pos)                 /*!< 0x00004000 */
#define DAC_CR_CEN1                 DAC_CR_CEN1_Msk                            /*!<DAC channel 1 calibration enable >*/

#define DAC_CR_EN2_Pos              (16U)
#define DAC_CR_EN2_Msk              (0x1UL << DAC_CR_EN2_Pos)                  /*!< 0x00010000 */
#define DAC_CR_EN2                  DAC_CR_EN2_Msk                             /*!<DAC channel2 enable */
#define DAC_CR_TEN2_Pos             (17U)
#define DAC_CR_TEN2_Msk             (0x1UL << DAC_CR_TEN2_Pos)                 /*!< 0x00020000 */
#define DAC_CR_TEN2                 DAC_CR_TEN2_Msk                            /*!<DAC channel2 Trigger enable */

#define DAC_CR_TSEL2_Pos            (18U)
#define DAC_CR_TSEL2_Msk            (0xFUL << DAC_CR_TSEL2_Pos)                /*!< 0x003C0000 */
#define DAC_CR_TSEL2                DAC_CR_TSEL2_Msk                           /*!<TSEL2[2:0] (DAC channel2 Trigger selection) */
#define DAC_CR_TSEL2_0              (0x1UL << DAC_CR_TSEL2_Pos)                /*!< 0x00040000 */
#define DAC_CR_TSEL2_1              (0x2UL << DAC_CR_TSEL2_Pos)                /*!< 0x00080000 */
#define DAC_CR_TSEL2_2              (0x4UL << DAC_CR_TSEL2_Pos)                /*!< 0x00100000 */
#define DAC_CR_TSEL2_3              (0x8UL << DAC_CR_TSEL2_Pos)                /*!< 0x00200000 */

#define DAC_CR_WAVE2_Pos            (22U)
#define DAC_CR_WAVE2_Msk            (0x3UL << DAC_CR_WAVE2_Pos)                /*!< 0x00C00000 */
#define DAC_CR_WAVE2                DAC_CR_WAVE2_Msk                           /*!<WAVE2[1:0] (DAC channel2 noise/triangle wave generation enable) */
#define DAC_CR_WAVE2_0              (0x1UL << DAC_CR_WAVE2_Pos)                /*!< 0x00400000 */
#define DAC_CR_WAVE2_1              (0x2UL << DAC_CR_WAVE2_Pos)                /*!< 0x00800000 */

#define DAC_CR_MAMP2_Pos            (24U)
#define DAC_CR_MAMP2_Msk            (0xFUL << DAC_CR_MAMP2_Pos)                /*!< 0x0F000000 */
#define DAC_CR_MAMP2                DAC_CR_MAMP2_Msk                           /*!<MAMP2[3:0] (DAC channel2 Mask/Amplitude selector) */
#define DAC_CR_MAMP2_0              (0x1UL << DAC_CR_MAMP2_Pos)                /*!< 0x01000000 */
#define DAC_CR_MAMP2_1              (0x2UL << DAC_CR_MAMP2_Pos)                /*!< 0x02000000 */
#define DAC_CR_MAMP2_2              (0x4UL << DAC_CR_MAMP2_Pos)                /*!< 0x04000000 */
#define DAC_CR_MAMP2_3              (0x8UL << DAC_CR_MAMP2_Pos)                /*!< 0x08000000 */

#define DAC_CR_DMAEN2_Pos           (28U)
#define DAC_CR_DMAEN2_Msk           (0x1UL << DAC_CR_DMAEN2_Pos)               /*!< 0x10000000 */
#define DAC_CR_DMAEN2               DAC_CR_DMAEN2_Msk                          /*!<DAC channel2 DMA enabled */
#define DAC_CR_DMAUDRIE2_Pos        (29U)
#define DAC_CR_DMAUDRIE2_Msk        (0x1UL << DAC_CR_DMAUDRIE2_Pos)            /*!< 0x20000000 */
#define DAC_CR_DMAUDRIE2            DAC_CR_DMAUDRIE2_Msk                       /*!<DAC channel2 DMA underrun interrupt enable  >*/
#define DAC_CR_CEN2_Pos             (30U)
#define DAC_CR_CEN2_Msk             (0x1UL << DAC_CR_CEN2_Pos)                 /*!< 0x40000000 */
#define DAC_CR_CEN2                 DAC_CR_CEN2_Msk                            /*!<DAC channel2 calibration enable >*/

/*****************  Bit definition for DAC_SWTRIGR register  ******************/
#define DAC_SWTRIGR_SWTRIG1_Pos     (0U)
#define DAC_SWTRIGR_SWTRIG1_Msk     (0x1UL << DAC_SWTRIGR_SWTRIG1_Pos)         /*!< 0x00000001 */
#define DAC_SWTRIGR_SWTRIG1         DAC_SWTRIGR_SWTRIG1_Msk                    /*!<DAC channel1 software trigger */
#define DAC_SWTRIGR_SWTRIG2_Pos     (1U)
#define DAC_SWTRIGR_SWTRIG2_Msk     (0x1UL << DAC_SWTRIGR_SWTRIG2_Pos)         /*!< 0x00000002 */
#define DAC_SWTRIGR_SWTRIG2         DAC_SWTRIGR_SWTRIG2_Msk                    /*!<DAC channel2 software trigger */

/*****************  Bit definition for DAC_DHR12R1 register  ******************/
#define DAC_DHR12R1_DACC1DHR_Pos    (0U)
#define DAC_DHR12R1_DACC1DHR_Msk    (0xFFFUL << DAC_DHR12R1_DACC1DHR_Pos)      /*!< 0x00000FFF */
#define DAC_DHR12R1_DACC1DHR        DAC_DHR12R1_DACC1DHR_Msk                   /*!<DAC channel1 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12L1 register  ******************/
#define DAC_DHR12L1_DACC1DHR_Pos    (4U)
#define DAC_DHR12L1_DACC1DHR_Msk    (0xFFFUL << DAC_DHR12L1_DACC1DHR_Pos)      /*!< 0x0000FFF0 */
#define DAC_DHR12L1_DACC1DHR        DAC_DHR12L1_DACC1DHR_Msk                   /*!<DAC channel1 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8R1 register  ******************/
#define DAC_DHR8R1_DACC1DHR_Pos     (0U)
#define DAC_DHR8R1_DACC1DHR_Msk     (0xFFUL << DAC_DHR8R1_DACC1DHR_Pos)        /*!< 0x000000FF */
#define DAC_DHR8R1_DACC1DHR         DAC_DHR8R1_DACC1DHR_Msk                    /*!<DAC channel1 8-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12R2 register  ******************/
#define DAC_DHR12R2_DACC2DHR_Pos    (0U)
#define DAC_DHR12R2_DACC2DHR_Msk    (0xFFFUL << DAC_DHR12R2_DACC2DHR_Pos)      /*!< 0x00000FFF */
#define DAC_DHR12R2_DACC2DHR        DAC_DHR12R2_DACC2DHR_Msk                   /*!<DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12L2 register  ******************/
#define DAC_DHR12L2_DACC2DHR_Pos    (4U)
#define DAC_DHR12L2_DACC2DHR_Msk    (0xFFFUL << DAC_DHR12L2_DACC2DHR_Pos)      /*!< 0x0000FFF0 */
#define DAC_DHR12L2_DACC2DHR        DAC_DHR12L2_DACC2DHR_Msk                   /*!<DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8R2 register  ******************/
#define DAC_DHR8R2_DACC2DHR_Pos     (0U)
#define DAC_DHR8R2_DACC2DHR_Msk     (0xFFUL << DAC_DHR8R2_DACC2DHR_Pos)        /*!< 0x000000FF */
#define DAC_DHR8R2_DACC2DHR         DAC_DHR8R2_DACC2DHR_Msk                    /*!<DAC channel2 8-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12RD register  ******************/
#define DAC_DHR12RD_DACC1DHR_Pos    (0U)
#define DAC_DHR12RD_DACC1DHR_Msk    (0xFFFUL << DAC_DHR12RD_DACC1DHR_Pos)      /*!< 0x00000FFF */
#define DAC_DHR12RD_DACC1DHR        DAC_DHR12RD_DACC1DHR_Msk                   /*!<DAC channel1 12-bit Right aligned data */
#define DAC_DHR12RD_DACC2DHR_Pos    (16U)
#define DAC_DHR12RD_DACC2DHR_Msk    (0xFFFUL << DAC_DHR12RD_DACC2DHR_Pos)      /*!< 0x0FFF0000 */
#define DAC_DHR12RD_DACC2DHR        DAC_DHR12RD_DACC2DHR_Msk                   /*!<DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12LD register  ******************/
#define DAC_DHR12LD_DACC1DHR_Pos    (4U)
#define DAC_DHR12LD_DACC1DHR_Msk    (0xFFFUL << DAC_DHR12LD_DACC1DHR_Pos)      /*!< 0x0000FFF0 */
#define DAC_DHR12LD_DACC1DHR        DAC_DHR12LD_DACC1DHR_Msk                   /*!<DAC channel1 12-bit Left aligned data */
#define DAC_DHR12LD_DACC2DHR_Pos    (20U)
#define DAC_DHR12LD_DACC2DHR_Msk    (0xFFFUL << DAC_DHR12LD_DACC2DHR_Pos)      /*!< 0xFFF00000 */
#define DAC_DHR12LD_DACC2DHR        DAC_DHR12LD_DACC2DHR_Msk                   /*!<DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8RD register  ******************/
#define DAC_DHR8RD_DACC1DHR_Pos     (0U)
#define DAC_DHR8RD_DACC1DHR_Msk     (0xFFUL << DAC_DHR8RD_DACC1DHR_Pos)        /*!< 0x000000FF */
#define DAC_DHR8RD_DACC1DHR         DAC_DHR8RD_DACC1DHR_Msk                    /*!<DAC channel1 8-bit Right aligned data */
#define DAC_DHR8RD_DACC2DHR_Pos     (8U)
#define DAC_DHR8RD_DACC2DHR_Msk     (0xFFUL << DAC_DHR8RD_DACC2DHR_Pos)        /*!< 0x0000FF00 */
#define DAC_DHR8RD_DACC2DHR         DAC_DHR8RD_DACC2DHR_Msk                    /*!<DAC channel2 8-bit Right aligned data */

/*******************  Bit definition for DAC_DOR1 register  *******************/
#define DAC_DOR1_DACC1DOR_Pos       (0U)
#define DAC_DOR1_DACC1DOR_Msk       (0xFFFUL << DAC_DOR1_DACC1DOR_Pos)         /*!< 0x00000FFF */
#define DAC_DOR1_DACC1DOR           DAC_DOR1_DACC1DOR_Msk                      /*!<DAC channel1 data output */

/*******************  Bit definition for DAC_DOR2 register  *******************/
#define DAC_DOR2_DACC2DOR_Pos       (0U)
#define DAC_DOR2_DACC2DOR_Msk       (0xFFFUL << DAC_DOR2_DACC2DOR_Pos)         /*!< 0x00000FFF */
#define DAC_DOR2_DACC2DOR           DAC_DOR2_DACC2DOR_Msk                      /*!<DAC channel2 data output */

/********************  Bit definition for DAC_SR register  ********************/
#define DAC_SR_DMAUDR1_Pos          (13U)
#define DAC_SR_DMAUDR1_Msk          (0x1UL << DAC_SR_DMAUDR1_Pos)              /*!< 0x00002000 */
#define DAC_SR_DMAUDR1              DAC_SR_DMAUDR1_Msk                         /*!<DAC channel1 DMA underrun flag */
#define DAC_SR_CAL_FLAG1_Pos        (14U)
#define DAC_SR_CAL_FLAG1_Msk        (0x1UL << DAC_SR_CAL_FLAG1_Pos)            /*!< 0x00004000 */
#define DAC_SR_CAL_FLAG1            DAC_SR_CAL_FLAG1_Msk                       /*!<DAC channel1 calibration offset status */
#define DAC_SR_BWST1_Pos            (15U)
#define DAC_SR_BWST1_Msk            (0x1UL << DAC_SR_BWST1_Pos)                /*!< 0x00008000 */
#define DAC_SR_BWST1                DAC_SR_BWST1_Msk                           /*!<DAC channel1 busy writing sample time flag */

#define DAC_SR_DMAUDR2_Pos          (29U)
#define DAC_SR_DMAUDR2_Msk          (0x1UL << DAC_SR_DMAUDR2_Pos)              /*!< 0x20000000 */
#define DAC_SR_DMAUDR2              DAC_SR_DMAUDR2_Msk                         /*!<DAC channel2 DMA underrun flag */
#define DAC_SR_CAL_FLAG2_Pos        (30U)
#define DAC_SR_CAL_FLAG2_Msk        (0x1UL << DAC_SR_CAL_FLAG2_Pos)            /*!< 0x40000000 */
#define DAC_SR_CAL_FLAG2            DAC_SR_CAL_FLAG2_Msk                       /*!<DAC channel2 calibration offset status */
#define DAC_SR_BWST2_Pos            (31U)
#define DAC_SR_BWST2_Msk            (0x1UL << DAC_SR_BWST2_Pos)                /*!< 0x80000000 */
#define DAC_SR_BWST2                DAC_SR_BWST2_Msk                           /*!<DAC channel2 busy writing sample time flag */

/*******************  Bit definition for DAC_CCR register  ********************/
#define DAC_CCR_OTRIM1_Pos          (0U)
#define DAC_CCR_OTRIM1_Msk          (0x1FUL << DAC_CCR_OTRIM1_Pos)             /*!< 0x0000001F */
#define DAC_CCR_OTRIM1              DAC_CCR_OTRIM1_Msk                         /*!<DAC channel1 offset trimming value */
#define DAC_CCR_OTRIM2_Pos          (16U)
#define DAC_CCR_OTRIM2_Msk          (0x1FUL << DAC_CCR_OTRIM2_Pos)             /*!< 0x001F0000 */
#define DAC_CCR_OTRIM2              DAC_CCR_OTRIM2_Msk                         /*!<DAC channel2 offset trimming value */

/*******************  Bit definition for DAC_MCR register  *******************/
#define DAC_MCR_MODE1_Pos           (0U)
#define DAC_MCR_MODE1_Msk           (0x7UL << DAC_MCR_MODE1_Pos)               /*!< 0x00000007 */
#define DAC_MCR_MODE1               DAC_MCR_MODE1_Msk                          /*!<MODE1[2:0] (DAC channel1 mode) */
#define DAC_MCR_MODE1_0             (0x1UL << DAC_MCR_MODE1_Pos)               /*!< 0x00000001 */
#define DAC_MCR_MODE1_1             (0x2UL << DAC_MCR_MODE1_Pos)               /*!< 0x00000002 */
#define DAC_MCR_MODE1_2             (0x4UL << DAC_MCR_MODE1_Pos)               /*!< 0x00000004 */

#define DAC_MCR_MODE2_Pos           (16U)
#define DAC_MCR_MODE2_Msk           (0x7UL << DAC_MCR_MODE2_Pos)               /*!< 0x00070000 */
#define DAC_MCR_MODE2               DAC_MCR_MODE2_Msk                          /*!<MODE2[2:0] (DAC channel2 mode) */
#define DAC_MCR_MODE2_0             (0x1UL << DAC_MCR_MODE2_Pos)               /*!< 0x00010000 */
#define DAC_MCR_MODE2_1             (0x2UL << DAC_MCR_MODE2_Pos)               /*!< 0x00020000 */
#define DAC_MCR_MODE2_2             (0x4UL << DAC_MCR_MODE2_Pos)               /*!< 0x00040000 */

/******************  Bit definition for DAC_SHSR1 register  ******************/
#define DAC_SHSR1_TSAMPLE1_Pos      (0U)
#define DAC_SHSR1_TSAMPLE1_Msk      (0x3FFUL << DAC_SHSR1_TSAMPLE1_Pos)        /*!< 0x000003FF */
#define DAC_SHSR1_TSAMPLE1          DAC_SHSR1_TSAMPLE1_Msk                     /*!<DAC channel1 sample time */

/******************  Bit definition for DAC_SHSR2 register  ******************/
#define DAC_SHSR2_TSAMPLE2_Pos      (0U)
#define DAC_SHSR2_TSAMPLE2_Msk      (0x3FFUL << DAC_SHSR2_TSAMPLE2_Pos)        /*!< 0x000003FF */
#define DAC_SHSR2_TSAMPLE2          DAC_SHSR2_TSAMPLE2_Msk                     /*!<DAC channel2 sample time */

/******************  Bit definition for DAC_SHHR register  ******************/
#define DAC_SHHR_THOLD1_Pos         (0U)
#define DAC_SHHR_THOLD1_Msk         (0x3FFUL << DAC_SHHR_THOLD1_Pos)           /*!< 0x000003FF */
#define DAC_SHHR_THOLD1             DAC_SHHR_THOLD1_Msk                        /*!<DAC channel1 hold time */
#define DAC_SHHR_THOLD2_Pos         (16U)
#define DAC_SHHR_THOLD2_Msk         (0x3FFUL << DAC_SHHR_THOLD2_Pos)           /*!< 0x03FF0000 */
#define DAC_SHHR_THOLD2             DAC_SHHR_THOLD2_Msk                        /*!<DAC channel2 hold time */

/******************  Bit definition for DAC_SHRR register  ******************/
#define DAC_SHRR_TREFRESH1_Pos      (0U)
#define DAC_SHRR_TREFRESH1_Msk      (0xFFUL << DAC_SHRR_TREFRESH1_Pos)         /*!< 0x000000FF */
#define DAC_SHRR_TREFRESH1          DAC_SHRR_TREFRESH1_Msk                     /*!<DAC channel1 refresh time */
#define DAC_SHRR_TREFRESH2_Pos      (16U)
#define DAC_SHRR_TREFRESH2_Msk      (0xFFUL << DAC_SHRR_TREFRESH2_Pos)         /*!< 0x00FF0000 */
#define DAC_SHRR_TREFRESH2          DAC_SHRR_TREFRESH2_Msk                     /*!<DAC channel2 refresh time */


/******************************************************************************/
/*                                                                            */
/*                                 Debug MCU                                  */
/*                                                                            */
/******************************************************************************/

/******************************************************************************/
/*                                                                            */
/*                           DMA Controller (DMA)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for DMA_ISR register  ********************/
#define DMA_ISR_GIF1_Pos       (0U)
#define DMA_ISR_GIF1_Msk       (0x1UL << DMA_ISR_GIF1_Pos)                     /*!< 0x00000001 */
#define DMA_ISR_GIF1           DMA_ISR_GIF1_Msk                                /*!< Channel 1 Global interrupt flag */
#define DMA_ISR_TCIF1_Pos      (1U)
#define DMA_ISR_TCIF1_Msk      (0x1UL << DMA_ISR_TCIF1_Pos)                    /*!< 0x00000002 */
#define DMA_ISR_TCIF1          DMA_ISR_TCIF1_Msk                               /*!< Channel 1 Transfer Complete flag */
#define DMA_ISR_HTIF1_Pos      (2U)
#define DMA_ISR_HTIF1_Msk      (0x1UL << DMA_ISR_HTIF1_Pos)                    /*!< 0x00000004 */
#define DMA_ISR_HTIF1          DMA_ISR_HTIF1_Msk                               /*!< Channel 1 Half Transfer flag */
#define DMA_ISR_TEIF1_Pos      (3U)
#define DMA_ISR_TEIF1_Msk      (0x1UL << DMA_ISR_TEIF1_Pos)                    /*!< 0x00000008 */
#define DMA_ISR_TEIF1          DMA_ISR_TEIF1_Msk                               /*!< Channel 1 Transfer Error flag */
#define DMA_ISR_GIF2_Pos       (4U)
#define DMA_ISR_GIF2_Msk       (0x1UL << DMA_ISR_GIF2_Pos)                     /*!< 0x00000010 */
#define DMA_ISR_GIF2           DMA_ISR_GIF2_Msk                                /*!< Channel 2 Global interrupt flag */
#define DMA_ISR_TCIF2_Pos      (5U)
#define DMA_ISR_TCIF2_Msk      (0x1UL << DMA_ISR_TCIF2_Pos)                    /*!< 0x00000020 */
#define DMA_ISR_TCIF2          DMA_ISR_TCIF2_Msk                               /*!< Channel 2 Transfer Complete flag */
#define DMA_ISR_HTIF2_Pos      (6U)
#define DMA_ISR_HTIF2_Msk      (0x1UL << DMA_ISR_HTIF2_Pos)                    /*!< 0x00000040 */
#define DMA_ISR_HTIF2          DMA_ISR_HTIF2_Msk                               /*!< Channel 2 Half Transfer flag */
#define DMA_ISR_TEIF2_Pos      (7U)
#define DMA_ISR_TEIF2_Msk      (0x1UL << DMA_ISR_TEIF2_Pos)                    /*!< 0x00000080 */
#define DMA_ISR_TEIF2          DMA_ISR_TEIF2_Msk                               /*!< Channel 2 Transfer Error flag */
#define DMA_ISR_GIF3_Pos       (8U)
#define DMA_ISR_GIF3_Msk       (0x1UL << DMA_ISR_GIF3_Pos)                     /*!< 0x00000100 */
#define DMA_ISR_GIF3           DMA_ISR_GIF3_Msk                                /*!< Channel 3 Global interrupt flag */
#define DMA_ISR_TCIF3_Pos      (9U)
#define DMA_ISR_TCIF3_Msk      (0x1UL << DMA_ISR_TCIF3_Pos)                    /*!< 0x00000200 */
#define DMA_ISR_TCIF3          DMA_ISR_TCIF3_Msk                               /*!< Channel 3 Transfer Complete flag */
#define DMA_ISR_HTIF3_Pos      (10U)
#define DMA_ISR_HTIF3_Msk      (0x1UL << DMA_ISR_HTIF3_Pos)                    /*!< 0x00000400 */
#define DMA_ISR_HTIF3          DMA_ISR_HTIF3_Msk                               /*!< Channel 3 Half Transfer flag */
#define DMA_ISR_TEIF3_Pos      (11U)
#define DMA_ISR_TEIF3_Msk      (0x1UL << DMA_ISR_TEIF3_Pos)                    /*!< 0x00000800 */
#define DMA_ISR_TEIF3          DMA_ISR_TEIF3_Msk                               /*!< Channel 3 Transfer Error flag */
#define DMA_ISR_GIF4_Pos       (12U)
#define DMA_ISR_GIF4_Msk       (0x1UL << DMA_ISR_GIF4_Pos)                     /*!< 0x00001000 */
#define DMA_ISR_GIF4           DMA_ISR_GIF4_Msk                                /*!< Channel 4 Global interrupt flag */
#define DMA_ISR_TCIF4_Pos      (13U)
#define DMA_ISR_TCIF4_Msk      (0x1UL << DMA_ISR_TCIF4_Pos)                    /*!< 0x00002000 */
#define DMA_ISR_TCIF4          DMA_ISR_TCIF4_Msk                               /*!< Channel 4 Transfer Complete flag */
#define DMA_ISR_HTIF4_Pos      (14U)
#define DMA_ISR_HTIF4_Msk      (0x1UL << DMA_ISR_HTIF4_Pos)                    /*!< 0x00004000 */
#define DMA_ISR_HTIF4          DMA_ISR_HTIF4_Msk                               /*!< Channel 4 Half Transfer flag */
#define DMA_ISR_TEIF4_Pos      (15U)
#define DMA_ISR_TEIF4_Msk      (0x1UL << DMA_ISR_TEIF4_Pos)                    /*!< 0x00008000 */
#define DMA_ISR_TEIF4          DMA_ISR_TEIF4_Msk                               /*!< Channel 4 Transfer Error flag */
#define DMA_ISR_GIF5_Pos       (16U)
#define DMA_ISR_GIF5_Msk       (0x1UL << DMA_ISR_GIF5_Pos)                     /*!< 0x00010000 */
#define DMA_ISR_GIF5           DMA_ISR_GIF5_Msk                                /*!< Channel 5 Global interrupt flag */
#define DMA_ISR_TCIF5_Pos      (17U)
#define DMA_ISR_TCIF5_Msk      (0x1UL << DMA_ISR_TCIF5_Pos)                    /*!< 0x00020000 */
#define DMA_ISR_TCIF5          DMA_ISR_TCIF5_Msk                               /*!< Channel 5 Transfer Complete flag */
#define DMA_ISR_HTIF5_Pos      (18U)
#define DMA_ISR_HTIF5_Msk      (0x1UL << DMA_ISR_HTIF5_Pos)                    /*!< 0x00040000 */
#define DMA_ISR_HTIF5          DMA_ISR_HTIF5_Msk                               /*!< Channel 5 Half Transfer flag */
#define DMA_ISR_TEIF5_Pos      (19U)
#define DMA_ISR_TEIF5_Msk      (0x1UL << DMA_ISR_TEIF5_Pos)                    /*!< 0x00080000 */
#define DMA_ISR_TEIF5          DMA_ISR_TEIF5_Msk                               /*!< Channel 5 Transfer Error flag */
#define DMA_ISR_GIF6_Pos       (20U)
#define DMA_ISR_GIF6_Msk       (0x1UL << DMA_ISR_GIF6_Pos)                     /*!< 0x00100000 */
#define DMA_ISR_GIF6           DMA_ISR_GIF6_Msk                                /*!< Channel 6 Global interrupt flag */
#define DMA_ISR_TCIF6_Pos      (21U)
#define DMA_ISR_TCIF6_Msk      (0x1UL << DMA_ISR_TCIF6_Pos)                    /*!< 0x00200000 */
#define DMA_ISR_TCIF6          DMA_ISR_TCIF6_Msk                               /*!< Channel 6 Transfer Complete flag */
#define DMA_ISR_HTIF6_Pos      (22U)
#define DMA_ISR_HTIF6_Msk      (0x1UL << DMA_ISR_HTIF6_Pos)                    /*!< 0x00400000 */
#define DMA_ISR_HTIF6          DMA_ISR_HTIF6_Msk                               /*!< Channel 6 Half Transfer flag */
#define DMA_ISR_TEIF6_Pos      (23U)
#define DMA_ISR_TEIF6_Msk      (0x1UL << DMA_ISR_TEIF6_Pos)                    /*!< 0x00800000 */
#define DMA_ISR_TEIF6          DMA_ISR_TEIF6_Msk                               /*!< Channel 6 Transfer Error flag */
#define DMA_ISR_GIF7_Pos       (24U)
#define DMA_ISR_GIF7_Msk       (0x1UL << DMA_ISR_GIF7_Pos)                     /*!< 0x01000000 */
#define DMA_ISR_GIF7           DMA_ISR_GIF7_Msk                                /*!< Channel 7 Global interrupt flag */
#define DMA_ISR_TCIF7_Pos      (25U)
#define DMA_ISR_TCIF7_Msk      (0x1UL << DMA_ISR_TCIF7_Pos)                    /*!< 0x02000000 */
#define DMA_ISR_TCIF7          DMA_ISR_TCIF7_Msk                               /*!< Channel 7 Transfer Complete flag */
#define DMA_ISR_HTIF7_Pos      (26U)
#define DMA_ISR_HTIF7_Msk      (0x1UL << DMA_ISR_HTIF7_Pos)                    /*!< 0x04000000 */
#define DMA_ISR_HTIF7          DMA_ISR_HTIF7_Msk                               /*!< Channel 7 Half Transfer flag */
#define DMA_ISR_TEIF7_Pos      (27U)
#define DMA_ISR_TEIF7_Msk      (0x1UL << DMA_ISR_TEIF7_Pos)                    /*!< 0x08000000 */
#define DMA_ISR_TEIF7          DMA_ISR_TEIF7_Msk                               /*!< Channel 7 Transfer Error flag */

/*******************  Bit definition for DMA_IFCR register  *******************/
#define DMA_IFCR_CGIF1_Pos     (0U)
#define DMA_IFCR_CGIF1_Msk     (0x1UL << DMA_IFCR_CGIF1_Pos)                   /*!< 0x00000001 */
#define DMA_IFCR_CGIF1         DMA_IFCR_CGIF1_Msk                              /*!< Channel 1 Global interrupt clearr */
#define DMA_IFCR_CTCIF1_Pos    (1U)
#define DMA_IFCR_CTCIF1_Msk    (0x1UL << DMA_IFCR_CTCIF1_Pos)                  /*!< 0x00000002 */
#define DMA_IFCR_CTCIF1        DMA_IFCR_CTCIF1_Msk                             /*!< Channel 1 Transfer Complete clear */
#define DMA_IFCR_CHTIF1_Pos    (2U)
#define DMA_IFCR_CHTIF1_Msk    (0x1UL << DMA_IFCR_CHTIF1_Pos)                  /*!< 0x00000004 */
#define DMA_IFCR_CHTIF1        DMA_IFCR_CHTIF1_Msk                             /*!< Channel 1 Half Transfer clear */
#define DMA_IFCR_CTEIF1_Pos    (3U)
#define DMA_IFCR_CTEIF1_Msk    (0x1UL << DMA_IFCR_CTEIF1_Pos)                  /*!< 0x00000008 */
#define DMA_IFCR_CTEIF1        DMA_IFCR_CTEIF1_Msk                             /*!< Channel 1 Transfer Error clear */
#define DMA_IFCR_CGIF2_Pos     (4U)
#define DMA_IFCR_CGIF2_Msk     (0x1UL << DMA_IFCR_CGIF2_Pos)                   /*!< 0x00000010 */
#define DMA_IFCR_CGIF2         DMA_IFCR_CGIF2_Msk                              /*!< Channel 2 Global interrupt clear */
#define DMA_IFCR_CTCIF2_Pos    (5U)
#define DMA_IFCR_CTCIF2_Msk    (0x1UL << DMA_IFCR_CTCIF2_Pos)                  /*!< 0x00000020 */
#define DMA_IFCR_CTCIF2        DMA_IFCR_CTCIF2_Msk                             /*!< Channel 2 Transfer Complete clear */
#define DMA_IFCR_CHTIF2_Pos    (6U)
#define DMA_IFCR_CHTIF2_Msk    (0x1UL << DMA_IFCR_CHTIF2_Pos)                  /*!< 0x00000040 */
#define DMA_IFCR_CHTIF2        DMA_IFCR_CHTIF2_Msk                             /*!< Channel 2 Half Transfer clear */
#define DMA_IFCR_CTEIF2_Pos    (7U)
#define DMA_IFCR_CTEIF2_Msk    (0x1UL << DMA_IFCR_CTEIF2_Pos)                  /*!< 0x00000080 */
#define DMA_IFCR_CTEIF2        DMA_IFCR_CTEIF2_Msk                             /*!< Channel 2 Transfer Error clear */
#define DMA_IFCR_CGIF3_Pos     (8U)
#define DMA_IFCR_CGIF3_Msk     (0x1UL << DMA_IFCR_CGIF3_Pos)                   /*!< 0x00000100 */
#define DMA_IFCR_CGIF3         DMA_IFCR_CGIF3_Msk                              /*!< Channel 3 Global interrupt clear */
#define DMA_IFCR_CTCIF3_Pos    (9U)
#define DMA_IFCR_CTCIF3_Msk    (0x1UL << DMA_IFCR_CTCIF3_Pos)                  /*!< 0x00000200 */
#define DMA_IFCR_CTCIF3        DMA_IFCR_CTCIF3_Msk                             /*!< Channel 3 Transfer Complete clear */
#define DMA_IFCR_CHTIF3_Pos    (10U)
#define DMA_IFCR_CHTIF3_Msk    (0x1UL << DMA_IFCR_CHTIF3_Pos)                  /*!< 0x00000400 */
#define DMA_IFCR_CHTIF3        DMA_IFCR_CHTIF3_Msk                             /*!< Channel 3 Half Transfer clear */
#define DMA_IFCR_CTEIF3_Pos    (11U)
#define DMA_IFCR_CTEIF3_Msk    (0x1UL << DMA_IFCR_CTEIF3_Pos)                  /*!< 0x00000800 */
#define DMA_IFCR_CTEIF3        DMA_IFCR_CTEIF3_Msk                             /*!< Channel 3 Transfer Error clear */
#define DMA_IFCR_CGIF4_Pos     (12U)
#define DMA_IFCR_CGIF4_Msk     (0x1UL << DMA_IFCR_CGIF4_Pos)                   /*!< 0x00001000 */
#define DMA_IFCR_CGIF4         DMA_IFCR_CGIF4_Msk                              /*!< Channel 4 Global interrupt clear */
#define DMA_IFCR_CTCIF4_Pos    (13U)
#define DMA_IFCR_CTCIF4_Msk    (0x1UL << DMA_IFCR_CTCIF4_Pos)                  /*!< 0x00002000 */
#define DMA_IFCR_CTCIF4        DMA_IFCR_CTCIF4_Msk                             /*!< Channel 4 Transfer Complete clear */
#define DMA_IFCR_CHTIF4_Pos    (14U)
#define DMA_IFCR_CHTIF4_Msk    (0x1UL << DMA_IFCR_CHTIF4_Pos)                  /*!< 0x00004000 */
#define DMA_IFCR_CHTIF4        DMA_IFCR_CHTIF4_Msk                             /*!< Channel 4 Half Transfer clear */
#define DMA_IFCR_CTEIF4_Pos    (15U)
#define DMA_IFCR_CTEIF4_Msk    (0x1UL << DMA_IFCR_CTEIF4_Pos)                  /*!< 0x00008000 */
#define DMA_IFCR_CTEIF4        DMA_IFCR_CTEIF4_Msk                             /*!< Channel 4 Transfer Error clear */
#define DMA_IFCR_CGIF5_Pos     (16U)
#define DMA_IFCR_CGIF5_Msk     (0x1UL << DMA_IFCR_CGIF5_Pos)                   /*!< 0x00010000 */
#define DMA_IFCR_CGIF5         DMA_IFCR_CGIF5_Msk                              /*!< Channel 5 Global interrupt clear */
#define DMA_IFCR_CTCIF5_Pos    (17U)
#define DMA_IFCR_CTCIF5_Msk    (0x1UL << DMA_IFCR_CTCIF5_Pos)                  /*!< 0x00020000 */
#define DMA_IFCR_CTCIF5        DMA_IFCR_CTCIF5_Msk                             /*!< Channel 5 Transfer Complete clear */
#define DMA_IFCR_CHTIF5_Pos    (18U)
#define DMA_IFCR_CHTIF5_Msk    (0x1UL << DMA_IFCR_CHTIF5_Pos)                  /*!< 0x00040000 */
#define DMA_IFCR_CHTIF5        DMA_IFCR_CHTIF5_Msk                             /*!< Channel 5 Half Transfer clear */
#define DMA_IFCR_CTEIF5_Pos    (19U)
#define DMA_IFCR_CTEIF5_Msk    (0x1UL << DMA_IFCR_CTEIF5_Pos)                  /*!< 0x00080000 */
#define DMA_IFCR_CTEIF5        DMA_IFCR_CTEIF5_Msk                             /*!< Channel 5 Transfer Error clear */
#define DMA_IFCR_CGIF6_Pos     (20U)
#define DMA_IFCR_CGIF6_Msk     (0x1UL << DMA_IFCR_CGIF6_Pos)                   /*!< 0x00100000 */
#define DMA_IFCR_CGIF6         DMA_IFCR_CGIF6_Msk                              /*!< Channel 6 Global interrupt clear */
#define DMA_IFCR_CTCIF6_Pos    (21U)
#define DMA_IFCR_CTCIF6_Msk    (0x1UL << DMA_IFCR_CTCIF6_Pos)                  /*!< 0x00200000 */
#define DMA_IFCR_CTCIF6        DMA_IFCR_CTCIF6_Msk                             /*!< Channel 6 Transfer Complete clear */
#define DMA_IFCR_CHTIF6_Pos    (22U)
#define DMA_IFCR_CHTIF6_Msk    (0x1UL << DMA_IFCR_CHTIF6_Pos)                  /*!< 0x00400000 */
#define DMA_IFCR_CHTIF6        DMA_IFCR_CHTIF6_Msk                             /*!< Channel 6 Half Transfer clear */
#define DMA_IFCR_CTEIF6_Pos    (23U)
#define DMA_IFCR_CTEIF6_Msk    (0x1UL << DMA_IFCR_CTEIF6_Pos)                  /*!< 0x00800000 */
#define DMA_IFCR_CTEIF6        DMA_IFCR_CTEIF6_Msk                             /*!< Channel 6 Transfer Error clear */
#define DMA_IFCR_CGIF7_Pos     (24U)
#define DMA_IFCR_CGIF7_Msk     (0x1UL << DMA_IFCR_CGIF7_Pos)                   /*!< 0x01000000 */
#define DMA_IFCR_CGIF7         DMA_IFCR_CGIF7_Msk                              /*!< Channel 7 Global interrupt clear */
#define DMA_IFCR_CTCIF7_Pos    (25U)
#define DMA_IFCR_CTCIF7_Msk    (0x1UL << DMA_IFCR_CTCIF7_Pos)                  /*!< 0x02000000 */
#define DMA_IFCR_CTCIF7        DMA_IFCR_CTCIF7_Msk                             /*!< Channel 7 Transfer Complete clear */
#define DMA_IFCR_CHTIF7_Pos    (26U)
#define DMA_IFCR_CHTIF7_Msk    (0x1UL << DMA_IFCR_CHTIF7_Pos)                  /*!< 0x04000000 */
#define DMA_IFCR_CHTIF7        DMA_IFCR_CHTIF7_Msk                             /*!< Channel 7 Half Transfer clear */
#define DMA_IFCR_CTEIF7_Pos    (27U)
#define DMA_IFCR_CTEIF7_Msk    (0x1UL << DMA_IFCR_CTEIF7_Pos)                  /*!< 0x08000000 */
#define DMA_IFCR_CTEIF7        DMA_IFCR_CTEIF7_Msk                             /*!< Channel 7 Transfer Error clear */

/*******************  Bit definition for DMA_CCR register  ********************/
#define DMA_CCR_EN_Pos         (0U)
#define DMA_CCR_EN_Msk         (0x1UL << DMA_CCR_EN_Pos)                       /*!< 0x00000001 */
#define DMA_CCR_EN             DMA_CCR_EN_Msk                                  /*!< Channel enable                      */
#define DMA_CCR_TCIE_Pos       (1U)
#define DMA_CCR_TCIE_Msk       (0x1UL << DMA_CCR_TCIE_Pos)                     /*!< 0x00000002 */
#define DMA_CCR_TCIE           DMA_CCR_TCIE_Msk                                /*!< Transfer complete interrupt enable  */
#define DMA_CCR_HTIE_Pos       (2U)
#define DMA_CCR_HTIE_Msk       (0x1UL << DMA_CCR_HTIE_Pos)                     /*!< 0x00000004 */
#define DMA_CCR_HTIE           DMA_CCR_HTIE_Msk                                /*!< Half Transfer interrupt enable      */
#define DMA_CCR_TEIE_Pos       (3U)
#define DMA_CCR_TEIE_Msk       (0x1UL << DMA_CCR_TEIE_Pos)                     /*!< 0x00000008 */
#define DMA_CCR_TEIE           DMA_CCR_TEIE_Msk                                /*!< Transfer error interrupt enable     */
#define DMA_CCR_DIR_Pos        (4U)
#define DMA_CCR_DIR_Msk        (0x1UL << DMA_CCR_DIR_Pos)                      /*!< 0x00000010 */
#define DMA_CCR_DIR            DMA_CCR_DIR_Msk                                 /*!< Data transfer direction             */
#define DMA_CCR_CIRC_Pos       (5U)
#define DMA_CCR_CIRC_Msk       (0x1UL << DMA_CCR_CIRC_Pos)                     /*!< 0x00000020 */
#define DMA_CCR_CIRC           DMA_CCR_CIRC_Msk                                /*!< Circular mode                       */
#define DMA_CCR_PINC_Pos       (6U)
#define DMA_CCR_PINC_Msk       (0x1UL << DMA_CCR_PINC_Pos)                     /*!< 0x00000040 */
#define DMA_CCR_PINC           DMA_CCR_PINC_Msk                                /*!< Peripheral increment mode           */
#define DMA_CCR_MINC_Pos       (7U)
#define DMA_CCR_MINC_Msk       (0x1UL << DMA_CCR_MINC_Pos)                     /*!< 0x00000080 */
#define DMA_CCR_MINC           DMA_CCR_MINC_Msk                                /*!< Memory increment mode               */

#define DMA_CCR_PSIZE_Pos      (8U)
#define DMA_CCR_PSIZE_Msk      (0x3UL << DMA_CCR_PSIZE_Pos)                    /*!< 0x00000300 */
#define DMA_CCR_PSIZE          DMA_CCR_PSIZE_Msk                               /*!< PSIZE[1:0] bits (Peripheral size)   */
#define DMA_CCR_PSIZE_0        (0x1UL << DMA_CCR_PSIZE_Pos)                    /*!< 0x00000100 */
#define DMA_CCR_PSIZE_1        (0x2UL << DMA_CCR_PSIZE_Pos)                    /*!< 0x00000200 */

#define DMA_CCR_MSIZE_Pos      (10U)
#define DMA_CCR_MSIZE_Msk      (0x3UL << DMA_CCR_MSIZE_Pos)                    /*!< 0x00000C00 */
#define DMA_CCR_MSIZE          DMA_CCR_MSIZE_Msk                               /*!< MSIZE[1:0] bits (Memory size)       */
#define DMA_CCR_MSIZE_0        (0x1UL << DMA_CCR_MSIZE_Pos)                    /*!< 0x00000400 */
#define DMA_CCR_MSIZE_1        (0x2UL << DMA_CCR_MSIZE_Pos)                    /*!< 0x00000800 */

#define DMA_CCR_PL_Pos         (12U)
#define DMA_CCR_PL_Msk         (0x3UL << DMA_CCR_PL_Pos)                       /*!< 0x00003000 */
#define DMA_CCR_PL             DMA_CCR_PL_Msk                                  /*!< PL[1:0] bits(Channel Priority level)*/
#define DMA_CCR_PL_0           (0x1UL << DMA_CCR_PL_Pos)                       /*!< 0x00001000 */
#define DMA_CCR_PL_1           (0x2UL << DMA_CCR_PL_Pos)                        /*!< 0x00002000 */

#define DMA_CCR_MEM2MEM_Pos    (14U)
#define DMA_CCR_MEM2MEM_Msk    (0x1UL << DMA_CCR_MEM2MEM_Pos)                  /*!< 0x00004000 */
#define DMA_CCR_MEM2MEM        DMA_CCR_MEM2MEM_Msk                             /*!< Memory to memory mode               */

/******************  Bit definition for DMA_CNDTR register  *******************/
#define DMA_CNDTR_NDT_Pos      (0U)
#define DMA_CNDTR_NDT_Msk      (0xFFFFUL << DMA_CNDTR_NDT_Pos)                 /*!< 0x0000FFFF */
#define DMA_CNDTR_NDT          DMA_CNDTR_NDT_Msk                               /*!< Number of data to Transfer          */

/******************  Bit definition for DMA_CPAR register  ********************/
#define DMA_CPAR_PA_Pos        (0U)
#define DMA_CPAR_PA_Msk        (0xFFFFFFFFUL << DMA_CPAR_PA_Pos)               /*!< 0xFFFFFFFF */
#define DMA_CPAR_PA            DMA_CPAR_PA_Msk                                 /*!< Peripheral Address                  */

/******************  Bit definition for DMA_CMAR register  ********************/
#define DMA_CMAR_MA_Pos        (0U)
#define DMA_CMAR_MA_Msk        (0xFFFFFFFFUL << DMA_CMAR_MA_Pos)               /*!< 0xFFFFFFFF */
#define DMA_CMAR_MA            DMA_CMAR_MA_Msk                                 /*!< Memory Address                      */

/******************************************************************************/
/*                                                                            */
/*                             DMAMUX Controller                              */
/*                                                                            */
/******************************************************************************/
/********************  Bits definition for DMAMUX_CxCR register  **************/
#define DMAMUX_CxCR_DMAREQ_ID_Pos              (0U)
#define DMAMUX_CxCR_DMAREQ_ID_Msk              (0xFFUL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x000000FF */
#define DMAMUX_CxCR_DMAREQ_ID                  DMAMUX_CxCR_DMAREQ_ID_Msk             /*!< DMA Request ID   */
#define DMAMUX_CxCR_DMAREQ_ID_0                (0x01UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000001 */
#define DMAMUX_CxCR_DMAREQ_ID_1                (0x02UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000002 */
#define DMAMUX_CxCR_DMAREQ_ID_2                (0x04UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000004 */
#define DMAMUX_CxCR_DMAREQ_ID_3                (0x08UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000008 */
#define DMAMUX_CxCR_DMAREQ_ID_4                (0x10UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000010 */
#define DMAMUX_CxCR_DMAREQ_ID_5                (0x20UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000020 */
#define DMAMUX_CxCR_DMAREQ_ID_6                (0x40UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000040 */
#define DMAMUX_CxCR_DMAREQ_ID_7                (0x80UL << DMAMUX_CxCR_DMAREQ_ID_Pos) /*!< 0x00000080 */
#define DMAMUX_CxCR_SOIE_Pos                   (8U)
#define DMAMUX_CxCR_SOIE_Msk                   (0x1UL << DMAMUX_CxCR_SOIE_Pos)  /*!< 0x00000100 */
#define DMAMUX_CxCR_SOIE                       DMAMUX_CxCR_SOIE_Msk             /*!< Synchro overrun interrupt enable     */
#define DMAMUX_CxCR_EGE_Pos                    (9U)
#define DMAMUX_CxCR_EGE_Msk                    (0x1UL << DMAMUX_CxCR_EGE_Pos)   /*!< 0x00000200 */
#define DMAMUX_CxCR_EGE                        DMAMUX_CxCR_EGE_Msk              /*!< Event generation interrupt enable    */
#define DMAMUX_CxCR_SE_Pos                     (16U)
#define DMAMUX_CxCR_SE_Msk                     (0x1UL << DMAMUX_CxCR_SE_Pos)    /*!< 0x00010000 */
#define DMAMUX_CxCR_SE                         DMAMUX_CxCR_SE_Msk               /*!< Synchronization enable               */
#define DMAMUX_CxCR_SPOL_Pos                   (17U)
#define DMAMUX_CxCR_SPOL_Msk                   (0x3UL << DMAMUX_CxCR_SPOL_Pos)  /*!< 0x00060000 */
#define DMAMUX_CxCR_SPOL                       DMAMUX_CxCR_SPOL_Msk             /*!< Synchronization polarity             */
#define DMAMUX_CxCR_SPOL_0                     (0x1UL << DMAMUX_CxCR_SPOL_Pos)  /*!< 0x00020000 */
#define DMAMUX_CxCR_SPOL_1                     (0x2UL << DMAMUX_CxCR_SPOL_Pos)  /*!< 0x00040000 */
#define DMAMUX_CxCR_NBREQ_Pos                  (19U)
#define DMAMUX_CxCR_NBREQ_Msk                  (0x1FUL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00F80000 */
#define DMAMUX_CxCR_NBREQ                      DMAMUX_CxCR_NBREQ_Msk             /*!< Number of request                    */
#define DMAMUX_CxCR_NBREQ_0                    (0x01UL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00080000 */
#define DMAMUX_CxCR_NBREQ_1                    (0x02UL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00100000 */
#define DMAMUX_CxCR_NBREQ_2                    (0x04UL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00200000 */
#define DMAMUX_CxCR_NBREQ_3                    (0x08UL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00400000 */
#define DMAMUX_CxCR_NBREQ_4                    (0x10UL << DMAMUX_CxCR_NBREQ_Pos) /*!< 0x00800000 */
#define DMAMUX_CxCR_SYNC_ID_Pos                (24U)
#define DMAMUX_CxCR_SYNC_ID_Msk                (0x1FUL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x1F000000 */
#define DMAMUX_CxCR_SYNC_ID                    DMAMUX_CxCR_SYNC_ID_Msk             /*!< Synchronization ID                   */
#define DMAMUX_CxCR_SYNC_ID_0                  (0x01UL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x01000000 */
#define DMAMUX_CxCR_SYNC_ID_1                  (0x02UL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x02000000 */
#define DMAMUX_CxCR_SYNC_ID_2                  (0x04UL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x04000000 */
#define DMAMUX_CxCR_SYNC_ID_3                  (0x08UL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x08000000 */
#define DMAMUX_CxCR_SYNC_ID_4                  (0x10UL << DMAMUX_CxCR_SYNC_ID_Pos) /*!< 0x10000000 */

/*******************  Bits definition for DMAMUX_CSR register  **************/
#define DMAMUX_CSR_SOF0_Pos                    (0U)
#define DMAMUX_CSR_SOF0_Msk                    (0x1UL << DMAMUX_CSR_SOF0_Pos)  /*!< 0x00000001 */
#define DMAMUX_CSR_SOF0                        DMAMUX_CSR_SOF0_Msk             /*!< Synchronization Overrun Flag 0       */
#define DMAMUX_CSR_SOF1_Pos                    (1U)
#define DMAMUX_CSR_SOF1_Msk                    (0x1UL << DMAMUX_CSR_SOF1_Pos)  /*!< 0x00000002 */
#define DMAMUX_CSR_SOF1                        DMAMUX_CSR_SOF1_Msk             /*!< Synchronization Overrun Flag 1       */
#define DMAMUX_CSR_SOF2_Pos                    (2U)
#define DMAMUX_CSR_SOF2_Msk                    (0x1UL << DMAMUX_CSR_SOF2_Pos)  /*!< 0x00000004 */
#define DMAMUX_CSR_SOF2                        DMAMUX_CSR_SOF2_Msk             /*!< Synchronization Overrun Flag 2       */
#define DMAMUX_CSR_SOF3_Pos                    (3U)
#define DMAMUX_CSR_SOF3_Msk                    (0x1UL << DMAMUX_CSR_SOF3_Pos)  /*!< 0x00000008 */
#define DMAMUX_CSR_SOF3                        DMAMUX_CSR_SOF3_Msk             /*!< Synchronization Overrun Flag 3       */
#define DMAMUX_CSR_SOF4_Pos                    (4U)
#define DMAMUX_CSR_SOF4_Msk                    (0x1UL << DMAMUX_CSR_SOF4_Pos)  /*!< 0x00000010 */
#define DMAMUX_CSR_SOF4                        DMAMUX_CSR_SOF4_Msk             /*!< Synchronization Overrun Flag 4       */
#define DMAMUX_CSR_SOF5_Pos                    (5U)
#define DMAMUX_CSR_SOF5_Msk                    (0x1UL << DMAMUX_CSR_SOF5_Pos)  /*!< 0x00000020 */
#define DMAMUX_CSR_SOF5                        DMAMUX_CSR_SOF5_Msk             /*!< Synchronization Overrun Flag 5       */
#define DMAMUX_CSR_SOF6_Pos                    (6U)
#define DMAMUX_CSR_SOF6_Msk                    (0x1UL << DMAMUX_CSR_SOF6_Pos)  /*!< 0x00000040 */
#define DMAMUX_CSR_SOF6                        DMAMUX_CSR_SOF6_Msk             /*!< Synchronization Overrun Flag 6       */

/********************  Bits definition for DMAMUX_CFR register  **************/
#define DMAMUX_CFR_CSOF0_Pos                   (0U)
#define DMAMUX_CFR_CSOF0_Msk                   (0x1UL << DMAMUX_CFR_CSOF0_Pos)  /*!< 0x00000001 */
#define DMAMUX_CFR_CSOF0                       DMAMUX_CFR_CSOF0_Msk             /*!< Clear Overrun Flag 0                 */
#define DMAMUX_CFR_CSOF1_Pos                   (1U)
#define DMAMUX_CFR_CSOF1_Msk                   (0x1UL << DMAMUX_CFR_CSOF1_Pos)  /*!< 0x00000002 */
#define DMAMUX_CFR_CSOF1                       DMAMUX_CFR_CSOF1_Msk             /*!< Clear Overrun Flag 1                 */
#define DMAMUX_CFR_CSOF2_Pos                   (2U)
#define DMAMUX_CFR_CSOF2_Msk                   (0x1UL << DMAMUX_CFR_CSOF2_Pos)  /*!< 0x00000004 */
#define DMAMUX_CFR_CSOF2                       DMAMUX_CFR_CSOF2_Msk             /*!< Clear Overrun Flag 2                 */
#define DMAMUX_CFR_CSOF3_Pos                   (3U)
#define DMAMUX_CFR_CSOF3_Msk                   (0x1UL << DMAMUX_CFR_CSOF3_Pos)  /*!< 0x00000008 */
#define DMAMUX_CFR_CSOF3                       DMAMUX_CFR_CSOF3_Msk             /*!< Clear Overrun Flag 3                 */
#define DMAMUX_CFR_CSOF4_Pos                   (4U)
#define DMAMUX_CFR_CSOF4_Msk                   (0x1UL << DMAMUX_CFR_CSOF4_Pos)  /*!< 0x00000010 */
#define DMAMUX_CFR_CSOF4                       DMAMUX_CFR_CSOF4_Msk             /*!< Clear Overrun Flag 4                 */
#define DMAMUX_CFR_CSOF5_Pos                   (5U)
#define DMAMUX_CFR_CSOF5_Msk                   (0x1UL << DMAMUX_CFR_CSOF5_Pos)  /*!< 0x00000020 */
#define DMAMUX_CFR_CSOF5                       DMAMUX_CFR_CSOF5_Msk             /*!< Clear Overrun Flag 5                 */
#define DMAMUX_CFR_CSOF6_Pos                   (6U)
#define DMAMUX_CFR_CSOF6_Msk                   (0x1UL << DMAMUX_CFR_CSOF6_Pos)  /*!< 0x00000040 */
#define DMAMUX_CFR_CSOF6                       DMAMUX_CFR_CSOF6_Msk             /*!< Clear Overrun Flag 6                 */

/********************  Bits definition for DMAMUX_RGxCR register  ************/
#define DMAMUX_RGxCR_SIG_ID_Pos                (0U)
#define DMAMUX_RGxCR_SIG_ID_Msk                (0x1FUL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x0000001F */
#define DMAMUX_RGxCR_SIG_ID                    DMAMUX_RGxCR_SIG_ID_Msk             /*!< Signal ID                         */
#define DMAMUX_RGxCR_SIG_ID_0                  (0x01UL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x00000001 */
#define DMAMUX_RGxCR_SIG_ID_1                  (0x02UL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x00000002 */
#define DMAMUX_RGxCR_SIG_ID_2                  (0x04UL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x00000004 */
#define DMAMUX_RGxCR_SIG_ID_3                  (0x08UL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x00000008 */
#define DMAMUX_RGxCR_SIG_ID_4                  (0x10UL << DMAMUX_RGxCR_SIG_ID_Pos) /*!< 0x00000010 */
#define DMAMUX_RGxCR_OIE_Pos                   (8U)
#define DMAMUX_RGxCR_OIE_Msk                   (0x1UL << DMAMUX_RGxCR_OIE_Pos)  /*!< 0x00000100 */
#define DMAMUX_RGxCR_OIE                       DMAMUX_RGxCR_OIE_Msk             /*!< Overrun interrupt enable             */
#define DMAMUX_RGxCR_GE_Pos                    (16U)
#define DMAMUX_RGxCR_GE_Msk                    (0x1UL << DMAMUX_RGxCR_GE_Pos)   /*!< 0x00010000 */
#define DMAMUX_RGxCR_GE                        DMAMUX_RGxCR_GE_Msk              /*!< Generation enable                    */
#define DMAMUX_RGxCR_GPOL_Pos                  (17U)
#define DMAMUX_RGxCR_GPOL_Msk                  (0x3UL << DMAMUX_RGxCR_GPOL_Pos) /*!< 0x00060000 */
#define DMAMUX_RGxCR_GPOL                      DMAMUX_RGxCR_GPOL_Msk            /*!< Generation polarity                  */
#define DMAMUX_RGxCR_GPOL_0                    (0x1UL << DMAMUX_RGxCR_GPOL_Pos) /*!< 0x00020000 */
#define DMAMUX_RGxCR_GPOL_1                    (0x2UL << DMAMUX_RGxCR_GPOL_Pos) /*!< 0x00040000 */
#define DMAMUX_RGxCR_GNBREQ_Pos                (19U)
#define DMAMUX_RGxCR_GNBREQ_Msk                (0x1FUL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00F80000 */
#define DMAMUX_RGxCR_GNBREQ                    DMAMUX_RGxCR_GNBREQ_Msk             /*!< Number of request                 */
#define DMAMUX_RGxCR_GNBREQ_0                  (0x01UL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00080000 */
#define DMAMUX_RGxCR_GNBREQ_1                  (0x02UL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00100000 */
#define DMAMUX_RGxCR_GNBREQ_2                  (0x04UL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00200000 */
#define DMAMUX_RGxCR_GNBREQ_3                  (0x08UL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00400000 */
#define DMAMUX_RGxCR_GNBREQ_4                  (0x10UL << DMAMUX_RGxCR_GNBREQ_Pos) /*!< 0x00800000 */

/********************  Bits definition for DMAMUX_RGSR register  **************/
#define DMAMUX_RGSR_OF0_Pos                    (0U)
#define DMAMUX_RGSR_OF0_Msk                    (0x1UL << DMAMUX_RGSR_OF0_Pos)   /*!< 0x00000001 */
#define DMAMUX_RGSR_OF0                        DMAMUX_RGSR_OF0_Msk              /*!< Overrun flag 0                       */
#define DMAMUX_RGSR_OF1_Pos                    (1U)
#define DMAMUX_RGSR_OF1_Msk                    (0x1UL << DMAMUX_RGSR_OF1_Pos)   /*!< 0x00000002 */
#define DMAMUX_RGSR_OF1                        DMAMUX_RGSR_OF1_Msk              /*!< Overrun flag 1                       */
#define DMAMUX_RGSR_OF2_Pos                    (2U)
#define DMAMUX_RGSR_OF2_Msk                    (0x1UL << DMAMUX_RGSR_OF2_Pos)   /*!< 0x00000004 */
#define DMAMUX_RGSR_OF2                        DMAMUX_RGSR_OF2_Msk              /*!< Overrun flag 2                       */
#define DMAMUX_RGSR_OF3_Pos                    (3U)
#define DMAMUX_RGSR_OF3_Msk                    (0x1UL << DMAMUX_RGSR_OF3_Pos)   /*!< 0x00000008 */
#define DMAMUX_RGSR_OF3                        DMAMUX_RGSR_OF3_Msk              /*!< Overrun flag 3                       */

/********************  Bits definition for DMAMUX_RGCFR register  **************/
#define DMAMUX_RGCFR_COF0_Pos                  (0U)
#define DMAMUX_RGCFR_COF0_Msk                  (0x1UL << DMAMUX_RGCFR_COF0_Pos) /*!< 0x00000001 */
#define DMAMUX_RGCFR_COF0                      DMAMUX_RGCFR_COF0_Msk            /*!< Clear Overrun flag 0                 */
#define DMAMUX_RGCFR_COF1_Pos                  (1U)
#define DMAMUX_RGCFR_COF1_Msk                  (0x1UL << DMAMUX_RGCFR_COF1_Pos) /*!< 0x00000002 */
#define DMAMUX_RGCFR_COF1                      DMAMUX_RGCFR_COF1_Msk            /*!< Clear Overrun flag 1                 */
#define DMAMUX_RGCFR_COF2_Pos                  (2U)
#define DMAMUX_RGCFR_COF2_Msk                  (0x1UL << DMAMUX_RGCFR_COF2_Pos) /*!< 0x00000004 */
#define DMAMUX_RGCFR_COF2                      DMAMUX_RGCFR_COF2_Msk            /*!< Clear Overrun flag 2                 */
#define DMAMUX_RGCFR_COF3_Pos                  (3U)
#define DMAMUX_RGCFR_COF3_Msk                  (0x1UL << DMAMUX_RGCFR_COF3_Pos) /*!< 0x00000008 */
#define DMAMUX_RGCFR_COF3                      DMAMUX_RGCFR_COF3_Msk            /*!< Clear Overrun flag 3                 */

/******************************************************************************/
/*                                                                            */
/*                    External Interrupt/Event Controller                     */
/*                                                                            */
/******************************************************************************/
/******************  Bit definition for EXTI_RTSR1 register  ******************/
#define EXTI_RTSR1_RT0_Pos           (0U)
#define EXTI_RTSR1_RT0_Msk           (0x1UL << EXTI_RTSR1_RT0_Pos)             /*!< 0x00000001 */
#define EXTI_RTSR1_RT0               EXTI_RTSR1_RT0_Msk                        /*!< Rising trigger configuration for input line 0 */
#define EXTI_RTSR1_RT1_Pos           (1U)
#define EXTI_RTSR1_RT1_Msk           (0x1UL << EXTI_RTSR1_RT1_Pos)             /*!< 0x00000002 */
#define EXTI_RTSR1_RT1               EXTI_RTSR1_RT1_Msk                        /*!< Rising trigger configuration for input line 1 */
#define EXTI_RTSR1_RT2_Pos           (2U)
#define EXTI_RTSR1_RT2_Msk           (0x1UL << EXTI_RTSR1_RT2_Pos)             /*!< 0x00000004 */
#define EXTI_RTSR1_RT2               EXTI_RTSR1_RT2_Msk                        /*!< Rising trigger configuration for input line 2 */
#define EXTI_RTSR1_RT3_Pos           (3U)
#define EXTI_RTSR1_RT3_Msk           (0x1UL << EXTI_RTSR1_RT3_Pos)             /*!< 0x00000008 */
#define EXTI_RTSR1_RT3               EXTI_RTSR1_RT3_Msk                        /*!< Rising trigger configuration for input line 3 */
#define EXTI_RTSR1_RT4_Pos           (4U)
#define EXTI_RTSR1_RT4_Msk           (0x1UL << EXTI_RTSR1_RT4_Pos)             /*!< 0x00000010 */
#define EXTI_RTSR1_RT4               EXTI_RTSR1_RT4_Msk                        /*!< Rising trigger configuration for input line 4 */
#define EXTI_RTSR1_RT5_Pos           (5U)
#define EXTI_RTSR1_RT5_Msk           (0x1UL << EXTI_RTSR1_RT5_Pos)             /*!< 0x00000020 */
#define EXTI_RTSR1_RT5               EXTI_RTSR1_RT5_Msk                        /*!< Rising trigger configuration for input line 5 */
#define EXTI_RTSR1_RT6_Pos           (6U)
#define EXTI_RTSR1_RT6_Msk           (0x1UL << EXTI_RTSR1_RT6_Pos)             /*!< 0x00000040 */
#define EXTI_RTSR1_RT6               EXTI_RTSR1_RT6_Msk                        /*!< Rising trigger configuration for input line 6 */
#define EXTI_RTSR1_RT7_Pos           (7U)
#define EXTI_RTSR1_RT7_Msk           (0x1UL << EXTI_RTSR1_RT7_Pos)             /*!< 0x00000080 */
#define EXTI_RTSR1_RT7               EXTI_RTSR1_RT7_Msk                        /*!< Rising trigger configuration for input line 7 */
#define EXTI_RTSR1_RT8_Pos           (8U)
#define EXTI_RTSR1_RT8_Msk           (0x1UL << EXTI_RTSR1_RT8_Pos)             /*!< 0x00000100 */
#define EXTI_RTSR1_RT8               EXTI_RTSR1_RT8_Msk                        /*!< Rising trigger configuration for input line 8 */
#define EXTI_RTSR1_RT9_Pos           (9U)
#define EXTI_RTSR1_RT9_Msk           (0x1UL << EXTI_RTSR1_RT9_Pos)             /*!< 0x00000200 */
#define EXTI_RTSR1_RT9               EXTI_RTSR1_RT9_Msk                        /*!< Rising trigger configuration for input line 9 */
#define EXTI_RTSR1_RT10_Pos          (10U)
#define EXTI_RTSR1_RT10_Msk          (0x1UL << EXTI_RTSR1_RT10_Pos)            /*!< 0x00000400 */
#define EXTI_RTSR1_RT10              EXTI_RTSR1_RT10_Msk                       /*!< Rising trigger configuration for input line 10 */
#define EXTI_RTSR1_RT11_Pos          (11U)
#define EXTI_RTSR1_RT11_Msk          (0x1UL << EXTI_RTSR1_RT11_Pos)            /*!< 0x00000800 */
#define EXTI_RTSR1_RT11              EXTI_RTSR1_RT11_Msk                       /*!< Rising trigger configuration for input line 11 */
#define EXTI_RTSR1_RT12_Pos          (12U)
#define EXTI_RTSR1_RT12_Msk          (0x1UL << EXTI_RTSR1_RT12_Pos)            /*!< 0x00001000 */
#define EXTI_RTSR1_RT12              EXTI_RTSR1_RT12_Msk                       /*!< Rising trigger configuration for input line 12 */
#define EXTI_RTSR1_RT13_Pos          (13U)
#define EXTI_RTSR1_RT13_Msk          (0x1UL << EXTI_RTSR1_RT13_Pos)            /*!< 0x00002000 */
#define EXTI_RTSR1_RT13              EXTI_RTSR1_RT13_Msk                       /*!< Rising trigger configuration for input line 13 */
#define EXTI_RTSR1_RT14_Pos          (14U)
#define EXTI_RTSR1_RT14_Msk          (0x1UL << EXTI_RTSR1_RT14_Pos)            /*!< 0x00004000 */
#define EXTI_RTSR1_RT14              EXTI_RTSR1_RT14_Msk                       /*!< Rising trigger configuration for input line 14 */
#define EXTI_RTSR1_RT15_Pos          (15U)
#define EXTI_RTSR1_RT15_Msk          (0x1UL << EXTI_RTSR1_RT15_Pos)            /*!< 0x00008000 */
#define EXTI_RTSR1_RT15              EXTI_RTSR1_RT15_Msk                       /*!< Rising trigger configuration for input line 15 */
#define EXTI_RTSR1_RT16_Pos          (16U)
#define EXTI_RTSR1_RT16_Msk          (0x1UL << EXTI_RTSR1_RT16_Pos)            /*!< 0x00010000 */
#define EXTI_RTSR1_RT16              EXTI_RTSR1_RT16_Msk                       /*!< Rising trigger configuration for input line 16 */
#define EXTI_RTSR1_RT17_Pos          (17U)
#define EXTI_RTSR1_RT17_Msk          (0x1UL << EXTI_RTSR1_RT17_Pos)            /*!< 0x00020000 */
#define EXTI_RTSR1_RT17              EXTI_RTSR1_RT17_Msk                       /*!< Rising trigger configuration for input line 17 */
#define EXTI_RTSR1_RT18_Pos          (18U)
#define EXTI_RTSR1_RT18_Msk          (0x1UL << EXTI_RTSR1_RT18_Pos)            /*!< 0x00040000 */
#define EXTI_RTSR1_RT18              EXTI_RTSR1_RT18_Msk                       /*!< Rising trigger configuration for input line 18 */

/******************  Bit definition for EXTI_FTSR1 register  ******************/
#define EXTI_FTSR1_FT0_Pos           (0U)
#define EXTI_FTSR1_FT0_Msk           (0x1UL << EXTI_FTSR1_FT0_Pos)             /*!< 0x00000001 */
#define EXTI_FTSR1_FT0               EXTI_FTSR1_FT0_Msk                        /*!< Falling trigger configuration for input line 0 */
#define EXTI_FTSR1_FT1_Pos           (1U)
#define EXTI_FTSR1_FT1_Msk           (0x1UL << EXTI_FTSR1_FT1_Pos)             /*!< 0x00000002 */
#define EXTI_FTSR1_FT1               EXTI_FTSR1_FT1_Msk                        /*!< Falling trigger configuration for input line 1 */
#define EXTI_FTSR1_FT2_Pos           (2U)
#define EXTI_FTSR1_FT2_Msk           (0x1UL << EXTI_FTSR1_FT2_Pos)             /*!< 0x00000004 */
#define EXTI_FTSR1_FT2               EXTI_FTSR1_FT2_Msk                        /*!< Falling trigger configuration for input line 2 */
#define EXTI_FTSR1_FT3_Pos           (3U)
#define EXTI_FTSR1_FT3_Msk           (0x1UL << EXTI_FTSR1_FT3_Pos)             /*!< 0x00000008 */
#define EXTI_FTSR1_FT3               EXTI_FTSR1_FT3_Msk                        /*!< Falling trigger configuration for input line 3 */
#define EXTI_FTSR1_FT4_Pos           (4U)
#define EXTI_FTSR1_FT4_Msk           (0x1UL << EXTI_FTSR1_FT4_Pos)             /*!< 0x00000010 */
#define EXTI_FTSR1_FT4               EXTI_FTSR1_FT4_Msk                        /*!< Falling trigger configuration for input line 4 */
#define EXTI_FTSR1_FT5_Pos           (5U)
#define EXTI_FTSR1_FT5_Msk           (0x1UL << EXTI_FTSR1_FT5_Pos)             /*!< 0x00000020 */
#define EXTI_FTSR1_FT5               EXTI_FTSR1_FT5_Msk                        /*!< Falling trigger configuration for input line 5 */
#define EXTI_FTSR1_FT6_Pos           (6U)
#define EXTI_FTSR1_FT6_Msk           (0x1UL << EXTI_FTSR1_FT6_Pos)             /*!< 0x00000040 */
#define EXTI_FTSR1_FT6               EXTI_FTSR1_FT6_Msk                        /*!< Falling trigger configuration for input line 6 */
#define EXTI_FTSR1_FT7_Pos           (7U)
#define EXTI_FTSR1_FT7_Msk           (0x1UL << EXTI_FTSR1_FT7_Pos)             /*!< 0x00000080 */
#define EXTI_FTSR1_FT7               EXTI_FTSR1_FT7_Msk                        /*!< Falling trigger configuration for input line 7 */
#define EXTI_FTSR1_FT8_Pos           (8U)
#define EXTI_FTSR1_FT8_Msk           (0x1UL << EXTI_FTSR1_FT8_Pos)             /*!< 0x00000100 */
#define EXTI_FTSR1_FT8               EXTI_FTSR1_FT8_Msk                        /*!< Falling trigger configuration for input line 8 */
#define EXTI_FTSR1_FT9_Pos           (9U)
#define EXTI_FTSR1_FT9_Msk           (0x1UL << EXTI_FTSR1_FT9_Pos)             /*!< 0x00000200 */
#define EXTI_FTSR1_FT9               EXTI_FTSR1_FT9_Msk                        /*!< Falling trigger configuration for input line 9 */
#define EXTI_FTSR1_FT10_Pos          (10U)
#define EXTI_FTSR1_FT10_Msk          (0x1UL << EXTI_FTSR1_FT10_Pos)            /*!< 0x00000400 */
#define EXTI_FTSR1_FT10              EXTI_FTSR1_FT10_Msk                       /*!< Falling trigger configuration for input line 10 */
#define EXTI_FTSR1_FT11_Pos          (11U)
#define EXTI_FTSR1_FT11_Msk          (0x1UL << EXTI_FTSR1_FT11_Pos)            /*!< 0x00000800 */
#define EXTI_FTSR1_FT11              EXTI_FTSR1_FT11_Msk                       /*!< Falling trigger configuration for input line 11 */
#define EXTI_FTSR1_FT12_Pos          (12U)
#define EXTI_FTSR1_FT12_Msk          (0x1UL << EXTI_FTSR1_FT12_Pos)            /*!< 0x00001000 */
#define EXTI_FTSR1_FT12              EXTI_FTSR1_FT12_Msk                       /*!< Falling trigger configuration for input line 12 */
#define EXTI_FTSR1_FT13_Pos          (13U)
#define EXTI_FTSR1_FT13_Msk          (0x1UL << EXTI_FTSR1_FT13_Pos)            /*!< 0x00002000 */
#define EXTI_FTSR1_FT13              EXTI_FTSR1_FT13_Msk                       /*!< Falling trigger configuration for input line 13 */
#define EXTI_FTSR1_FT14_Pos          (14U)
#define EXTI_FTSR1_FT14_Msk          (0x1UL << EXTI_FTSR1_FT14_Pos)            /*!< 0x00004000 */
#define EXTI_FTSR1_FT14              EXTI_FTSR1_FT14_Msk                       /*!< Falling trigger configuration for input line 14 */
#define EXTI_FTSR1_FT15_Pos          (15U)
#define EXTI_FTSR1_FT15_Msk          (0x1UL << EXTI_FTSR1_FT15_Pos)            /*!< 0x00008000 */
#define EXTI_FTSR1_FT15              EXTI_FTSR1_FT15_Msk                       /*!< Falling trigger configuration for input line 15 */
#define EXTI_FTSR1_FT16_Pos          (16U)
#define EXTI_FTSR1_FT16_Msk          (0x1UL << EXTI_FTSR1_FT16_Pos)            /*!< 0x00010000 */
#define EXTI_FTSR1_FT16              EXTI_FTSR1_FT16_Msk                       /*!< Falling trigger configuration for input line 16 */
#define EXTI_FTSR1_FT17_Pos          (17U)
#define EXTI_FTSR1_FT17_Msk          (0x1UL << EXTI_FTSR1_FT17_Pos)            /*!< 0x00020000 */
#define EXTI_FTSR1_FT17              EXTI_FTSR1_FT17_Msk                       /*!< Falling trigger configuration for input line 17 */
#define EXTI_FTSR1_FT18_Pos          (18U)
#define EXTI_FTSR1_FT18_Msk          (0x1UL << EXTI_FTSR1_FT18_Pos)            /*!< 0x00040000 */
#define EXTI_FTSR1_FT18              EXTI_FTSR1_FT18_Msk                       /*!< Falling trigger configuration for input line 18 */

/******************  Bit definition for EXTI_SWIER1 register  *****************/
#define EXTI_SWIER1_SWI0_Pos         (0U)
#define EXTI_SWIER1_SWI0_Msk         (0x1UL << EXTI_SWIER1_SWI0_Pos)           /*!< 0x00000001 */
#define EXTI_SWIER1_SWI0             EXTI_SWIER1_SWI0_Msk                      /*!< Software Interrupt on line 0 */
#define EXTI_SWIER1_SWI1_Pos         (1U)
#define EXTI_SWIER1_SWI1_Msk         (0x1UL << EXTI_SWIER1_SWI1_Pos)           /*!< 0x00000002 */
#define EXTI_SWIER1_SWI1             EXTI_SWIER1_SWI1_Msk                      /*!< Software Interrupt on line 1 */
#define EXTI_SWIER1_SWI2_Pos         (2U)
#define EXTI_SWIER1_SWI2_Msk         (0x1UL << EXTI_SWIER1_SWI2_Pos)           /*!< 0x00000004 */
#define EXTI_SWIER1_SWI2             EXTI_SWIER1_SWI2_Msk                      /*!< Software Interrupt on line 2 */
#define EXTI_SWIER1_SWI3_Pos         (3U)
#define EXTI_SWIER1_SWI3_Msk         (0x1UL << EXTI_SWIER1_SWI3_Pos)           /*!< 0x00000008 */
#define EXTI_SWIER1_SWI3             EXTI_SWIER1_SWI3_Msk                      /*!< Software Interrupt on line 3 */
#define EXTI_SWIER1_SWI4_Pos         (4U)
#define EXTI_SWIER1_SWI4_Msk         (0x1UL << EXTI_SWIER1_SWI4_Pos)           /*!< 0x00000010 */
#define EXTI_SWIER1_SWI4             EXTI_SWIER1_SWI4_Msk                      /*!< Software Interrupt on line 4 */
#define EXTI_SWIER1_SWI5_Pos         (5U)
#define EXTI_SWIER1_SWI5_Msk         (0x1UL << EXTI_SWIER1_SWI5_Pos)           /*!< 0x00000020 */
#define EXTI_SWIER1_SWI5             EXTI_SWIER1_SWI5_Msk                      /*!< Software Interrupt on line 5 */
#define EXTI_SWIER1_SWI6_Pos         (6U)
#define EXTI_SWIER1_SWI6_Msk         (0x1UL << EXTI_SWIER1_SWI6_Pos)           /*!< 0x00000040 */
#define EXTI_SWIER1_SWI6             EXTI_SWIER1_SWI6_Msk                      /*!< Software Interrupt on line 6 */
#define EXTI_SWIER1_SWI7_Pos         (7U)
#define EXTI_SWIER1_SWI7_Msk         (0x1UL << EXTI_SWIER1_SWI7_Pos)           /*!< 0x00000080 */
#define EXTI_SWIER1_SWI7             EXTI_SWIER1_SWI7_Msk                      /*!< Software Interrupt on line 7 */
#define EXTI_SWIER1_SWI8_Pos         (8U)
#define EXTI_SWIER1_SWI8_Msk         (0x1UL << EXTI_SWIER1_SWI8_Pos)           /*!< 0x00000100 */
#define EXTI_SWIER1_SWI8             EXTI_SWIER1_SWI8_Msk                      /*!< Software Interrupt on line 8 */
#define EXTI_SWIER1_SWI9_Pos         (9U)
#define EXTI_SWIER1_SWI9_Msk         (0x1UL << EXTI_SWIER1_SWI9_Pos)           /*!< 0x00000200 */
#define EXTI_SWIER1_SWI9             EXTI_SWIER1_SWI9_Msk                      /*!< Software Interrupt on line 9 */
#define EXTI_SWIER1_SWI10_Pos        (10U)
#define EXTI_SWIER1_SWI10_Msk        (0x1UL << EXTI_SWIER1_SWI10_Pos)          /*!< 0x00000400 */
#define EXTI_SWIER1_SWI10            EXTI_SWIER1_SWI10_Msk                     /*!< Software Interrupt on line 10 */
#define EXTI_SWIER1_SWI11_Pos        (11U)
#define EXTI_SWIER1_SWI11_Msk        (0x1UL << EXTI_SWIER1_SWI11_Pos)          /*!< 0x00000800 */
#define EXTI_SWIER1_SWI11            EXTI_SWIER1_SWI11_Msk                     /*!< Software Interrupt on line 11 */
#define EXTI_SWIER1_SWI12_Pos        (12U)
#define EXTI_SWIER1_SWI12_Msk        (0x1UL << EXTI_SWIER1_SWI12_Pos)          /*!< 0x00001000 */
#define EXTI_SWIER1_SWI12            EXTI_SWIER1_SWI12_Msk                     /*!< Software Interrupt on line 12 */
#define EXTI_SWIER1_SWI13_Pos        (13U)
#define EXTI_SWIER1_SWI13_Msk        (0x1UL << EXTI_SWIER1_SWI13_Pos)          /*!< 0x00002000 */
#define EXTI_SWIER1_SWI13            EXTI_SWIER1_SWI13_Msk                     /*!< Software Interrupt on line 13 */
#define EXTI_SWIER1_SWI14_Pos        (14U)
#define EXTI_SWIER1_SWI14_Msk        (0x1UL << EXTI_SWIER1_SWI14_Pos)          /*!< 0x00004000 */
#define EXTI_SWIER1_SWI14            EXTI_SWIER1_SWI14_Msk                     /*!< Software Interrupt on line 14 */
#define EXTI_SWIER1_SWI15_Pos        (15U)
#define EXTI_SWIER1_SWI15_Msk        (0x1UL << EXTI_SWIER1_SWI15_Pos)          /*!< 0x00008000 */
#define EXTI_SWIER1_SWI15            EXTI_SWIER1_SWI15_Msk                     /*!< Software Interrupt on line 15 */
#define EXTI_SWIER1_SWI16_Pos        (16U)
#define EXTI_SWIER1_SWI16_Msk        (0x1UL << EXTI_SWIER1_SWI16_Pos)          /*!< 0x00010000 */
#define EXTI_SWIER1_SWI16            EXTI_SWIER1_SWI16_Msk                     /*!< Software Interrupt on line 16 */
#define EXTI_SWIER1_SWI17_Pos        (17U)
#define EXTI_SWIER1_SWI17_Msk        (0x1UL << EXTI_SWIER1_SWI17_Pos)          /*!< 0x00020000 */
#define EXTI_SWIER1_SWI17            EXTI_SWIER1_SWI17_Msk                     /*!< Software Interrupt on line 17 */
#define EXTI_SWIER1_SWI18_Pos        (18U)
#define EXTI_SWIER1_SWI18_Msk        (0x1UL << EXTI_SWIER1_SWI18_Pos)          /*!< 0x00040000 */
#define EXTI_SWIER1_SWI18            EXTI_SWIER1_SWI18_Msk                     /*!< Software Interrupt on line 18 */

/*******************  Bit definition for EXTI_RPR1 register  ******************/
#define EXTI_RPR1_RPIF0_Pos          (0U)
#define EXTI_RPR1_RPIF0_Msk          (0x1UL << EXTI_RPR1_RPIF0_Pos)            /*!< 0x00000001 */
#define EXTI_RPR1_RPIF0              EXTI_RPR1_RPIF0_Msk                       /*!< Rising Pending Interrupt Flag on line 0 */
#define EXTI_RPR1_RPIF1_Pos          (1U)
#define EXTI_RPR1_RPIF1_Msk          (0x1UL << EXTI_RPR1_RPIF1_Pos)            /*!< 0x00000002 */
#define EXTI_RPR1_RPIF1              EXTI_RPR1_RPIF1_Msk                       /*!< Rising Pending Interrupt Flag on line 1 */
#define EXTI_RPR1_RPIF2_Pos          (2U)
#define EXTI_RPR1_RPIF2_Msk          (0x1UL << EXTI_RPR1_RPIF2_Pos)            /*!< 0x00000004 */
#define EXTI_RPR1_RPIF2              EXTI_RPR1_RPIF2_Msk                       /*!< Rising Pending Interrupt Flag on line 2 */
#define EXTI_RPR1_RPIF3_Pos          (3U)
#define EXTI_RPR1_RPIF3_Msk          (0x1UL << EXTI_RPR1_RPIF3_Pos)            /*!< 0x00000008 */
#define EXTI_RPR1_RPIF3              EXTI_RPR1_RPIF3_Msk                       /*!< Rising Pending Interrupt Flag on line 3 */
#define EXTI_RPR1_RPIF4_Pos          (4U)
#define EXTI_RPR1_RPIF4_Msk          (0x1UL << EXTI_RPR1_RPIF4_Pos)            /*!< 0x00000010 */
#define EXTI_RPR1_RPIF4              EXTI_RPR1_RPIF4_Msk                       /*!< Rising Pending Interrupt Flag on line 4 */
#define EXTI_RPR1_RPIF5_Pos          (5U)
#define EXTI_RPR1_RPIF5_Msk          (0x1UL << EXTI_RPR1_RPIF5_Pos)            /*!< 0x00000020 */
#define EXTI_RPR1_RPIF5              EXTI_RPR1_RPIF5_Msk                       /*!< Rising Pending Interrupt Flag on line 5 */
#define EXTI_RPR1_RPIF6_Pos          (6U)
#define EXTI_RPR1_RPIF6_Msk          (0x1UL << EXTI_RPR1_RPIF6_Pos)            /*!< 0x00000040 */
#define EXTI_RPR1_RPIF6              EXTI_RPR1_RPIF6_Msk                       /*!< Rising Pending Interrupt Flag on line 6 */
#define EXTI_RPR1_RPIF7_Pos          (7U)
#define EXTI_RPR1_RPIF7_Msk          (0x1UL << EXTI_RPR1_RPIF7_Pos)            /*!< 0x00000080 */
#define EXTI_RPR1_RPIF7              EXTI_RPR1_RPIF7_Msk                       /*!< Rising Pending Interrupt Flag on line 7 */
#define EXTI_RPR1_RPIF8_Pos          (8U)
#define EXTI_RPR1_RPIF8_Msk          (0x1UL << EXTI_RPR1_RPIF8_Pos)            /*!< 0x00000100 */
#define EXTI_RPR1_RPIF8              EXTI_RPR1_RPIF8_Msk                       /*!< Rising Pending Interrupt Flag on line 8 */
#define EXTI_RPR1_RPIF9_Pos          (9U)
#define EXTI_RPR1_RPIF9_Msk          (0x1UL << EXTI_RPR1_RPIF9_Pos)            /*!< 0x00000200 */
#define EXTI_RPR1_RPIF9              EXTI_RPR1_RPIF9_Msk                       /*!< Rising Pending Interrupt Flag on line 9 */
#define EXTI_RPR1_RPIF10_Pos         (10U)
#define EXTI_RPR1_RPIF10_Msk         (0x1UL << EXTI_RPR1_RPIF10_Pos)           /*!< 0x00000400 */
#define EXTI_RPR1_RPIF10             EXTI_RPR1_RPIF10_Msk                      /*!< Rising Pending Interrupt Flag on line 10 */
#define EXTI_RPR1_RPIF11_Pos         (11U)
#define EXTI_RPR1_RPIF11_Msk         (0x1UL << EXTI_RPR1_RPIF11_Pos)           /*!< 0x00000800 */
#define EXTI_RPR1_RPIF11             EXTI_RPR1_RPIF11_Msk                      /*!< Rising Pending Interrupt Flag on line 11 */
#define EXTI_RPR1_RPIF12_Pos         (12U)
#define EXTI_RPR1_RPIF12_Msk         (0x1UL << EXTI_RPR1_RPIF12_Pos)           /*!< 0x00001000 */
#define EXTI_RPR1_RPIF12             EXTI_RPR1_RPIF12_Msk                      /*!< Rising Pending Interrupt Flag on line 12 */
#define EXTI_RPR1_RPIF13_Pos         (13U)
#define EXTI_RPR1_RPIF13_Msk         (0x1UL << EXTI_RPR1_RPIF13_Pos)           /*!< 0x00002000 */
#define EXTI_RPR1_RPIF13             EXTI_RPR1_RPIF13_Msk                      /*!< Rising Pending Interrupt Flag on line 13 */
#define EXTI_RPR1_RPIF14_Pos         (14U)
#define EXTI_RPR1_RPIF14_Msk         (0x1UL << EXTI_RPR1_RPIF14_Pos)           /*!< 0x00004000 */
#define EXTI_RPR1_RPIF14             EXTI_RPR1_RPIF14_Msk                      /*!< Rising Pending Interrupt Flag on line 14 */
#define EXTI_RPR1_RPIF15_Pos         (15U)
#define EXTI_RPR1_RPIF15_Msk         (0x1UL << EXTI_RPR1_RPIF15_Pos)           /*!< 0x00008000 */
#define EXTI_RPR1_RPIF15             EXTI_RPR1_RPIF15_Msk                      /*!< Rising Pending Interrupt Flag on line 15 */
#define EXTI_RPR1_RPIF16_Pos         (16U)
#define EXTI_RPR1_RPIF16_Msk         (0x1UL << EXTI_RPR1_RPIF16_Pos)           /*!< 0x00010000 */
#define EXTI_RPR1_RPIF16             EXTI_RPR1_RPIF16_Msk                      /*!< Rising Pending Interrupt Flag on line 16 */
#define EXTI_RPR1_RPIF17_Pos         (17U)
#define EXTI_RPR1_RPIF17_Msk         (0x1UL << EXTI_RPR1_RPIF17_Pos)           /*!< 0x00020000 */
#define EXTI_RPR1_RPIF17             EXTI_RPR1_RPIF17_Msk                      /*!< Rising Pending Interrupt Flag on line 17 */
#define EXTI_RPR1_RPIF18_Pos         (18U)
#define EXTI_RPR1_RPIF18_Msk         (0x1UL << EXTI_RPR1_RPIF18_Pos)           /*!< 0x00040000 */
#define EXTI_RPR1_RPIF18             EXTI_RPR1_RPIF18_Msk                      /*!< Rising Pending Interrupt Flag on line 18 */

/*******************  Bit definition for EXTI_FPR1 register  ******************/
#define EXTI_FPR1_FPIF0_Pos          (0U)
#define EXTI_FPR1_FPIF0_Msk          (0x1UL << EXTI_FPR1_FPIF0_Pos)            /*!< 0x00000001 */
#define EXTI_FPR1_FPIF0              EXTI_FPR1_FPIF0_Msk                       /*!< Falling Pending Interrupt Flag on line 0 */
#define EXTI_FPR1_FPIF1_Pos          (1U)
#define EXTI_FPR1_FPIF1_Msk          (0x1UL << EXTI_FPR1_FPIF1_Pos)            /*!< 0x00000002 */
#define EXTI_FPR1_FPIF1              EXTI_FPR1_FPIF1_Msk                       /*!< Falling Pending Interrupt Flag on line 1 */
#define EXTI_FPR1_FPIF2_Pos          (2U)
#define EXTI_FPR1_FPIF2_Msk          (0x1UL << EXTI_FPR1_FPIF2_Pos)            /*!< 0x00000004 */
#define EXTI_FPR1_FPIF2              EXTI_FPR1_FPIF2_Msk                       /*!< Falling Pending Interrupt Flag on line 2 */
#define EXTI_FPR1_FPIF3_Pos          (3U)
#define EXTI_FPR1_FPIF3_Msk          (0x1UL << EXTI_FPR1_FPIF3_Pos)            /*!< 0x00000008 */
#define EXTI_FPR1_FPIF3              EXTI_FPR1_FPIF3_Msk                       /*!< Falling Pending Interrupt Flag on line 3 */
#define EXTI_FPR1_FPIF4_Pos          (4U)
#define EXTI_FPR1_FPIF4_Msk          (0x1UL << EXTI_FPR1_FPIF4_Pos)            /*!< 0x00000010 */
#define EXTI_FPR1_FPIF4              EXTI_FPR1_FPIF4_Msk                       /*!< Falling Pending Interrupt Flag on line 4 */
#define EXTI_FPR1_FPIF5_Pos          (5U)
#define EXTI_FPR1_FPIF5_Msk          (0x1UL << EXTI_FPR1_FPIF5_Pos)            /*!< 0x00000020 */
#define EXTI_FPR1_FPIF5              EXTI_FPR1_FPIF5_Msk                       /*!< Falling Pending Interrupt Flag on line 5 */
#define EXTI_FPR1_FPIF6_Pos          (6U)
#define EXTI_FPR1_FPIF6_Msk          (0x1UL << EXTI_FPR1_FPIF6_Pos)            /*!< 0x00000040 */
#define EXTI_FPR1_FPIF6              EXTI_FPR1_FPIF6_Msk                       /*!< Falling Pending Interrupt Flag on line 6 */
#define EXTI_FPR1_FPIF7_Pos          (7U)
#define EXTI_FPR1_FPIF7_Msk          (0x1UL << EXTI_FPR1_FPIF7_Pos)            /*!< 0x00000080 */
#define EXTI_FPR1_FPIF7              EXTI_FPR1_FPIF7_Msk                       /*!< Falling Pending Interrupt Flag on line 7 */
#define EXTI_FPR1_FPIF8_Pos          (8U)
#define EXTI_FPR1_FPIF8_Msk          (0x1UL << EXTI_FPR1_FPIF8_Pos)            /*!< 0x00000100 */
#define EXTI_FPR1_FPIF8              EXTI_FPR1_FPIF8_Msk                       /*!< Falling Pending Interrupt Flag on line 8 */
#define EXTI_FPR1_FPIF9_Pos          (9U)
#define EXTI_FPR1_FPIF9_Msk          (0x1UL << EXTI_FPR1_FPIF9_Pos)            /*!< 0x00000200 */
#define EXTI_FPR1_FPIF9              EXTI_FPR1_FPIF9_Msk                       /*!< Falling Pending Interrupt Flag on line 9 */
#define EXTI_FPR1_FPIF10_Pos         (10U)
#define EXTI_FPR1_FPIF10_Msk         (0x1UL << EXTI_FPR1_FPIF10_Pos)           /*!< 0x00000400 */
#define EXTI_FPR1_FPIF10             EXTI_FPR1_FPIF10_Msk                      /*!< Falling Pending Interrupt Flag on line 10 */
#define EXTI_FPR1_FPIF11_Pos         (11U)
#define EXTI_FPR1_FPIF11_Msk         (0x1UL << EXTI_FPR1_FPIF11_Pos)           /*!< 0x00000800 */
#define EXTI_FPR1_FPIF11             EXTI_FPR1_FPIF11_Msk                      /*!< Falling Pending Interrupt Flag on line 11 */
#define EXTI_FPR1_FPIF12_Pos         (12U)
#define EXTI_FPR1_FPIF12_Msk         (0x1UL << EXTI_FPR1_FPIF12_Pos)           /*!< 0x00001000 */
#define EXTI_FPR1_FPIF12             EXTI_FPR1_FPIF12_Msk                      /*!< Falling Pending Interrupt Flag on line 12 */
#define EXTI_FPR1_FPIF13_Pos         (13U)
#define EXTI_FPR1_FPIF13_Msk         (0x1UL << EXTI_FPR1_FPIF13_Pos)           /*!< 0x00002000 */
#define EXTI_FPR1_FPIF13             EXTI_FPR1_FPIF13_Msk                      /*!< Falling Pending Interrupt Flag on line 13 */
#define EXTI_FPR1_FPIF14_Pos         (14U)
#define EXTI_FPR1_FPIF14_Msk         (0x1UL << EXTI_FPR1_FPIF14_Pos)           /*!< 0x00004000 */
#define EXTI_FPR1_FPIF14             EXTI_FPR1_FPIF14_Msk                      /*!< Falling Pending Interrupt Flag on line 14 */
#define EXTI_FPR1_FPIF15_Pos         (15U)
#define EXTI_FPR1_FPIF15_Msk         (0x1UL << EXTI_FPR1_FPIF15_Pos)           /*!< 0x00008000 */
#define EXTI_FPR1_FPIF15             EXTI_FPR1_FPIF15_Msk                      /*!< Falling Pending Interrupt Flag on line 15 */
#define EXTI_FPR1_FPIF16_Pos         (16U)
#define EXTI_FPR1_FPIF16_Msk         (0x1UL << EXTI_FPR1_FPIF16_Pos)           /*!< 0x00010000 */
#define EXTI_FPR1_FPIF16             EXTI_FPR1_FPIF16_Msk                      /*!< Falling Pending Interrupt Flag on line 16 */
#define EXTI_FPR1_FPIF17_Pos         (17U)
#define EXTI_FPR1_FPIF17_Msk         (0x1UL << EXTI_FPR1_FPIF17_Pos)           /*!< 0x00020000 */
#define EXTI_FPR1_FPIF17             EXTI_FPR1_FPIF17_Msk                      /*!< Falling Pending Interrupt Flag on line 17 */
#define EXTI_FPR1_FPIF18_Pos         (18U)
#define EXTI_FPR1_FPIF18_Msk         (0x1UL << EXTI_FPR1_FPIF18_Pos)           /*!< 0x00040000 */
#define EXTI_FPR1_FPIF18             EXTI_FPR1_FPIF18_Msk                      /*!< Falling Pending Interrupt Flag on line 18 */

/*****************  Bit definition for EXTI_EXTICR1 register  **************/
#define EXTI_EXTICR1_EXTI0_Pos       (0U)
#define EXTI_EXTICR1_EXTI0_Msk       (0x7UL << EXTI_EXTICR1_EXTI0_Pos)         /*!< 0x00000007 */
#define EXTI_EXTICR1_EXTI0           EXTI_EXTICR1_EXTI0_Msk                    /*!< EXTI 0 configuration */
#define EXTI_EXTICR1_EXTI0_0         (0x1UL << EXTI_EXTICR1_EXTI0_Pos)         /*!< 0x00000001 */
#define EXTI_EXTICR1_EXTI0_1         (0x2UL << EXTI_EXTICR1_EXTI0_Pos)         /*!< 0x00000002 */
#define EXTI_EXTICR1_EXTI0_2         (0x4UL << EXTI_EXTICR1_EXTI0_Pos)         /*!< 0x00000004 */
#define EXTI_EXTICR1_EXTI1_Pos       (8U)
#define EXTI_EXTICR1_EXTI1_Msk       (0x7UL << EXTI_EXTICR1_EXTI1_Pos)         /*!< 0x00000700 */
#define EXTI_EXTICR1_EXTI1           EXTI_EXTICR1_EXTI1_Msk                    /*!< EXTI 1 configuration */
#define EXTI_EXTICR1_EXTI1_0         (0x1UL << EXTI_EXTICR1_EXTI1_Pos)         /*!< 0x00000100 */
#define EXTI_EXTICR1_EXTI1_1         (0x2UL << EXTI_EXTICR1_EXTI1_Pos)         /*!< 0x00000200 */
#define EXTI_EXTICR1_EXTI1_2         (0x4UL << EXTI_EXTICR1_EXTI1_Pos)         /*!< 0x00000400 */
#define EXTI_EXTICR1_EXTI2_Pos       (16U)
#define EXTI_EXTICR1_EXTI2_Msk       (0x7UL << EXTI_EXTICR1_EXTI2_Pos)         /*!< 0x00070000 */
#define EXTI_EXTICR1_EXTI2           EXTI_EXTICR1_EXTI2_Msk                    /*!< EXTI 2 configuration */
#define EXTI_EXTICR1_EXTI2_0         (0x1UL << EXTI_EXTICR1_EXTI2_Pos)         /*!< 0x00010000 */
#define EXTI_EXTICR1_EXTI2_1         (0x2UL << EXTI_EXTICR1_EXTI2_Pos)         /*!< 0x00020000 */
#define EXTI_EXTICR1_EXTI2_2         (0x4UL << EXTI_EXTICR1_EXTI2_Pos)         /*!< 0x00040000 */
#define EXTI_EXTICR1_EXTI3_Pos       (24U)
#define EXTI_EXTICR1_EXTI3_Msk       (0x7UL << EXTI_EXTICR1_EXTI3_Pos)         /*!< 0x07000000 */
#define EXTI_EXTICR1_EXTI3           EXTI_EXTICR1_EXTI3_Msk                    /*!< EXTI 3 configuration */
#define EXTI_EXTICR1_EXTI3_0         (0x1UL << EXTI_EXTICR1_EXTI3_Pos)         /*!< 0x01000000 */
#define EXTI_EXTICR1_EXTI3_1         (0x2UL << EXTI_EXTICR1_EXTI3_Pos)         /*!< 0x02000000 */
#define EXTI_EXTICR1_EXTI3_2         (0x4UL << EXTI_EXTICR1_EXTI3_Pos)         /*!< 0x04000000 */

/*****************  Bit definition for EXTI_EXTICR2 register  **************/
#define EXTI_EXTICR2_EXTI4_Pos       (0U)
#define EXTI_EXTICR2_EXTI4_Msk       (0x7UL << EXTI_EXTICR2_EXTI4_Pos)         /*!< 0x00000007 */
#define EXTI_EXTICR2_EXTI4           EXTI_EXTICR2_EXTI4_Msk                    /*!< EXTI 4 configuration */
#define EXTI_EXTICR2_EXTI4_0         (0x1UL << EXTI_EXTICR2_EXTI4_Pos)         /*!< 0x00000001 */
#define EXTI_EXTICR2_EXTI4_1         (0x2UL << EXTI_EXTICR2_EXTI4_Pos)         /*!< 0x00000002 */
#define EXTI_EXTICR2_EXTI4_2         (0x4UL << EXTI_EXTICR2_EXTI4_Pos)         /*!< 0x00000004 */
#define EXTI_EXTICR2_EXTI5_Pos       (8U)
#define EXTI_EXTICR2_EXTI5_Msk       (0x7UL << EXTI_EXTICR2_EXTI5_Pos)         /*!< 0x00000700 */
#define EXTI_EXTICR2_EXTI5           EXTI_EXTICR2_EXTI5_Msk                    /*!< EXTI 5 configuration */
#define EXTI_EXTICR2_EXTI5_0         (0x1UL << EXTI_EXTICR2_EXTI5_Pos)         /*!< 0x00000100 */
#define EXTI_EXTICR2_EXTI5_1         (0x2UL << EXTI_EXTICR2_EXTI5_Pos)         /*!< 0x00000200 */
#define EXTI_EXTICR2_EXTI5_2         (0x4UL << EXTI_EXTICR2_EXTI5_Pos)         /*!< 0x00000400 */
#define EXTI_EXTICR2_EXTI6_Pos       (16U)
#define EXTI_EXTICR2_EXTI6_Msk       (0x7UL << EXTI_EXTICR2_EXTI6_Pos)         /*!< 0x00070000 */
#define EXTI_EXTICR2_EXTI6           EXTI_EXTICR2_EXTI6_Msk                    /*!< EXTI 6 configuration */
#define EXTI_EXTICR2_EXTI6_0         (0x1UL << EXTI_EXTICR2_EXTI6_Pos)         /*!< 0x00010000 */
#define EXTI_EXTICR2_EXTI6_1         (0x2UL << EXTI_EXTICR2_EXTI6_Pos)         /*!< 0x00020000 */
#define EXTI_EXTICR2_EXTI6_2         (0x4UL << EXTI_EXTICR2_EXTI6_Pos)         /*!< 0x00040000 */
#define EXTI_EXTICR2_EXTI7_Pos       (24U)
#define EXTI_EXTICR2_EXTI7_Msk       (0x7UL << EXTI_EXTICR2_EXTI7_Pos)         /*!< 0x07000000 */
#define EXTI_EXTICR2_EXTI7           EXTI_EXTICR2_EXTI7_Msk                    /*!< EXTI 7 configuration */
#define EXTI_EXTICR2_EXTI7_0         (0x1UL << EXTI_EXTICR2_EXTI7_Pos)         /*!< 0x01000000 */
#define EXTI_EXTICR2_EXTI7_1         (0x2UL << EXTI_EXTICR2_EXTI7_Pos)         /*!< 0x02000000 */
#define EXTI_EXTICR2_EXTI7_2         (0x4UL << EXTI_EXTICR2_EXTI7_Pos)         /*!< 0x04000000 */

/*****************  Bit definition for EXTI_EXTICR3 register  **************/
#define EXTI_EXTICR3_EXTI8_Pos       (0U)
#define EXTI_EXTICR3_EXTI8_Msk       (0x7UL << EXTI_EXTICR3_EXTI8_Pos)         /*!< 0x00000007 */
#define EXTI_EXTICR3_EXTI8           EXTI_EXTICR3_EXTI8_Msk                    /*!< EXTI 8 configuration */
#define EXTI_EXTICR3_EXTI8_0         (0x1UL << EXTI_EXTICR3_EXTI8_Pos)         /*!< 0x00000001 */
#define EXTI_EXTICR3_EXTI8_1         (0x2UL << EXTI_EXTICR3_EXTI8_Pos)         /*!< 0x00000002 */
#define EXTI_EXTICR3_EXTI8_2         (0x4UL << EXTI_EXTICR3_EXTI8_Pos)         /*!< 0x00000004 */
#define EXTI_EXTICR3_EXTI9_Pos       (8U)
#define EXTI_EXTICR3_EXTI9_Msk       (0x7UL << EXTI_EXTICR3_EXTI9_Pos)         /*!< 0x00000700 */
#define EXTI_EXTICR3_EXTI9           EXTI_EXTICR3_EXTI9_Msk                    /*!< EXTI 9 configuration */
#define EXTI_EXTICR3_EXTI9_0         (0x1UL << EXTI_EXTICR3_EXTI9_Pos)         /*!< 0x00000100 */
#define EXTI_EXTICR3_EXTI9_1         (0x2UL << EXTI_EXTICR3_EXTI9_Pos)         /*!< 0x00000200 */
#define EXTI_EXTICR3_EXTI9_2         (0x4UL << EXTI_EXTICR3_EXTI9_Pos)         /*!< 0x00000400 */
#define EXTI_EXTICR3_EXTI10_Pos      (16U)
#define EXTI_EXTICR3_EXTI10_Msk      (0x7UL << EXTI_EXTICR3_EXTI10_Pos)        /*!< 0x00070000 */
#define EXTI_EXTICR3_EXTI10          EXTI_EXTICR3_EXTI10_Msk                   /*!< EXTI 10 configuration */
#define EXTI_EXTICR3_EXTI10_0        (0x1UL << EXTI_EXTICR3_EXTI10_Pos)        /*!< 0x00010000 */
#define EXTI_EXTICR3_EXTI10_1        (0x2UL << EXTI_EXTICR3_EXTI10_Pos)        /*!< 0x00020000 */
#define EXTI_EXTICR3_EXTI10_2        (0x4UL << EXTI_EXTICR3_EXTI10_Pos)        /*!< 0x00040000 */
#define EXTI_EXTICR3_EXTI11_Pos      (24U)
#define EXTI_EXTICR3_EXTI11_Msk      (0x7UL << EXTI_EXTICR3_EXTI11_Pos)        /*!< 0x07000000 */
#define EXTI_EXTICR3_EXTI11          EXTI_EXTICR3_EXTI11_Msk                   /*!< EXTI 11 configuration */
#define EXTI_EXTICR3_EXTI11_0        (0x1UL << EXTI_EXTICR3_EXTI11_Pos)        /*!< 0x01000000 */
#define EXTI_EXTICR3_EXTI11_1        (0x2UL << EXTI_EXTICR3_EXTI11_Pos)        /*!< 0x02000000 */
#define EXTI_EXTICR3_EXTI11_2        (0x4UL << EXTI_EXTICR3_EXTI11_Pos)        /*!< 0x04000000 */

/*****************  Bit definition for EXTI_EXTICR4 register  **************/
#define EXTI_EXTICR4_EXTI12_Pos      (0U)
#define EXTI_EXTICR4_EXTI12_Msk      (0x7UL << EXTI_EXTICR4_EXTI12_Pos)        /*!< 0x00000007 */
#define EXTI_EXTICR4_EXTI12          EXTI_EXTICR4_EXTI12_Msk                   /*!< EXTI 12 configuration */
#define EXTI_EXTICR4_EXTI12_0        (0x1UL << EXTI_EXTICR4_EXTI12_Pos)        /*!< 0x00000001 */
#define EXTI_EXTICR4_EXTI12_1        (0x2UL << EXTI_EXTICR4_EXTI12_Pos)        /*!< 0x00000002 */
#define EXTI_EXTICR4_EXTI12_2        (0x4UL << EXTI_EXTICR4_EXTI12_Pos)        /*!< 0x00000004 */
#define EXTI_EXTICR4_EXTI13_Pos      (8U)
#define EXTI_EXTICR4_EXTI13_Msk      (0x7UL << EXTI_EXTICR4_EXTI13_Pos)        /*!< 0x00000700 */
#define EXTI_EXTICR4_EXTI13          EXTI_EXTICR4_EXTI13_Msk                   /*!< EXTI 13 configuration */
#define EXTI_EXTICR4_EXTI13_0        (0x1UL << EXTI_EXTICR4_EXTI13_Pos)        /*!< 0x00000100 */
#define EXTI_EXTICR4_EXTI13_1        (0x2UL << EXTI_EXTICR4_EXTI13_Pos)       /*!< 0x00000200 */
#define EXTI_EXTICR4_EXTI13_2        (0x4UL << EXTI_EXTICR4_EXTI13_Pos)         /*!< 0x00000400 */
#define EXTI_EXTICR4_EXTI14_Pos      (16U)
#define EXTI_EXTICR4_EXTI14_Msk      (0x7UL << EXTI_EXTICR4_EXTI14_Pos)        /*!< 0x00070000 */
#define EXTI_EXTICR4_EXTI14          EXTI_EXTICR4_EXTI14_Msk                   /*!< EXTI 14 configuration */
#define EXTI_EXTICR4_EXTI14_0        (0x1UL << EXTI_EXTICR4_EXTI14_Pos)        /*!< 0x00010000 */
#define EXTI_EXTICR4_EXTI14_1        (0x2UL << EXTI_EXTICR4_EXTI14_Pos)        /*!< 0x00020000 */
#define EXTI_EXTICR4_EXTI14_2        (0x4UL << EXTI_EXTICR4_EXTI14_Pos)        /*!< 0x00040000 */
#define EXTI_EXTICR4_EXTI15_Pos      (24U)
#define EXTI_EXTICR4_EXTI15_Msk      (0x7UL << EXTI_EXTICR4_EXTI15_Pos)        /*!< 0x07000000 */
#define EXTI_EXTICR4_EXTI15          EXTI_EXTICR4_EXTI15_Msk                   /*!< EXTI 15 configuration */
#define EXTI_EXTICR4_EXTI15_0        (0x1UL << EXTI_EXTICR4_EXTI15_Pos)        /*!< 0x01000000 */
#define EXTI_EXTICR4_EXTI15_1        (0x2UL << EXTI_EXTICR4_EXTI15_Pos)        /*!< 0x02000000 */
#define EXTI_EXTICR4_EXTI15_2        (0x4UL << EXTI_EXTICR4_EXTI15_Pos)        /*!< 0x04000000 */

/*******************  Bit definition for EXTI_IMR1 register  ******************/
#define EXTI_IMR1_IM0_Pos            (0U)
#define EXTI_IMR1_IM0_Msk            (0x1UL << EXTI_IMR1_IM0_Pos)              /*!< 0x00000001 */
#define EXTI_IMR1_IM0                EXTI_IMR1_IM0_Msk                         /*!< Interrupt Mask on line 0 */
#define EXTI_IMR1_IM1_Pos            (1U)
#define EXTI_IMR1_IM1_Msk            (0x1UL << EXTI_IMR1_IM1_Pos)              /*!< 0x00000002 */
#define EXTI_IMR1_IM1                EXTI_IMR1_IM1_Msk                         /*!< Interrupt Mask on line 1 */
#define EXTI_IMR1_IM2_Pos            (2U)
#define EXTI_IMR1_IM2_Msk            (0x1UL << EXTI_IMR1_IM2_Pos)              /*!< 0x00000004 */
#define EXTI_IMR1_IM2                EXTI_IMR1_IM2_Msk                         /*!< Interrupt Mask on line 2 */
#define EXTI_IMR1_IM3_Pos            (3U)
#define EXTI_IMR1_IM3_Msk            (0x1UL << EXTI_IMR1_IM3_Pos)              /*!< 0x00000008 */
#define EXTI_IMR1_IM3                EXTI_IMR1_IM3_Msk                         /*!< Interrupt Mask on line 3 */
#define EXTI_IMR1_IM4_Pos            (4U)
#define EXTI_IMR1_IM4_Msk            (0x1UL << EXTI_IMR1_IM4_Pos)              /*!< 0x00000010 */
#define EXTI_IMR1_IM4                EXTI_IMR1_IM4_Msk                         /*!< Interrupt Mask on line 4 */
#define EXTI_IMR1_IM5_Pos            (5U)
#define EXTI_IMR1_IM5_Msk            (0x1UL << EXTI_IMR1_IM5_Pos)              /*!< 0x00000020 */
#define EXTI_IMR1_IM5                EXTI_IMR1_IM5_Msk                         /*!< Interrupt Mask on line 5 */
#define EXTI_IMR1_IM6_Pos            (6U)
#define EXTI_IMR1_IM6_Msk            (0x1UL << EXTI_IMR1_IM6_Pos)              /*!< 0x00000040 */
#define EXTI_IMR1_IM6                EXTI_IMR1_IM6_Msk                         /*!< Interrupt Mask on line 6 */
#define EXTI_IMR1_IM7_Pos            (7U)
#define EXTI_IMR1_IM7_Msk            (0x1UL << EXTI_IMR1_IM7_Pos)              /*!< 0x00000080 */
#define EXTI_IMR1_IM7                EXTI_IMR1_IM7_Msk                         /*!< Interrupt Mask on line 7 */
#define EXTI_IMR1_IM8_Pos            (8U)
#define EXTI_IMR1_IM8_Msk            (0x1UL << EXTI_IMR1_IM8_Pos)              /*!< 0x00000100 */
#define EXTI_IMR1_IM8                EXTI_IMR1_IM8_Msk                         /*!< Interrupt Mask on line 8 */
#define EXTI_IMR1_IM9_Pos            (9U)
#define EXTI_IMR1_IM9_Msk            (0x1UL << EXTI_IMR1_IM9_Pos)              /*!< 0x00000200 */
#define EXTI_IMR1_IM9                EXTI_IMR1_IM9_Msk                         /*!< Interrupt Mask on line 9 */
#define EXTI_IMR1_IM10_Pos           (10U)
#define EXTI_IMR1_IM10_Msk           (0x1UL << EXTI_IMR1_IM10_Pos)             /*!< 0x00000400 */
#define EXTI_IMR1_IM10               EXTI_IMR1_IM10_Msk                        /*!< Interrupt Mask on line 10 */
#define EXTI_IMR1_IM11_Pos           (11U)
#define EXTI_IMR1_IM11_Msk           (0x1UL << EXTI_IMR1_IM11_Pos)             /*!< 0x00000800 */
#define EXTI_IMR1_IM11               EXTI_IMR1_IM11_Msk                        /*!< Interrupt Mask on line 11 */
#define EXTI_IMR1_IM12_Pos           (12U)
#define EXTI_IMR1_IM12_Msk           (0x1UL << EXTI_IMR1_IM12_Pos)             /*!< 0x00001000 */
#define EXTI_IMR1_IM12               EXTI_IMR1_IM12_Msk                        /*!< Interrupt Mask on line 12 */
#define EXTI_IMR1_IM13_Pos           (13U)
#define EXTI_IMR1_IM13_Msk           (0x1UL << EXTI_IMR1_IM13_Pos)             /*!< 0x00002000 */
#define EXTI_IMR1_IM13               EXTI_IMR1_IM13_Msk                        /*!< Interrupt Mask on line 13 */
#define EXTI_IMR1_IM14_Pos           (14U)
#define EXTI_IMR1_IM14_Msk           (0x1UL << EXTI_IMR1_IM14_Pos)             /*!< 0x00004000 */
#define EXTI_IMR1_IM14               EXTI_IMR1_IM14_Msk                        /*!< Interrupt Mask on line 14 */
#define EXTI_IMR1_IM15_Pos           (15U)
#define EXTI_IMR1_IM15_Msk           (0x1UL << EXTI_IMR1_IM15_Pos)             /*!< 0x00008000 */
#define EXTI_IMR1_IM15               EXTI_IMR1_IM15_Msk                        /*!< Interrupt Mask on line 15 */
#define EXTI_IMR1_IM16_Pos           (16U)
#define EXTI_IMR1_IM16_Msk           (0x1UL << EXTI_IMR1_IM16_Pos)             /*!< 0x00010000 */
#define EXTI_IMR1_IM16               EXTI_IMR1_IM16_Msk                        /*!< Interrupt Mask on line 16 */
#define EXTI_IMR1_IM17_Pos           (17U)
#define EXTI_IMR1_IM17_Msk           (0x1UL << EXTI_IMR1_IM17_Pos)             /*!< 0x00020000 */
#define EXTI_IMR1_IM17               EXTI_IMR1_IM17_Msk                        /*!< Interrupt Mask on line 17 */
#define EXTI_IMR1_IM18_Pos           (18U)
#define EXTI_IMR1_IM18_Msk           (0x1UL << EXTI_IMR1_IM18_Pos)             /*!< 0x00040000 */
#define EXTI_IMR1_IM18               EXTI_IMR1_IM18_Msk                        /*!< Interrupt Mask on line 18 */
#define EXTI_IMR1_IM19_Pos           (19U)
#define EXTI_IMR1_IM19_Msk           (0x1UL << EXTI_IMR1_IM19_Pos)             /*!< 0x00080000 */
#define EXTI_IMR1_IM19               EXTI_IMR1_IM19_Msk                        /*!< Interrupt Mask on line 19 */
#define EXTI_IMR1_IM20_Pos           (20U)
#define EXTI_IMR1_IM20_Msk           (0x1UL << EXTI_IMR1_IM20_Pos)             /*!< 0x00100000 */
#define EXTI_IMR1_IM20               EXTI_IMR1_IM20_Msk                        /*!< Interrupt Mask on line 20 */
#define EXTI_IMR1_IM21_Pos           (21U)
#define EXTI_IMR1_IM21_Msk           (0x1UL << EXTI_IMR1_IM21_Pos)             /*!< 0x00200000 */
#define EXTI_IMR1_IM21               EXTI_IMR1_IM21_Msk                        /*!< Interrupt Mask on line 21 */
#define EXTI_IMR1_IM22_Pos           (22U)
#define EXTI_IMR1_IM22_Msk           (0x1UL << EXTI_IMR1_IM22_Pos)             /*!< 0x00400000 */
#define EXTI_IMR1_IM22               EXTI_IMR1_IM22_Msk                        /*!< Interrupt Mask on line 22 */
#define EXTI_IMR1_IM23_Pos           (23U)
#define EXTI_IMR1_IM23_Msk           (0x1UL << EXTI_IMR1_IM23_Pos)             /*!< 0x00800000 */
#define EXTI_IMR1_IM23               EXTI_IMR1_IM23_Msk                        /*!< Interrupt Mask on line 23 */
#define EXTI_IMR1_IM24_Pos           (24U)
#define EXTI_IMR1_IM24_Msk           (0x1UL << EXTI_IMR1_IM24_Pos)             /*!< 0x01000000 */
#define EXTI_IMR1_IM24               EXTI_IMR1_IM24_Msk                        /*!< Interrupt Mask on line 24 */
#define EXTI_IMR1_IM25_Pos           (25U)
#define EXTI_IMR1_IM25_Msk           (0x1UL << EXTI_IMR1_IM25_Pos)             /*!< 0x02000000 */
#define EXTI_IMR1_IM25               EXTI_IMR1_IM25_Msk                        /*!< Interrupt Mask on line 25 */
#define EXTI_IMR1_IM26_Pos           (26U)
#define EXTI_IMR1_IM26_Msk           (0x1UL << EXTI_IMR1_IM26_Pos)             /*!< 0x04000000 */
#define EXTI_IMR1_IM26               EXTI_IMR1_IM26_Msk                        /*!< Interrupt Mask on line 26 */
#define EXTI_IMR1_IM27_Pos           (27U)
#define EXTI_IMR1_IM27_Msk           (0x1UL << EXTI_IMR1_IM27_Pos)             /*!< 0x08000000 */
#define EXTI_IMR1_IM27               EXTI_IMR1_IM27_Msk                        /*!< Interrupt Mask on line 27 */
#define EXTI_IMR1_IM28_Pos           (28U)
#define EXTI_IMR1_IM28_Msk           (0x1UL << EXTI_IMR1_IM28_Pos)             /*!< 0x10000000 */
#define EXTI_IMR1_IM28               EXTI_IMR1_IM28_Msk                        /*!< Interrupt Mask on line 28 */
#define EXTI_IMR1_IM29_Pos           (29U)
#define EXTI_IMR1_IM29_Msk           (0x1UL << EXTI_IMR1_IM29_Pos)             /*!< 0x20000000 */
#define EXTI_IMR1_IM29               EXTI_IMR1_IM29_Msk                        /*!< Interrupt Mask on line 29 */
#define EXTI_IMR1_IM30_Pos           (30U)
#define EXTI_IMR1_IM30_Msk           (0x1UL << EXTI_IMR1_IM30_Pos)             /*!< 0x40000000 */
#define EXTI_IMR1_IM30               EXTI_IMR1_IM30_Msk                        /*!< Interrupt Mask on line 30 */
#define EXTI_IMR1_IM31_Pos           (31U)
#define EXTI_IMR1_IM31_Msk           (0x1UL << EXTI_IMR1_IM31_Pos)              /*!< 0x80000000 */
#define EXTI_IMR1_IM31               EXTI_IMR1_IM31_Msk                        /*!< Interrupt Mask on line 31 */
#define EXTI_IMR1_IM_Pos             (0U)
#define EXTI_IMR1_IM_Msk             (0xFEAFFFFFUL << EXTI_IMR1_IM_Pos)        /*!< 0xFEAFFFFF */
#define EXTI_IMR1_IM                 EXTI_IMR1_IM_Msk                          /*!< Interrupt Mask All */

/*******************  Bit definition for EXTI_IMR2 register  ******************/
#define EXTI_IMR2_IM32_Pos           (0U)
#define EXTI_IMR2_IM32_Msk           (0x1UL << EXTI_IMR2_IM32_Pos)             /*!< 0x00000001 */
#define EXTI_IMR2_IM32               EXTI_IMR2_IM32_Msk                        /*!< Interrupt Mask on line 32 */
#define EXTI_IMR2_IM33_Pos           (1U)
#define EXTI_IMR2_IM33_Msk           (0x1UL << EXTI_IMR2_IM33_Pos)             /*!< 0x00000002 */
#define EXTI_IMR2_IM33               EXTI_IMR2_IM33_Msk                        /*!< Interrupt Mask on line 33 */
#define EXTI_IMR2_IM_Pos             (0U)
#define EXTI_IMR2_IM_Msk             (0x3UL << EXTI_IMR2_IM_Pos)               /*!< 0x00000003 */
#define EXTI_IMR2_IM                 EXTI_IMR2_IM_Msk                          /*!< Interrupt Mask All */

/*******************  Bit definition for EXTI_EMR1 register  ******************/
#define EXTI_EMR1_EM0_Pos            (0U)
#define EXTI_EMR1_EM0_Msk            (0x1UL << EXTI_EMR1_EM0_Pos)              /*!< 0x00000001 */
#define EXTI_EMR1_EM0                EXTI_EMR1_EM0_Msk                         /*!< Event Mask on line 0 */
#define EXTI_EMR1_EM1_Pos            (1U)
#define EXTI_EMR1_EM1_Msk            (0x1UL << EXTI_EMR1_EM1_Pos)              /*!< 0x00000002 */
#define EXTI_EMR1_EM1                EXTI_EMR1_EM1_Msk                         /*!< Event Mask on line 1 */
#define EXTI_EMR1_EM2_Pos            (2U)
#define EXTI_EMR1_EM2_Msk            (0x1UL << EXTI_EMR1_EM2_Pos)              /*!< 0x00000004 */
#define EXTI_EMR1_EM2                EXTI_EMR1_EM2_Msk                         /*!< Event Mask on line 2 */
#define EXTI_EMR1_EM3_Pos            (3U)
#define EXTI_EMR1_EM3_Msk            (0x1UL << EXTI_EMR1_EM3_Pos)              /*!< 0x00000008 */
#define EXTI_EMR1_EM3                EXTI_EMR1_EM3_Msk                         /*!< Event Mask on line 3 */
#define EXTI_EMR1_EM4_Pos            (4U)
#define EXTI_EMR1_EM4_Msk            (0x1UL << EXTI_EMR1_EM4_Pos)              /*!< 0x00000010 */
#define EXTI_EMR1_EM4                EXTI_EMR1_EM4_Msk                         /*!< Event Mask on line 4 */
#define EXTI_EMR1_EM5_Pos            (5U)
#define EXTI_EMR1_EM5_Msk            (0x1UL << EXTI_EMR1_EM5_Pos)              /*!< 0x00000020 */
#define EXTI_EMR1_EM5                EXTI_EMR1_EM5_Msk                         /*!< Event Mask on line 5 */
#define EXTI_EMR1_EM6_Pos            (6U)
#define EXTI_EMR1_EM6_Msk            (0x1UL << EXTI_EMR1_EM6_Pos)              /*!< 0x00000040 */
#define EXTI_EMR1_EM6                EXTI_EMR1_EM6_Msk                         /*!< Event Mask on line 6 */
#define EXTI_EMR1_EM7_Pos            (7U)
#define EXTI_EMR1_EM7_Msk            (0x1UL << EXTI_EMR1_EM7_Pos)              /*!< 0x00000080 */
#define EXTI_EMR1_EM7                EXTI_EMR1_EM7_Msk                         /*!< Event Mask on line 7 */
#define EXTI_EMR1_EM8_Pos            (8U)
#define EXTI_EMR1_EM8_Msk            (0x1UL << EXTI_EMR1_EM8_Pos)              /*!< 0x00000100 */
#define EXTI_EMR1_EM8                EXTI_EMR1_EM8_Msk                         /*!< Event Mask on line 8 */
#define EXTI_EMR1_EM9_Pos            (9U)
#define EXTI_EMR1_EM9_Msk            (0x1UL << EXTI_EMR1_EM9_Pos)              /*!< 0x00000200 */
#define EXTI_EMR1_EM9                EXTI_EMR1_EM9_Msk                         /*!< Event Mask on line 9 */
#define EXTI_EMR1_EM10_Pos           (10U)
#define EXTI_EMR1_EM10_Msk           (0x1UL << EXTI_EMR1_EM10_Pos)             /*!< 0x00000400 */
#define EXTI_EMR1_EM10               EXTI_EMR1_EM10_Msk                        /*!< Event Mask on line 10 */
#define EXTI_EMR1_EM11_Pos           (11U)
#define EXTI_EMR1_EM11_Msk           (0x1UL << EXTI_EMR1_EM11_Pos)             /*!< 0x00000800 */
#define EXTI_EMR1_EM11               EXTI_EMR1_EM11_Msk                        /*!< Event Mask on line 11 */
#define EXTI_EMR1_EM12_Pos           (12U)
#define EXTI_EMR1_EM12_Msk           (0x1UL << EXTI_EMR1_EM12_Pos)             /*!< 0x00001000 */
#define EXTI_EMR1_EM12               EXTI_EMR1_EM12_Msk                        /*!< Event Mask on line 12 */
#define EXTI_EMR1_EM13_Pos           (13U)
#define EXTI_EMR1_EM13_Msk           (0x1UL << EXTI_EMR1_EM13_Pos)             /*!< 0x00002000 */
#define EXTI_EMR1_EM13               EXTI_EMR1_EM13_Msk                        /*!< Event Mask on line 13 */
#define EXTI_EMR1_EM14_Pos           (14U)
#define EXTI_EMR1_EM14_Msk           (0x1UL << EXTI_EMR1_EM14_Pos)             /*!< 0x00004000 */
#define EXTI_EMR1_EM14               EXTI_EMR1_EM14_Msk                        /*!< Event Mask on line 14 */
#define EXTI_EMR1_EM15_Pos           (15U)
#define EXTI_EMR1_EM15_Msk           (0x1UL << EXTI_EMR1_EM15_Pos)             /*!< 0x00008000 */
#define EXTI_EMR1_EM15               EXTI_EMR1_EM15_Msk                        /*!< Event Mask on line 15 */
#define EXTI_EMR1_EM16_Pos           (16U)
#define EXTI_EMR1_EM16_Msk           (0x1UL << EXTI_EMR1_EM16_Pos)             /*!< 0x00010000 */
#define EXTI_EMR1_EM16               EXTI_EMR1_EM16_Msk                        /*!< Event Mask on line 16 */
#define EXTI_EMR1_EM17_Pos           (17U)
#define EXTI_EMR1_EM17_Msk           (0x1UL << EXTI_EMR1_EM17_Pos)             /*!< 0x00020000 */
#define EXTI_EMR1_EM17               EXTI_EMR1_EM17_Msk                        /*!< Event Mask on line 17 */
#define EXTI_EMR1_EM18_Pos           (18U)
#define EXTI_EMR1_EM18_Msk           (0x1UL << EXTI_EMR1_EM18_Pos)             /*!< 0x00040000 */
#define EXTI_EMR1_EM18               EXTI_EMR1_EM18_Msk                        /*!< Event Mask on line 18 */
#define EXTI_EMR1_EM19_Pos           (19U)
#define EXTI_EMR1_EM19_Msk           (0x1UL << EXTI_EMR1_EM19_Pos)             /*!< 0x00080000 */
#define EXTI_EMR1_EM19               EXTI_EMR1_EM19_Msk                        /*!< Event Mask on line 19 */
#define EXTI_EMR1_EM21_Pos           (21U)
#define EXTI_EMR1_EM21_Msk           (0x1UL << EXTI_EMR1_EM21_Pos)             /*!< 0x00200000 */
#define EXTI_EMR1_EM21               EXTI_EMR1_EM21_Msk                        /*!< Event Mask on line 21 */
#define EXTI_EMR1_EM23_Pos           (23U)
#define EXTI_EMR1_EM23_Msk           (0x1UL << EXTI_EMR1_EM23_Pos)             /*!< 0x00800000 */
#define EXTI_EMR1_EM23               EXTI_EMR1_EM23_Msk                        /*!< Event Mask on line 23 */
#define EXTI_EMR1_EM25_Pos           (25U)
#define EXTI_EMR1_EM25_Msk           (0x1UL << EXTI_EMR1_EM25_Pos)             /*!< 0x02000000 */
#define EXTI_EMR1_EM25               EXTI_EMR1_EM25_Msk                        /*!< Event Mask on line 25 */
#define EXTI_EMR1_EM26_Pos           (26U)
#define EXTI_EMR1_EM26_Msk           (0x1UL << EXTI_EMR1_EM26_Pos)             /*!< 0x04000000 */
#define EXTI_EMR1_EM26               EXTI_EMR1_EM26_Msk                        /*!< Event Mask on line 26 */
#define EXTI_EMR1_EM27_Pos           (27U)
#define EXTI_EMR1_EM27_Msk           (0x1UL << EXTI_EMR1_EM27_Pos)             /*!< 0x08000000 */
#define EXTI_EMR1_EM27               EXTI_EMR1_EM27_Msk                        /*!< Event Mask on line 27 */
#define EXTI_EMR1_EM28_Pos           (28U)
#define EXTI_EMR1_EM28_Msk           (0x1UL << EXTI_EMR1_EM28_Pos)             /*!< 0x10000000 */
#define EXTI_EMR1_EM28               EXTI_EMR1_EM28_Msk                        /*!< Event Mask on line 28 */
#define EXTI_EMR1_EM29_Pos           (29U)
#define EXTI_EMR1_EM29_Msk           (0x1UL << EXTI_EMR1_EM29_Pos)             /*!< 0x20000000 */
#define EXTI_EMR1_EM29               EXTI_EMR1_EM29_Msk                        /*!< Event Mask on line 29 */
#define EXTI_EMR1_EM30_Pos           (30U)
#define EXTI_EMR1_EM30_Msk           (0x1UL << EXTI_EMR1_EM30_Pos)             /*!< 0x40000000 */
#define EXTI_EMR1_EM30               EXTI_EMR1_EM30_Msk                        /*!< Event Mask on line 30 */
#define EXTI_EMR1_EM31_Pos           (31U)
#define EXTI_EMR1_EM31_Msk           (0x1UL << EXTI_EMR1_EM31_Pos)             /*!< 0x80000000 */
#define EXTI_EMR1_EM31               EXTI_EMR1_EM31_Msk                        /*!< Event Mask on line 31 */

/*******************  Bit definition for EXTI_EMR2 register  ******************/
#define EXTI_EMR2_EM32_Pos           (0U)
#define EXTI_EMR2_EM32_Msk           (0x1UL << EXTI_EMR2_EM32_Pos)             /*!< 0x00000001 */
#define EXTI_EMR2_EM32               EXTI_EMR2_EM32_Msk                        /*!< Event Mask on line 32 */
#define EXTI_EMR2_EM33_Pos           (1U)
#define EXTI_EMR2_EM33_Msk           (0x1UL << EXTI_EMR2_EM33_Pos)             /*!< 0x00000002 */
#define EXTI_EMR2_EM33               EXTI_EMR2_EM33_Msk                        /*!< Event Mask on line 33 */


/******************************************************************************/
/*                                                                            */
/*                                    FLASH                                   */
/*                                                                            */
/******************************************************************************/
#define GPIO_NRST_CONFIG_SUPPORT         /*!< GPIO feature available only on specific devices: Configure NRST pin */
#define FLASH_SECURABLE_MEMORY_SUPPORT   /*!< Flash feature available only on specific devices: allow to secure memory */
#define FLASH_PCROP_SUPPORT              /*!< Flash feature available only on specific devices: proprietary code read protection areas selected by option */

/*******************  Bits definition for FLASH_ACR register  *****************/
#define FLASH_ACR_LATENCY_Pos                  (0U)
#define FLASH_ACR_LATENCY_Msk                  (0x7UL << FLASH_ACR_LATENCY_Pos) /*!< 0x00000007 */
#define FLASH_ACR_LATENCY                      FLASH_ACR_LATENCY_Msk
#define FLASH_ACR_LATENCY_0                    (0x1UL << FLASH_ACR_LATENCY_Pos) /*!< 0x00000001 */
#define FLASH_ACR_LATENCY_1                    (0x2UL << FLASH_ACR_LATENCY_Pos) /*!< 0x00000002 */
#define FLASH_ACR_LATENCY_2                    (0x4UL << FLASH_ACR_LATENCY_Pos) /*!< 0x00000004 */
#define FLASH_ACR_PRFTEN_Pos                   (8U)
#define FLASH_ACR_PRFTEN_Msk                   (0x1UL << FLASH_ACR_PRFTEN_Pos)  /*!< 0x00000100 */
#define FLASH_ACR_PRFTEN                       FLASH_ACR_PRFTEN_Msk
#define FLASH_ACR_ICEN_Pos                     (9U)
#define FLASH_ACR_ICEN_Msk                     (0x1UL << FLASH_ACR_ICEN_Pos)    /*!< 0x00000200 */
#define FLASH_ACR_ICEN                         FLASH_ACR_ICEN_Msk
#define FLASH_ACR_ICRST_Pos                    (11U)
#define FLASH_ACR_ICRST_Msk                    (0x1UL << FLASH_ACR_ICRST_Pos)   /*!< 0x00000800 */
#define FLASH_ACR_ICRST                        FLASH_ACR_ICRST_Msk
#define FLASH_ACR_PROGEMPTY_Pos                (16U)
#define FLASH_ACR_PROGEMPTY_Msk                (0x1UL << FLASH_ACR_PROGEMPTY_Pos) /*!< 0x00010000 */
#define FLASH_ACR_PROGEMPTY                    FLASH_ACR_PROGEMPTY_Msk
#define FLASH_ACR_DBG_SWEN_Pos                 (18U)
#define FLASH_ACR_DBG_SWEN_Msk                 (0x1UL << FLASH_ACR_DBG_SWEN_Pos) /*!< 0x00040000 */
#define FLASH_ACR_DBG_SWEN                     FLASH_ACR_DBG_SWEN_Msk

/*******************  Bits definition for FLASH_SR register  ******************/
#define FLASH_SR_EOP_Pos                       (0U)
#define FLASH_SR_EOP_Msk                       (0x1UL << FLASH_SR_EOP_Pos)      /*!< 0x00000001 */
#define FLASH_SR_EOP                           FLASH_SR_EOP_Msk
#define FLASH_SR_OPERR_Pos                     (1U)
#define FLASH_SR_OPERR_Msk                     (0x1UL << FLASH_SR_OPERR_Pos)    /*!< 0x00000002 */
#define FLASH_SR_OPERR                         FLASH_SR_OPERR_Msk
#define FLASH_SR_PROGERR_Pos                   (3U)
#define FLASH_SR_PROGERR_Msk                   (0x1UL << FLASH_SR_PROGERR_Pos)  /*!< 0x00000008 */
#define FLASH_SR_PROGERR                       FLASH_SR_PROGERR_Msk
#define FLASH_SR_WRPERR_Pos                    (4U)
#define FLASH_SR_WRPERR_Msk                    (0x1UL << FLASH_SR_WRPERR_Pos)   /*!< 0x00000010 */
#define FLASH_SR_WRPERR                        FLASH_SR_WRPERR_Msk
#define FLASH_SR_PGAERR_Pos                    (5U)
#define FLASH_SR_PGAERR_Msk                    (0x1UL << FLASH_SR_PGAERR_Pos)   /*!< 0x00000020 */
#define FLASH_SR_PGAERR                        FLASH_SR_PGAERR_Msk
#define FLASH_SR_SIZERR_Pos                    (6U)
#define FLASH_SR_SIZERR_Msk                    (0x1UL << FLASH_SR_SIZERR_Pos)   /*!< 0x00000040 */
#define FLASH_SR_SIZERR                        FLASH_SR_SIZERR_Msk
#define FLASH_SR_PGSERR_Pos                    (7U)
#define FLASH_SR_PGSERR_Msk                    (0x1UL << FLASH_SR_PGSERR_Pos)   /*!< 0x00000080 */
#define FLASH_SR_PGSERR                        FLASH_SR_PGSERR_Msk
#define FLASH_SR_MISERR_Pos                    (8U)
#define FLASH_SR_MISERR_Msk                    (0x1UL << FLASH_SR_MISERR_Pos)   /*!< 0x00000100 */
#define FLASH_SR_MISERR                        FLASH_SR_MISERR_Msk
#define FLASH_SR_FASTERR_Pos                   (9U)
#define FLASH_SR_FASTERR_Msk                   (0x1UL << FLASH_SR_FASTERR_Pos)  /*!< 0x00000200 */
#define FLASH_SR_FASTERR                       FLASH_SR_FASTERR_Msk
#define FLASH_SR_RDERR_Pos                     (14U)
#define FLASH_SR_RDERR_Msk                     (0x1UL << FLASH_SR_RDERR_Pos)    /*!< 0x00004000 */
#define FLASH_SR_RDERR                         FLASH_SR_RDERR_Msk
#define FLASH_SR_OPTVERR_Pos                   (15U)
#define FLASH_SR_OPTVERR_Msk                   (0x1UL << FLASH_SR_OPTVERR_Pos)  /*!< 0x00008000 */
#define FLASH_SR_OPTVERR                       FLASH_SR_OPTVERR_Msk
#define FLASH_SR_BSY1_Pos                      (16U)
#define FLASH_SR_BSY1_Msk                      (0x1UL << FLASH_SR_BSY1_Pos)     /*!< 0x00010000 */
#define FLASH_SR_BSY1                          FLASH_SR_BSY1_Msk
#define FLASH_SR_CFGBSY_Pos                    (18U)
#define FLASH_SR_CFGBSY_Msk                    (0x1UL << FLASH_SR_CFGBSY_Pos)   /*!< 0x00040000 */
#define FLASH_SR_CFGBSY                        FLASH_SR_CFGBSY_Msk

/*******************  Bits definition for FLASH_CR register  ******************/
#define FLASH_CR_PG_Pos                        (0U)
#define FLASH_CR_PG_Msk                        (0x1UL << FLASH_CR_PG_Pos)       /*!< 0x00000001 */
#define FLASH_CR_PG                            FLASH_CR_PG_Msk
#define FLASH_CR_PER_Pos                       (1U)
#define FLASH_CR_PER_Msk                       (0x1UL << FLASH_CR_PER_Pos)      /*!< 0x00000002 */
#define FLASH_CR_PER                           FLASH_CR_PER_Msk
#define FLASH_CR_MER1_Pos                      (2U)
#define FLASH_CR_MER1_Msk                      (0x1UL << FLASH_CR_MER1_Pos)     /*!< 0x00000004 */
#define FLASH_CR_MER1                          FLASH_CR_MER1_Msk
#define FLASH_CR_PNB_Pos                       (3U)
#define FLASH_CR_PNB_Msk                       (0x3FUL << FLASH_CR_PNB_Pos)     /*!< 0x000001F8 */
#define FLASH_CR_PNB                           FLASH_CR_PNB_Msk
#define FLASH_CR_STRT_Pos                      (16U)
#define FLASH_CR_STRT_Msk                      (0x1UL << FLASH_CR_STRT_Pos)     /*!< 0x00010000 */
#define FLASH_CR_STRT                          FLASH_CR_STRT_Msk
#define FLASH_CR_OPTSTRT_Pos                   (17U)
#define FLASH_CR_OPTSTRT_Msk                   (0x1UL << FLASH_CR_OPTSTRT_Pos)  /*!< 0x00020000 */
#define FLASH_CR_OPTSTRT                       FLASH_CR_OPTSTRT_Msk
#define FLASH_CR_FSTPG_Pos                     (18U)
#define FLASH_CR_FSTPG_Msk                     (0x1UL << FLASH_CR_FSTPG_Pos)    /*!< 0x00040000 */
#define FLASH_CR_FSTPG                         FLASH_CR_FSTPG_Msk
#define FLASH_CR_EOPIE_Pos                     (24U)
#define FLASH_CR_EOPIE_Msk                     (0x1UL << FLASH_CR_EOPIE_Pos)    /*!< 0x01000000 */
#define FLASH_CR_EOPIE                         FLASH_CR_EOPIE_Msk
#define FLASH_CR_ERRIE_Pos                     (25U)
#define FLASH_CR_ERRIE_Msk                     (0x1UL << FLASH_CR_ERRIE_Pos)    /*!< 0x02000000 */
#define FLASH_CR_ERRIE                         FLASH_CR_ERRIE_Msk
#define FLASH_CR_RDERRIE_Pos                   (26U)
#define FLASH_CR_RDERRIE_Msk                   (0x1UL << FLASH_CR_RDERRIE_Pos)  /*!< 0x04000000 */
#define FLASH_CR_RDERRIE                       FLASH_CR_RDERRIE_Msk
#define FLASH_CR_OBL_LAUNCH_Pos                (27U)
#define FLASH_CR_OBL_LAUNCH_Msk                (0x1UL << FLASH_CR_OBL_LAUNCH_Pos) /*!< 0x08000000 */
#define FLASH_CR_OBL_LAUNCH                    FLASH_CR_OBL_LAUNCH_Msk
#define FLASH_CR_SEC_PROT_Pos                  (28U)
#define FLASH_CR_SEC_PROT_Msk                  (0x1UL << FLASH_CR_SEC_PROT_Pos) /*!< 0x10000000 */
#define FLASH_CR_SEC_PROT                      FLASH_CR_SEC_PROT_Msk
#define FLASH_CR_OPTLOCK_Pos                   (30U)
#define FLASH_CR_OPTLOCK_Msk                   (0x1UL << FLASH_CR_OPTLOCK_Pos)  /*!< 0x40000000 */
#define FLASH_CR_OPTLOCK                       FLASH_CR_OPTLOCK_Msk
#define FLASH_CR_LOCK_Pos                      (31U)
#define FLASH_CR_LOCK_Msk                      (0x1UL << FLASH_CR_LOCK_Pos)     /*!< 0x80000000 */
#define FLASH_CR_LOCK                          FLASH_CR_LOCK_Msk

/*******************  Bits definition for FLASH_ECCR register  ****************/
#define FLASH_ECCR_ADDR_ECC_Pos                (0U)
#define FLASH_ECCR_ADDR_ECC_Msk                (0x3FFFUL << FLASH_ECCR_ADDR_ECC_Pos) /*!< 0x00003FFF */
#define FLASH_ECCR_ADDR_ECC                    FLASH_ECCR_ADDR_ECC_Msk
#define FLASH_ECCR_SYSF_ECC_Pos                (20U)
#define FLASH_ECCR_SYSF_ECC_Msk                (0x1UL << FLASH_ECCR_SYSF_ECC_Pos) /*!< 0x00100000 */
#define FLASH_ECCR_SYSF_ECC                    FLASH_ECCR_SYSF_ECC_Msk
#define FLASH_ECCR_ECCCIE_Pos                  (24U)
#define FLASH_ECCR_ECCCIE_Msk                  (0x1UL << FLASH_ECCR_ECCCIE_Pos) /*!< 0x01000000 */
#define FLASH_ECCR_ECCCIE                      FLASH_ECCR_ECCCIE_Msk
#define FLASH_ECCR_ECCC_Pos                    (30U)
#define FLASH_ECCR_ECCC_Msk                    (0x1UL << FLASH_ECCR_ECCC_Pos)   /*!< 0x40000000 */
#define FLASH_ECCR_ECCC                        FLASH_ECCR_ECCC_Msk
#define FLASH_ECCR_ECCD_Pos                    (31U)
#define FLASH_ECCR_ECCD_Msk                    (0x1UL << FLASH_ECCR_ECCD_Pos)   /*!< 0x80000000 */
#define FLASH_ECCR_ECCD                        FLASH_ECCR_ECCD_Msk

/*******************  Bits definition for FLASH_OPTR register  ****************/
#define FLASH_OPTR_RDP_Pos                     (0U)
#define FLASH_OPTR_RDP_Msk                     (0xFFUL << FLASH_OPTR_RDP_Pos)   /*!< 0x000000FF */
#define FLASH_OPTR_RDP                         FLASH_OPTR_RDP_Msk
#define FLASH_OPTR_BOR_EN_Pos                  (8U)
#define FLASH_OPTR_BOR_EN_Msk                  (0x1UL << FLASH_OPTR_BOR_EN_Pos) /*!< 0x00000100 */
#define FLASH_OPTR_BOR_EN                      FLASH_OPTR_BOR_EN_Msk
#define FLASH_OPTR_BORF_LEV_Pos                (9U)
#define FLASH_OPTR_BORF_LEV_Msk                (0x3UL << FLASH_OPTR_BORF_LEV_Pos) /*!< 0x00000600 */
#define FLASH_OPTR_BORF_LEV                    FLASH_OPTR_BORF_LEV_Msk
#define FLASH_OPTR_BORF_LEV_0                  (0x1UL << FLASH_OPTR_BORF_LEV_Pos) /*!< 0x00000200 */
#define FLASH_OPTR_BORF_LEV_1                  (0x2UL << FLASH_OPTR_BORF_LEV_Pos) /*!< 0x00000400 */
#define FLASH_OPTR_BORR_LEV_Pos                (11U)
#define FLASH_OPTR_BORR_LEV_Msk                (0x3UL << FLASH_OPTR_BORR_LEV_Pos) /*!< 0x00001800 */
#define FLASH_OPTR_BORR_LEV                    FLASH_OPTR_BORR_LEV_Msk
#define FLASH_OPTR_BORR_LEV_0                  (0x1UL << FLASH_OPTR_BORR_LEV_Pos) /*!< 0x00000800 */
#define FLASH_OPTR_BORR_LEV_1                  (0x2UL << FLASH_OPTR_BORR_LEV_Pos) /*!< 0x00001000 */
#define FLASH_OPTR_nRST_STOP_Pos               (13U)
#define FLASH_OPTR_nRST_STOP_Msk               (0x1UL << FLASH_OPTR_nRST_STOP_Pos) /*!< 0x00002000 */
#define FLASH_OPTR_nRST_STOP                   FLASH_OPTR_nRST_STOP_Msk
#define FLASH_OPTR_nRST_STDBY_Pos              (14U)
#define FLASH_OPTR_nRST_STDBY_Msk              (0x1UL << FLASH_OPTR_nRST_STDBY_Pos) /*!< 0x00004000 */
#define FLASH_OPTR_nRST_STDBY                  FLASH_OPTR_nRST_STDBY_Msk
#define FLASH_OPTR_nRST_SHDW_Pos               (15U)
#define FLASH_OPTR_nRST_SHDW_Msk               (0x1UL << FLASH_OPTR_nRST_SHDW_Pos) /*!< 0x00008000 */
#define FLASH_OPTR_nRST_SHDW                   FLASH_OPTR_nRST_SHDW_Msk
#define FLASH_OPTR_IWDG_SW_Pos                 (16U)
#define FLASH_OPTR_IWDG_SW_Msk                 (0x1UL << FLASH_OPTR_IWDG_SW_Pos)   /*!< 0x00010000 */
#define FLASH_OPTR_IWDG_SW                     FLASH_OPTR_IWDG_SW_Msk
#define FLASH_OPTR_IWDG_STOP_Pos               (17U)
#define FLASH_OPTR_IWDG_STOP_Msk               (0x1UL << FLASH_OPTR_IWDG_STOP_Pos) /*!< 0x00020000 */
#define FLASH_OPTR_IWDG_STOP                   FLASH_OPTR_IWDG_STOP_Msk
#define FLASH_OPTR_IWDG_STDBY_Pos              (18U)
#define FLASH_OPTR_IWDG_STDBY_Msk              (0x1UL << FLASH_OPTR_IWDG_STDBY_Pos) /*!< 0x00040000 */
#define FLASH_OPTR_IWDG_STDBY                  FLASH_OPTR_IWDG_STDBY_Msk
#define FLASH_OPTR_WWDG_SW_Pos                 (19U)
#define FLASH_OPTR_WWDG_SW_Msk                 (0x1UL << FLASH_OPTR_WWDG_SW_Pos) /*!< 0x00080000 */
#define FLASH_OPTR_WWDG_SW                     FLASH_OPTR_WWDG_SW_Msk
#define FLASH_OPTR_RAM_PARITY_CHECK_Pos        (22U)
#define FLASH_OPTR_RAM_PARITY_CHECK_Msk        (0x1UL << FLASH_OPTR_RAM_PARITY_CHECK_Pos) /*!< 0x00400000 */
#define FLASH_OPTR_RAM_PARITY_CHECK            FLASH_OPTR_RAM_PARITY_CHECK_Msk
#define FLASH_OPTR_nBOOT_SEL_Pos               (24U)
#define FLASH_OPTR_nBOOT_SEL_Msk               (0x1UL << FLASH_OPTR_nBOOT_SEL_Pos) /*!< 0x01000000 */
#define FLASH_OPTR_nBOOT_SEL                   FLASH_OPTR_nBOOT_SEL_Msk
#define FLASH_OPTR_nBOOT1_Pos                  (25U)
#define FLASH_OPTR_nBOOT1_Msk                  (0x1UL << FLASH_OPTR_nBOOT1_Pos) /*!< 0x02000000 */
#define FLASH_OPTR_nBOOT1                      FLASH_OPTR_nBOOT1_Msk
#define FLASH_OPTR_nBOOT0_Pos                  (26U)
#define FLASH_OPTR_nBOOT0_Msk                  (0x1UL << FLASH_OPTR_nBOOT0_Pos) /*!< 0x04000000 */
#define FLASH_OPTR_nBOOT0                      FLASH_OPTR_nBOOT0_Msk
#define FLASH_OPTR_NRST_MODE_Pos               (27U)
#define FLASH_OPTR_NRST_MODE_Msk               (0x3UL << FLASH_OPTR_NRST_MODE_Pos) /*!< 0x18000000 */
#define FLASH_OPTR_NRST_MODE                   FLASH_OPTR_NRST_MODE_Msk
#define FLASH_OPTR_NRST_MODE_0                 (0x1UL << FLASH_OPTR_NRST_MODE_Pos) /*!< 0x08000000 */
#define FLASH_OPTR_NRST_MODE_1                 (0x2UL << FLASH_OPTR_NRST_MODE_Pos) /*!< 0x10000000 */
#define FLASH_OPTR_IRHEN_Pos                   (29U)
#define FLASH_OPTR_IRHEN_Msk                   (0x1UL << FLASH_OPTR_IRHEN_Pos) /*!< 0x20000000 */
#define FLASH_OPTR_IRHEN                       FLASH_OPTR_IRHEN_Msk

/******************  Bits definition for FLASH_PCROP1ASR register  ************/
#define FLASH_PCROP1ASR_PCROP1A_STRT_Pos       (0U)
#define FLASH_PCROP1ASR_PCROP1A_STRT_Msk       (0xFFUL << FLASH_PCROP1ASR_PCROP1A_STRT_Pos) /*!< 0x000000FF */
#define FLASH_PCROP1ASR_PCROP1A_STRT           FLASH_PCROP1ASR_PCROP1A_STRT_Msk

/******************  Bits definition for FLASH_PCROP1AER register  ************/
#define FLASH_PCROP1AER_PCROP1A_END_Pos        (0U)
#define FLASH_PCROP1AER_PCROP1A_END_Msk        (0xFFUL << FLASH_PCROP1AER_PCROP1A_END_Pos) /*!< 0x000000FF */
#define FLASH_PCROP1AER_PCROP1A_END            FLASH_PCROP1AER_PCROP1A_END_Msk
#define FLASH_PCROP1AER_PCROP_RDP_Pos          (31U)
#define FLASH_PCROP1AER_PCROP_RDP_Msk          (0x1UL << FLASH_PCROP1AER_PCROP_RDP_Pos) /*!< 0x80000000 */
#define FLASH_PCROP1AER_PCROP_RDP              FLASH_PCROP1AER_PCROP_RDP_Msk

/******************  Bits definition for FLASH_WRP1AR register  ***************/
#define FLASH_WRP1AR_WRP1A_STRT_Pos            (0U)
#define FLASH_WRP1AR_WRP1A_STRT_Msk            (0x3FUL << FLASH_WRP1AR_WRP1A_STRT_Pos) /*!< 0x0000003F */
#define FLASH_WRP1AR_WRP1A_STRT                FLASH_WRP1AR_WRP1A_STRT_Msk
#define FLASH_WRP1AR_WRP1A_END_Pos             (16U)
#define FLASH_WRP1AR_WRP1A_END_Msk             (0x3FUL << FLASH_WRP1AR_WRP1A_END_Pos) /*!< 0x003F0000 */
#define FLASH_WRP1AR_WRP1A_END                 FLASH_WRP1AR_WRP1A_END_Msk

/******************  Bits definition for FLASH_WRP1BR register  ***************/
#define FLASH_WRP1BR_WRP1B_STRT_Pos            (0U)
#define FLASH_WRP1BR_WRP1B_STRT_Msk            (0x3FUL << FLASH_WRP1BR_WRP1B_STRT_Pos) /*!< 0x0000003F */
#define FLASH_WRP1BR_WRP1B_STRT                FLASH_WRP1BR_WRP1B_STRT_Msk
#define FLASH_WRP1BR_WRP1B_END_Pos             (16U)
#define FLASH_WRP1BR_WRP1B_END_Msk             (0x3FUL << FLASH_WRP1BR_WRP1B_END_Pos) /*!< 0x003F0000 */
#define FLASH_WRP1BR_WRP1B_END                 FLASH_WRP1BR_WRP1B_END_Msk

/******************  Bits definition for FLASH_PCROP1BSR register  ************/
#define FLASH_PCROP1BSR_PCROP1B_STRT_Pos       (0U)
#define FLASH_PCROP1BSR_PCROP1B_STRT_Msk       (0xFFUL << FLASH_PCROP1BSR_PCROP1B_STRT_Pos) /*!< 0x000000FF */
#define FLASH_PCROP1BSR_PCROP1B_STRT           FLASH_PCROP1BSR_PCROP1B_STRT_Msk

/******************  Bits definition for FLASH_PCROP1BER register  ************/
#define FLASH_PCROP1BER_PCROP1B_END_Pos        (0U)
#define FLASH_PCROP1BER_PCROP1B_END_Msk        (0xFFUL << FLASH_PCROP1BER_PCROP1B_END_Pos)  /*!< 0x000000FF */
#define FLASH_PCROP1BER_PCROP1B_END            FLASH_PCROP1BER_PCROP1B_END_Msk

/******************  Bits definition for FLASH_SECR register  *****************/
#define FLASH_SECR_SEC_SIZE_Pos                (0U)
#define FLASH_SECR_SEC_SIZE_Msk                (0x7FUL << FLASH_SECR_SEC_SIZE_Pos) /*!< 0x0000007F */
#define FLASH_SECR_SEC_SIZE                    FLASH_SECR_SEC_SIZE_Msk
#define FLASH_SECR_BOOT_LOCK_Pos               (16U)
#define FLASH_SECR_BOOT_LOCK_Msk               (0x1UL << FLASH_SECR_BOOT_LOCK_Pos) /*!< 0x00010000 */
#define FLASH_SECR_BOOT_LOCK                   FLASH_SECR_BOOT_LOCK_Msk


/******************************************************************************/
/*                                                                            */
/*                            General Purpose I/O                             */
/*                                                                            */
/******************************************************************************/
/******************  Bits definition for GPIO_MODER register  *****************/
#define GPIO_MODER_MODE0_Pos           (0U)
#define GPIO_MODER_MODE0_Msk           (0x3UL << GPIO_MODER_MODE0_Pos)          /*!< 0x00000003 */
#define GPIO_MODER_MODE0               GPIO_MODER_MODE0_Msk
#define GPIO_MODER_MODE0_0             (0x1UL << GPIO_MODER_MODE0_Pos)          /*!< 0x00000001 */
#define GPIO_MODER_MODE0_1             (0x2UL << GPIO_MODER_MODE0_Pos)          /*!< 0x00000002 */
#define GPIO_MODER_MODE1_Pos           (2U)
#define GPIO_MODER_MODE1_Msk           (0x3UL << GPIO_MODER_MODE1_Pos)          /*!< 0x0000000C */
#define GPIO_MODER_MODE1               GPIO_MODER_MODE1_Msk
#define GPIO_MODER_MODE1_0             (0x1UL << GPIO_MODER_MODE1_Pos)          /*!< 0x00000004 */
#define GPIO_MODER_MODE1_1             (0x2UL << GPIO_MODER_MODE1_Pos)          /*!< 0x00000008 */
#define GPIO_MODER_MODE2_Pos           (4U)
#define GPIO_MODER_MODE2_Msk           (0x3UL << GPIO_MODER_MODE2_Pos)          /*!< 0x00000030 */
#define GPIO_MODER_MODE2               GPIO_MODER_MODE2_Msk
#define GPIO_MODER_MODE2_0             (0x1UL << GPIO_MODER_MODE2_Pos)          /*!< 0x00000010 */
#define GPIO_MODER_MODE2_1             (0x2UL << GPIO_MODER_MODE2_Pos)          /*!< 0x00000020 */
#define GPIO_MODER_MODE3_Pos           (6U)
#define GPIO_MODER_MODE3_Msk           (0x3UL << GPIO_MODER_MODE3_Pos)          /*!< 0x000000C0 */
#define GPIO_MODER_MODE3               GPIO_MODER_MODE3_Msk
#define GPIO_MODER_MODE3_0             (0x1UL << GPIO_MODER_MODE3_Pos)          /*!< 0x00000040 */
#define GPIO_MODER_MODE3_1             (0x2UL << GPIO_MODER_MODE3_Pos)          /*!< 0x00000080 */
#define GPIO_MODER_MODE4_Pos           (8U)
#define GPIO_MODER_MODE4_Msk           (0x3UL << GPIO_MODER_MODE4_Pos)          /*!< 0x00000300 */
#define GPIO_MODER_MODE4               GPIO_MODER_MODE4_Msk
#define GPIO_MODER_MODE4_0             (0x1UL << GPIO_MODER_MODE4_Pos)          /*!< 0x00000100 */
#define GPIO_MODER_MODE4_1             (0x2UL << GPIO_MODER_MODE4_Pos)          /*!< 0x00000200 */
#define GPIO_MODER_MODE5_Pos           (10U)
#define GPIO_MODER_MODE5_Msk           (0x3UL << GPIO_MODER_MODE5_Pos)          /*!< 0x00000C00 */
#define GPIO_MODER_MODE5               GPIO_MODER_MODE5_Msk
#define GPIO_MODER_MODE5_0             (0x1UL << GPIO_MODER_MODE5_Pos)          /*!< 0x00000400 */
#define GPIO_MODER_MODE5_1             (0x2UL << GPIO_MODER_MODE5_Pos)          /*!< 0x00000800 */
#define GPIO_MODER_MODE6_Pos           (12U)
#define GPIO_MODER_MODE6_Msk           (0x3UL << GPIO_MODER_MODE6_Pos)          /*!< 0x00003000 */
#define GPIO_MODER_MODE6               GPIO_MODER_MODE6_Msk
#define GPIO_MODER_MODE6_0             (0x1UL << GPIO_MODER_MODE6_Pos)          /*!< 0x00001000 */
#define GPIO_MODER_MODE6_1             (0x2UL << GPIO_MODER_MODE6_Pos)          /*!< 0x00002000 */
#define GPIO_MODER_MODE7_Pos           (14U)
#define GPIO_MODER_MODE7_Msk           (0x3UL << GPIO_MODER_MODE7_Pos)          /*!< 0x0000C000 */
#define GPIO_MODER_MODE7               GPIO_MODER_MODE7_Msk
#define GPIO_MODER_MODE7_0             (0x1UL << GPIO_MODER_MODE7_Pos)          /*!< 0x00004000 */
#define GPIO_MODER_MODE7_1             (0x2UL << GPIO_MODER_MODE7_Pos)          /*!< 0x00008000 */
#define GPIO_MODER_MODE8_Pos           (16U)
#define GPIO_MODER_MODE8_Msk           (0x3UL << GPIO_MODER_MODE8_Pos)          /*!< 0x00030000 */
#define GPIO_MODER_MODE8               GPIO_MODER_MODE8_Msk
#define GPIO_MODER_MODE8_0             (0x1UL << GPIO_MODER_MODE8_Pos)          /*!< 0x00010000 */
#define GPIO_MODER_MODE8_1             (0x2UL << GPIO_MODER_MODE8_Pos)          /*!< 0x00020000 */
#define GPIO_MODER_MODE9_Pos           (18U)
#define GPIO_MODER_MODE9_Msk           (0x3UL << GPIO_MODER_MODE9_Pos)          /*!< 0x000C0000 */
#define GPIO_MODER_MODE9               GPIO_MODER_MODE9_Msk
#define GPIO_MODER_MODE9_0             (0x1UL << GPIO_MODER_MODE9_Pos)          /*!< 0x00040000 */
#define GPIO_MODER_MODE9_1             (0x2UL << GPIO_MODER_MODE9_Pos)          /*!< 0x00080000 */
#define GPIO_MODER_MODE10_Pos          (20U)
#define GPIO_MODER_MODE10_Msk          (0x3UL << GPIO_MODER_MODE10_Pos)         /*!< 0x00300000 */
#define GPIO_MODER_MODE10              GPIO_MODER_MODE10_Msk
#define GPIO_MODER_MODE10_0            (0x1UL << GPIO_MODER_MODE10_Pos)         /*!< 0x00100000 */
#define GPIO_MODER_MODE10_1            (0x2UL << GPIO_MODER_MODE10_Pos)         /*!< 0x00200000 */
#define GPIO_MODER_MODE11_Pos          (22U)
#define GPIO_MODER_MODE11_Msk          (0x3UL << GPIO_MODER_MODE11_Pos)         /*!< 0x00C00000 */
#define GPIO_MODER_MODE11              GPIO_MODER_MODE11_Msk
#define GPIO_MODER_MODE11_0            (0x1UL << GPIO_MODER_MODE11_Pos)         /*!< 0x00400000 */
#define GPIO_MODER_MODE11_1            (0x2UL << GPIO_MODER_MODE11_Pos)         /*!< 0x00800000 */
#define GPIO_MODER_MODE12_Pos          (24U)
#define GPIO_MODER_MODE12_Msk          (0x3UL << GPIO_MODER_MODE12_Pos)         /*!< 0x03000000 */
#define GPIO_MODER_MODE12              GPIO_MODER_MODE12_Msk
#define GPIO_MODER_MODE12_0            (0x1UL << GPIO_MODER_MODE12_Pos)         /*!< 0x01000000 */
#define GPIO_MODER_MODE12_1            (0x2UL << GPIO_MODER_MODE12_Pos)         /*!< 0x02000000 */
#define GPIO_MODER_MODE13_Pos          (26U)
#define GPIO_MODER_MODE13_Msk          (0x3UL << GPIO_MODER_MODE13_Pos)         /*!< 0x0C000000 */
#define GPIO_MODER_MODE13              GPIO_MODER_MODE13_Msk
#define GPIO_MODER_MODE13_0            (0x1UL << GPIO_MODER_MODE13_Pos)         /*!< 0x04000000 */
#define GPIO_MODER_MODE13_1            (0x2UL << GPIO_MODER_MODE13_Pos)         /*!< 0x08000000 */
#define GPIO_MODER_MODE14_Pos          (28U)
#define GPIO_MODER_MODE14_Msk          (0x3UL << GPIO_MODER_MODE14_Pos)         /*!< 0x30000000 */
#define GPIO_MODER_MODE14              GPIO_MODER_MODE14_Msk
#define GPIO_MODER_MODE14_0            (0x1UL << GPIO_MODER_MODE14_Pos)         /*!< 0x10000000 */
#define GPIO_MODER_MODE14_1            (0x2UL << GPIO_MODER_MODE14_Pos)         /*!< 0x20000000 */
#define GPIO_MODER_MODE15_Pos          (30U)
#define GPIO_MODER_MODE15_Msk          (0x3UL << GPIO_MODER_MODE15_Pos)         /*!< 0xC0000000 */
#define GPIO_MODER_MODE15              GPIO_MODER_MODE15_Msk
#define GPIO_MODER_MODE15_0            (0x1UL << GPIO_MODER_MODE15_Pos)         /*!< 0x40000000 */
#define GPIO_MODER_MODE15_1            (0x2UL << GPIO_MODER_MODE15_Pos)         /*!< 0x80000000 */

/******************  Bits definition for GPIO_OTYPER register  ****************/
#define GPIO_OTYPER_OT0_Pos            (0U)
#define GPIO_OTYPER_OT0_Msk            (0x1UL << GPIO_OTYPER_OT0_Pos)           /*!< 0x00000001 */
#define GPIO_OTYPER_OT0                GPIO_OTYPER_OT0_Msk
#define GPIO_OTYPER_OT1_Pos            (1U)
#define GPIO_OTYPER_OT1_Msk            (0x1UL << GPIO_OTYPER_OT1_Pos)           /*!< 0x00000002 */
#define GPIO_OTYPER_OT1                GPIO_OTYPER_OT1_Msk
#define GPIO_OTYPER_OT2_Pos            (2U)
#define GPIO_OTYPER_OT2_Msk            (0x1UL << GPIO_OTYPER_OT2_Pos)           /*!< 0x00000004 */
#define GPIO_OTYPER_OT2                GPIO_OTYPER_OT2_Msk
#define GPIO_OTYPER_OT3_Pos            (3U)
#define GPIO_OTYPER_OT3_Msk            (0x1UL << GPIO_OTYPER_OT3_Pos)           /*!< 0x00000008 */
#define GPIO_OTYPER_OT3                GPIO_OTYPER_OT3_Msk
#define GPIO_OTYPER_OT4_Pos            (4U)
#define GPIO_OTYPER_OT4_Msk            (0x1UL << GPIO_OTYPER_OT4_Pos)           /*!< 0x00000010 */
#define GPIO_OTYPER_OT4                GPIO_OTYPER_OT4_Msk
#define GPIO_OTYPER_OT5_Pos            (5U)
#define GPIO_OTYPER_OT5_Msk            (0x1UL << GPIO_OTYPER_OT5_Pos)           /*!< 0x00000020 */
#define GPIO_OTYPER_OT5                GPIO_OTYPER_OT5_Msk
#define GPIO_OTYPER_OT6_Pos            (6U)
#define GPIO_OTYPER_OT6_Msk            (0x1UL << GPIO_OTYPER_OT6_Pos)           /*!< 0x00000040 */
#define GPIO_OTYPER_OT6                GPIO_OTYPER_OT6_Msk
#define GPIO_OTYPER_OT7_Pos            (7U)
#define GPIO_OTYPER_OT7_Msk            (0x1UL << GPIO_OTYPER_OT7_Pos)           /*!< 0x00000080 */
#define GPIO_OTYPER_OT7                GPIO_OTYPER_OT7_Msk
#define GPIO_OTYPER_OT8_Pos            (8U)
#define GPIO_OTYPER_OT8_Msk            (0x1UL << GPIO_OTYPER_OT8_Pos)           /*!< 0x00000100 */
#define GPIO_OTYPER_OT8                GPIO_OTYPER_OT8_Msk
#define GPIO_OTYPER_OT9_Pos            (9U)
#define GPIO_OTYPER_OT9_Msk            (0x1UL << GPIO_OTYPER_OT9_Pos)           /*!< 0x00000200 */
#define GPIO_OTYPER_OT9                GPIO_OTYPER_OT9_Msk
#define GPIO_OTYPER_OT10_Pos           (10U)
#define GPIO_OTYPER_OT10_Msk           (0x1UL << GPIO_OTYPER_OT10_Pos)          /*!< 0x00000400 */
#define GPIO_OTYPER_OT10               GPIO_OTYPER_OT10_Msk
#define GPIO_OTYPER_OT11_Pos           (11U)
#define GPIO_OTYPER_OT11_Msk           (0x1UL << GPIO_OTYPER_OT11_Pos)          /*!< 0x00000800 */
#define GPIO_OTYPER_OT11               GPIO_OTYPER_OT11_Msk
#define GPIO_OTYPER_OT12_Pos           (12U)
#define GPIO_OTYPER_OT12_Msk           (0x1UL << GPIO_OTYPER_OT12_Pos)          /*!< 0x00001000 */
#define GPIO_OTYPER_OT12               GPIO_OTYPER_OT12_Msk
#define GPIO_OTYPER_OT13_Pos           (13U)
#define GPIO_OTYPER_OT13_Msk           (0x1UL << GPIO_OTYPER_OT13_Pos)          /*!< 0x00002000 */
#define GPIO_OTYPER_OT13               GPIO_OTYPER_OT13_Msk
#define GPIO_OTYPER_OT14_Pos           (14U)
#define GPIO_OTYPER_OT14_Msk           (0x1UL << GPIO_OTYPER_OT14_Pos)          /*!< 0x00004000 */
#define GPIO_OTYPER_OT14               GPIO_OTYPER_OT14_Msk
#define GPIO_OTYPER_OT15_Pos           (15U)
#define GPIO_OTYPER_OT15_Msk           (0x1UL << GPIO_OTYPER_OT15_Pos)          /*!< 0x00008000 */
#define GPIO_OTYPER_OT15               GPIO_OTYPER_OT15_Msk

/******************  Bits definition for GPIO_OSPEEDR register  ***************/
#define GPIO_OSPEEDR_OSPEED0_Pos       (0U)
#define GPIO_OSPEEDR_OSPEED0_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED0_Pos)      /*!< 0x00000003 */
#define GPIO_OSPEEDR_OSPEED0           GPIO_OSPEEDR_OSPEED0_Msk
#define GPIO_OSPEEDR_OSPEED0_0         (0x1UL << GPIO_OSPEEDR_OSPEED0_Pos)      /*!< 0x00000001 */
#define GPIO_OSPEEDR_OSPEED0_1         (0x2UL << GPIO_OSPEEDR_OSPEED0_Pos)      /*!< 0x00000002 */
#define GPIO_OSPEEDR_OSPEED1_Pos       (2U)
#define GPIO_OSPEEDR_OSPEED1_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED1_Pos)      /*!< 0x0000000C */
#define GPIO_OSPEEDR_OSPEED1           GPIO_OSPEEDR_OSPEED1_Msk
#define GPIO_OSPEEDR_OSPEED1_0         (0x1UL << GPIO_OSPEEDR_OSPEED1_Pos)      /*!< 0x00000004 */
#define GPIO_OSPEEDR_OSPEED1_1         (0x2UL << GPIO_OSPEEDR_OSPEED1_Pos)      /*!< 0x00000008 */
#define GPIO_OSPEEDR_OSPEED2_Pos       (4U)
#define GPIO_OSPEEDR_OSPEED2_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED2_Pos)      /*!< 0x00000030 */
#define GPIO_OSPEEDR_OSPEED2           GPIO_OSPEEDR_OSPEED2_Msk
#define GPIO_OSPEEDR_OSPEED2_0         (0x1UL << GPIO_OSPEEDR_OSPEED2_Pos)      /*!< 0x00000010 */
#define GPIO_OSPEEDR_OSPEED2_1         (0x2UL << GPIO_OSPEEDR_OSPEED2_Pos)      /*!< 0x00000020 */
#define GPIO_OSPEEDR_OSPEED3_Pos       (6U)
#define GPIO_OSPEEDR_OSPEED3_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED3_Pos)      /*!< 0x000000C0 */
#define GPIO_OSPEEDR_OSPEED3           GPIO_OSPEEDR_OSPEED3_Msk
#define GPIO_OSPEEDR_OSPEED3_0         (0x1UL << GPIO_OSPEEDR_OSPEED3_Pos)      /*!< 0x00000040 */
#define GPIO_OSPEEDR_OSPEED3_1         (0x2UL << GPIO_OSPEEDR_OSPEED3_Pos)      /*!< 0x00000080 */
#define GPIO_OSPEEDR_OSPEED4_Pos       (8U)
#define GPIO_OSPEEDR_OSPEED4_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED4_Pos)      /*!< 0x00000300 */
#define GPIO_OSPEEDR_OSPEED4           GPIO_OSPEEDR_OSPEED4_Msk
#define GPIO_OSPEEDR_OSPEED4_0         (0x1UL << GPIO_OSPEEDR_OSPEED4_Pos)      /*!< 0x00000100 */
#define GPIO_OSPEEDR_OSPEED4_1         (0x2UL << GPIO_OSPEEDR_OSPEED4_Pos)      /*!< 0x00000200 */
#define GPIO_OSPEEDR_OSPEED5_Pos       (10U)
#define GPIO_OSPEEDR_OSPEED5_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED5_Pos)      /*!< 0x00000C00 */
#define GPIO_OSPEEDR_OSPEED5           GPIO_OSPEEDR_OSPEED5_Msk
#define GPIO_OSPEEDR_OSPEED5_0         (0x1UL << GPIO_OSPEEDR_OSPEED5_Pos)      /*!< 0x00000400 */
#define GPIO_OSPEEDR_OSPEED5_1         (0x2UL << GPIO_OSPEEDR_OSPEED5_Pos)      /*!< 0x00000800 */
#define GPIO_OSPEEDR_OSPEED6_Pos       (12U)
#define GPIO_OSPEEDR_OSPEED6_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED6_Pos)      /*!< 0x00003000 */
#define GPIO_OSPEEDR_OSPEED6           GPIO_OSPEEDR_OSPEED6_Msk
#define GPIO_OSPEEDR_OSPEED6_0         (0x1UL << GPIO_OSPEEDR_OSPEED6_Pos)      /*!< 0x00001000 */
#define GPIO_OSPEEDR_OSPEED6_1         (0x2UL << GPIO_OSPEEDR_OSPEED6_Pos)      /*!< 0x00002000 */
#define GPIO_OSPEEDR_OSPEED7_Pos       (14U)
#define GPIO_OSPEEDR_OSPEED7_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED7_Pos)      /*!< 0x0000C000 */
#define GPIO_OSPEEDR_OSPEED7           GPIO_OSPEEDR_OSPEED7_Msk
#define GPIO_OSPEEDR_OSPEED7_0         (0x1UL << GPIO_OSPEEDR_OSPEED7_Pos)      /*!< 0x00004000 */
#define GPIO_OSPEEDR_OSPEED7_1         (0x2UL << GPIO_OSPEEDR_OSPEED7_Pos)      /*!< 0x00008000 */
#define GPIO_OSPEEDR_OSPEED8_Pos       (16U)
#define GPIO_OSPEEDR_OSPEED8_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED8_Pos)      /*!< 0x00030000 */
#define GPIO_OSPEEDR_OSPEED8           GPIO_OSPEEDR_OSPEED8_Msk
#define GPIO_OSPEEDR_OSPEED8_0         (0x1UL << GPIO_OSPEEDR_OSPEED8_Pos)      /*!< 0x00010000 */
#define GPIO_OSPEEDR_OSPEED8_1         (0x2UL << GPIO_OSPEEDR_OSPEED8_Pos)      /*!< 0x00020000 */
#define GPIO_OSPEEDR_OSPEED9_Pos       (18U)
#define GPIO_OSPEEDR_OSPEED9_Msk       (0x3UL << GPIO_OSPEEDR_OSPEED9_Pos)      /*!< 0x000C0000 */
#define GPIO_OSPEEDR_OSPEED9           GPIO_OSPEEDR_OSPEED9_Msk
#define GPIO_OSPEEDR_OSPEED9_0         (0x1UL << GPIO_OSPEEDR_OSPEED9_Pos)      /*!< 0x00040000 */
#define GPIO_OSPEEDR_OSPEED9_1         (0x2UL << GPIO_OSPEEDR_OSPEED9_Pos)      /*!< 0x00080000 */
#define GPIO_OSPEEDR_OSPEED10_Pos      (20U)
#define GPIO_OSPEEDR_OSPEED10_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED10_Pos)     /*!< 0x00300000 */
#define GPIO_OSPEEDR_OSPEED10          GPIO_OSPEEDR_OSPEED10_Msk
#define GPIO_OSPEEDR_OSPEED10_0        (0x1UL << GPIO_OSPEEDR_OSPEED10_Pos)     /*!< 0x00100000 */
#define GPIO_OSPEEDR_OSPEED10_1        (0x2UL << GPIO_OSPEEDR_OSPEED10_Pos)     /*!< 0x00200000 */
#define GPIO_OSPEEDR_OSPEED11_Pos      (22U)
#define GPIO_OSPEEDR_OSPEED11_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED11_Pos)     /*!< 0x00C00000 */
#define GPIO_OSPEEDR_OSPEED11          GPIO_OSPEEDR_OSPEED11_Msk
#define GPIO_OSPEEDR_OSPEED11_0        (0x1UL << GPIO_OSPEEDR_OSPEED11_Pos)     /*!< 0x00400000 */
#define GPIO_OSPEEDR_OSPEED11_1        (0x2UL << GPIO_OSPEEDR_OSPEED11_Pos)     /*!< 0x00800000 */
#define GPIO_OSPEEDR_OSPEED12_Pos      (24U)
#define GPIO_OSPEEDR_OSPEED12_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED12_Pos)     /*!< 0x03000000 */
#define GPIO_OSPEEDR_OSPEED12          GPIO_OSPEEDR_OSPEED12_Msk
#define GPIO_OSPEEDR_OSPEED12_0        (0x1UL << GPIO_OSPEEDR_OSPEED12_Pos)     /*!< 0x01000000 */
#define GPIO_OSPEEDR_OSPEED12_1        (0x2UL << GPIO_OSPEEDR_OSPEED12_Pos)     /*!< 0x02000000 */
#define GPIO_OSPEEDR_OSPEED13_Pos      (26U)
#define GPIO_OSPEEDR_OSPEED13_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED13_Pos)     /*!< 0x0C000000 */
#define GPIO_OSPEEDR_OSPEED13          GPIO_OSPEEDR_OSPEED13_Msk
#define GPIO_OSPEEDR_OSPEED13_0        (0x1UL << GPIO_OSPEEDR_OSPEED13_Pos)     /*!< 0x04000000 */
#define GPIO_OSPEEDR_OSPEED13_1        (0x2UL << GPIO_OSPEEDR_OSPEED13_Pos)     /*!< 0x08000000 */
#define GPIO_OSPEEDR_OSPEED14_Pos      (28U)
#define GPIO_OSPEEDR_OSPEED14_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED14_Pos)     /*!< 0x30000000 */
#define GPIO_OSPEEDR_OSPEED14          GPIO_OSPEEDR_OSPEED14_Msk
#define GPIO_OSPEEDR_OSPEED14_0        (0x1UL << GPIO_OSPEEDR_OSPEED14_Pos)     /*!< 0x10000000 */
#define GPIO_OSPEEDR_OSPEED14_1        (0x2UL << GPIO_OSPEEDR_OSPEED14_Pos)     /*!< 0x20000000 */
#define GPIO_OSPEEDR_OSPEED15_Pos      (30U)
#define GPIO_OSPEEDR_OSPEED15_Msk      (0x3UL << GPIO_OSPEEDR_OSPEED15_Pos)     /*!< 0xC0000000 */
#define GPIO_OSPEEDR_OSPEED15          GPIO_OSPEEDR_OSPEED15_Msk
#define GPIO_OSPEEDR_OSPEED15_0        (0x1UL << GPIO_OSPEEDR_OSPEED15_Pos)     /*!< 0x40000000 */
#define GPIO_OSPEEDR_OSPEED15_1        (0x2UL << GPIO_OSPEEDR_OSPEED15_Pos)     /*!< 0x80000000 */

/******************  Bits definition for GPIO_PUPDR register  *****************/
#define GPIO_PUPDR_PUPD0_Pos           (0U)
#define GPIO_PUPDR_PUPD0_Msk           (0x3UL << GPIO_PUPDR_PUPD0_Pos)          /*!< 0x00000003 */
#define GPIO_PUPDR_PUPD0               GPIO_PUPDR_PUPD0_Msk
#define GPIO_PUPDR_PUPD0_0             (0x1UL << GPIO_PUPDR_PUPD0_Pos)          /*!< 0x00000001 */
#define GPIO_PUPDR_PUPD0_1             (0x2UL << GPIO_PUPDR_PUPD0_Pos)          /*!< 0x00000002 */
#define GPIO_PUPDR_PUPD1_Pos           (2U)
#define GPIO_PUPDR_PUPD1_Msk           (0x3UL << GPIO_PUPDR_PUPD1_Pos)          /*!< 0x0000000C */
#define GPIO_PUPDR_PUPD1               GPIO_PUPDR_PUPD1_Msk
#define GPIO_PUPDR_PUPD1_0             (0x1UL << GPIO_PUPDR_PUPD1_Pos)          /*!< 0x00000004 */
#define GPIO_PUPDR_PUPD1_1             (0x2UL << GPIO_PUPDR_PUPD1_Pos)          /*!< 0x00000008 */
#define GPIO_PUPDR_PUPD2_Pos           (4U)
#define GPIO_PUPDR_PUPD2_Msk           (0x3UL << GPIO_PUPDR_PUPD2_Pos)          /*!< 0x00000030 */
#define GPIO_PUPDR_PUPD2               GPIO_PUPDR_PUPD2_Msk
#define GPIO_PUPDR_PUPD2_0             (0x1UL << GPIO_PUPDR_PUPD2_Pos)          /*!< 0x00000010 */
#define GPIO_PUPDR_PUPD2_1             (0x2UL << GPIO_PUPDR_PUPD2_Pos)          /*!< 0x00000020 */
#define GPIO_PUPDR_PUPD3_Pos           (6U)
#define GPIO_PUPDR_PUPD3_Msk           (0x3UL << GPIO_PUPDR_PUPD3_Pos)          /*!< 0x000000C0 */
#define GPIO_PUPDR_PUPD3               GPIO_PUPDR_PUPD3_Msk
#define GPIO_PUPDR_PUPD3_0             (0x1UL << GPIO_PUPDR_PUPD3_Pos)          /*!< 0x00000040 */
#define GPIO_PUPDR_PUPD3_1             (0x2UL << GPIO_PUPDR_PUPD3_Pos)          /*!< 0x00000080 */
#define GPIO_PUPDR_PUPD4_Pos           (8U)
#define GPIO_PUPDR_PUPD4_Msk           (0x3UL << GPIO_PUPDR_PUPD4_Pos)          /*!< 0x00000300 */
#define GPIO_PUPDR_PUPD4               GPIO_PUPDR_PUPD4_Msk
#define GPIO_PUPDR_PUPD4_0             (0x1UL << GPIO_PUPDR_PUPD4_Pos)          /*!< 0x00000100 */
#define GPIO_PUPDR_PUPD4_1             (0x2UL << GPIO_PUPDR_PUPD4_Pos)          /*!< 0x00000200 */
#define GPIO_PUPDR_PUPD5_Pos           (10U)
#define GPIO_PUPDR_PUPD5_Msk           (0x3UL << GPIO_PUPDR_PUPD5_Pos)          /*!< 0x00000C00 */
#define GPIO_PUPDR_PUPD5               GPIO_PUPDR_PUPD5_Msk
#define GPIO_PUPDR_PUPD5_0             (0x1UL << GPIO_PUPDR_PUPD5_Pos)          /*!< 0x00000400 */
#define GPIO_PUPDR_PUPD5_1             (0x2UL << GPIO_PUPDR_PUPD5_Pos)          /*!< 0x00000800 */
#define GPIO_PUPDR_PUPD6_Pos           (12U)
#define GPIO_PUPDR_PUPD6_Msk           (0x3UL << GPIO_PUPDR_PUPD6_Pos)          /*!< 0x00003000 */
#define GPIO_PUPDR_PUPD6               GPIO_PUPDR_PUPD6_Msk
#define GPIO_PUPDR_PUPD6_0             (0x1UL << GPIO_PUPDR_PUPD6_Pos)          /*!< 0x00001000 */
#define GPIO_PUPDR_PUPD6_1             (0x2UL << GPIO_PUPDR_PUPD6_Pos)          /*!< 0x00002000 */
#define GPIO_PUPDR_PUPD7_Pos           (14U)
#define GPIO_PUPDR_PUPD7_Msk           (0x3UL << GPIO_PUPDR_PUPD7_Pos)          /*!< 0x0000C000 */
#define GPIO_PUPDR_PUPD7               GPIO_PUPDR_PUPD7_Msk
#define GPIO_PUPDR_PUPD7_0             (0x1UL << GPIO_PUPDR_PUPD7_Pos)          /*!< 0x00004000 */
#define GPIO_PUPDR_PUPD7_1             (0x2UL << GPIO_PUPDR_PUPD7_Pos)          /*!< 0x00008000 */
#define GPIO_PUPDR_PUPD8_Pos           (16U)
#define GPIO_PUPDR_PUPD8_Msk           (0x3UL << GPIO_PUPDR_PUPD8_Pos)          /*!< 0x00030000 */
#define GPIO_PUPDR_PUPD8               GPIO_PUPDR_PUPD8_Msk
#define GPIO_PUPDR_PUPD8_0             (0x1UL << GPIO_PUPDR_PUPD8_Pos)          /*!< 0x00010000 */
#define GPIO_PUPDR_PUPD8_1             (0x2UL << GPIO_PUPDR_PUPD8_Pos)          /*!< 0x00020000 */
#define GPIO_PUPDR_PUPD9_Pos           (18U)
#define GPIO_PUPDR_PUPD9_Msk           (0x3UL << GPIO_PUPDR_PUPD9_Pos)          /*!< 0x000C0000 */
#define GPIO_PUPDR_PUPD9               GPIO_PUPDR_PUPD9_Msk
#define GPIO_PUPDR_PUPD9_0             (0x1UL << GPIO_PUPDR_PUPD9_Pos)          /*!< 0x00040000 */
#define GPIO_PUPDR_PUPD9_1             (0x2UL << GPIO_PUPDR_PUPD9_Pos)          /*!< 0x00080000 */
#define GPIO_PUPDR_PUPD10_Pos          (20U)
#define GPIO_PUPDR_PUPD10_Msk          (0x3UL << GPIO_PUPDR_PUPD10_Pos)         /*!< 0x00300000 */
#define GPIO_PUPDR_PUPD10              GPIO_PUPDR_PUPD10_Msk
#define GPIO_PUPDR_PUPD10_0            (0x1UL << GPIO_PUPDR_PUPD10_Pos)         /*!< 0x00100000 */
#define GPIO_PUPDR_PUPD10_1            (0x2UL << GPIO_PUPDR_PUPD10_Pos)         /*!< 0x00200000 */
#define GPIO_PUPDR_PUPD11_Pos          (22U)
#define GPIO_PUPDR_PUPD11_Msk          (0x3UL << GPIO_PUPDR_PUPD11_Pos)         /*!< 0x00C00000 */
#define GPIO_PUPDR_PUPD11              GPIO_PUPDR_PUPD11_Msk
#define GPIO_PUPDR_PUPD11_0            (0x1UL << GPIO_PUPDR_PUPD11_Pos)         /*!< 0x00400000 */
#define GPIO_PUPDR_PUPD11_1            (0x2UL << GPIO_PUPDR_PUPD11_Pos)         /*!< 0x00800000 */
#define GPIO_PUPDR_PUPD12_Pos          (24U)
#define GPIO_PUPDR_PUPD12_Msk          (0x3UL << GPIO_PUPDR_PUPD12_Pos)         /*!< 0x03000000 */
#define GPIO_PUPDR_PUPD12              GPIO_PUPDR_PUPD12_Msk
#define GPIO_PUPDR_PUPD12_0            (0x1UL << GPIO_PUPDR_PUPD12_Pos)         /*!< 0x01000000 */
#define GPIO_PUPDR_PUPD12_1            (0x2UL << GPIO_PUPDR_PUPD12_Pos)         /*!< 0x02000000 */
#define GPIO_PUPDR_PUPD13_Pos          (26U)
#define GPIO_PUPDR_PUPD13_Msk          (0x3UL << GPIO_PUPDR_PUPD13_Pos)         /*!< 0x0C000000 */
#define GPIO_PUPDR_PUPD13              GPIO_PUPDR_PUPD13_Msk
#define GPIO_PUPDR_PUPD13_0            (0x1UL << GPIO_PUPDR_PUPD13_Pos)         /*!< 0x04000000 */
#define GPIO_PUPDR_PUPD13_1            (0x2UL << GPIO_PUPDR_PUPD13_Pos)         /*!< 0x08000000 */
#define GPIO_PUPDR_PUPD14_Pos          (28U)
#define GPIO_PUPDR_PUPD14_Msk          (0x3UL << GPIO_PUPDR_PUPD14_Pos)         /*!< 0x30000000 */
#define GPIO_PUPDR_PUPD14              GPIO_PUPDR_PUPD14_Msk
#define GPIO_PUPDR_PUPD14_0            (0x1UL << GPIO_PUPDR_PUPD14_Pos)         /*!< 0x10000000 */
#define GPIO_PUPDR_PUPD14_1            (0x2UL << GPIO_PUPDR_PUPD14_Pos)         /*!< 0x20000000 */
#define GPIO_PUPDR_PUPD15_Pos          (30U)
#define GPIO_PUPDR_PUPD15_Msk          (0x3UL << GPIO_PUPDR_PUPD15_Pos)         /*!< 0xC0000000 */
#define GPIO_PUPDR_PUPD15              GPIO_PUPDR_PUPD15_Msk
#define GPIO_PUPDR_PUPD15_0            (0x1UL << GPIO_PUPDR_PUPD15_Pos)         /*!< 0x40000000 */
#define GPIO_PUPDR_PUPD15_1            (0x2UL << GPIO_PUPDR_PUPD15_Pos)         /*!< 0x80000000 */

/******************  Bits definition for GPIO_IDR register  *******************/
#define GPIO_IDR_ID0_Pos               (0U)
#define GPIO_IDR_ID0_Msk               (0x1UL << GPIO_IDR_ID0_Pos)              /*!< 0x00000001 */
#define GPIO_IDR_ID0                   GPIO_IDR_ID0_Msk
#define GPIO_IDR_ID1_Pos               (1U)
#define GPIO_IDR_ID1_Msk               (0x1UL << GPIO_IDR_ID1_Pos)              /*!< 0x00000002 */
#define GPIO_IDR_ID1                   GPIO_IDR_ID1_Msk
#define GPIO_IDR_ID2_Pos               (2U)
#define GPIO_IDR_ID2_Msk               (0x1UL << GPIO_IDR_ID2_Pos)              /*!< 0x00000004 */
#define GPIO_IDR_ID2                   GPIO_IDR_ID2_Msk
#define GPIO_IDR_ID3_Pos               (3U)
#define GPIO_IDR_ID3_Msk               (0x1UL << GPIO_IDR_ID3_Pos)              /*!< 0x00000008 */
#define GPIO_IDR_ID3                   GPIO_IDR_ID3_Msk
#define GPIO_IDR_ID4_Pos               (4U)
#define GPIO_IDR_ID4_Msk               (0x1UL << GPIO_IDR_ID4_Pos)              /*!< 0x00000010 */
#define GPIO_IDR_ID4                   GPIO_IDR_ID4_Msk
#define GPIO_IDR_ID5_Pos               (5U)
#define GPIO_IDR_ID5_Msk               (0x1UL << GPIO_IDR_ID5_Pos)              /*!< 0x00000020 */
#define GPIO_IDR_ID5                   GPIO_IDR_ID5_Msk
#define GPIO_IDR_ID6_Pos               (6U)
#define GPIO_IDR_ID6_Msk               (0x1UL << GPIO_IDR_ID6_Pos)              /*!< 0x00000040 */
#define GPIO_IDR_ID6                   GPIO_IDR_ID6_Msk
#define GPIO_IDR_ID7_Pos               (7U)
#define GPIO_IDR_ID7_Msk               (0x1UL << GPIO_IDR_ID7_Pos)              /*!< 0x00000080 */
#define GPIO_IDR_ID7                   GPIO_IDR_ID7_Msk
#define GPIO_IDR_ID8_Pos               (8U)
#define GPIO_IDR_ID8_Msk               (0x1UL << GPIO_IDR_ID8_Pos)              /*!< 0x00000100 */
#define GPIO_IDR_ID8                   GPIO_IDR_ID8_Msk
#define GPIO_IDR_ID9_Pos               (9U)
#define GPIO_IDR_ID9_Msk               (0x1UL << GPIO_IDR_ID9_Pos)              /*!< 0x00000200 */
#define GPIO_IDR_ID9                   GPIO_IDR_ID9_Msk
#define GPIO_IDR_ID10_Pos              (10U)
#define GPIO_IDR_ID10_Msk              (0x1UL << GPIO_IDR_ID10_Pos)             /*!< 0x00000400 */
#define GPIO_IDR_ID10                  GPIO_IDR_ID10_Msk
#define GPIO_IDR_ID11_Pos              (11U)
#define GPIO_IDR_ID11_Msk              (0x1UL << GPIO_IDR_ID11_Pos)             /*!< 0x00000800 */
#define GPIO_IDR_ID11                  GPIO_IDR_ID11_Msk
#define GPIO_IDR_ID12_Pos              (12U)
#define GPIO_IDR_ID12_Msk              (0x1UL << GPIO_IDR_ID12_Pos)             /*!< 0x00001000 */
#define GPIO_IDR_ID12                  GPIO_IDR_ID12_Msk
#define GPIO_IDR_ID13_Pos              (13U)
#define GPIO_IDR_ID13_Msk              (0x1UL << GPIO_IDR_ID13_Pos)             /*!< 0x00002000 */
#define GPIO_IDR_ID13                  GPIO_IDR_ID13_Msk
#define GPIO_IDR_ID14_Pos              (14U)
#define GPIO_IDR_ID14_Msk              (0x1UL << GPIO_IDR_ID14_Pos)             /*!< 0x00004000 */
#define GPIO_IDR_ID14                  GPIO_IDR_ID14_Msk
#define GPIO_IDR_ID15_Pos              (15U)
#define GPIO_IDR_ID15_Msk              (0x1UL << GPIO_IDR_ID15_Pos)             /*!< 0x00008000 */
#define GPIO_IDR_ID15                  GPIO_IDR_ID15_Msk

/******************  Bits definition for GPIO_ODR register  *******************/
#define GPIO_ODR_OD0_Pos               (0U)
#define GPIO_ODR_OD0_Msk               (0x1UL << GPIO_ODR_OD0_Pos)              /*!< 0x00000001 */
#define GPIO_ODR_OD0                   GPIO_ODR_OD0_Msk
#define GPIO_ODR_OD1_Pos               (1U)
#define GPIO_ODR_OD1_Msk               (0x1UL << GPIO_ODR_OD1_Pos)              /*!< 0x00000002 */
#define GPIO_ODR_OD1                   GPIO_ODR_OD1_Msk
#define GPIO_ODR_OD2_Pos               (2U)
#define GPIO_ODR_OD2_Msk               (0x1UL << GPIO_ODR_OD2_Pos)              /*!< 0x00000004 */
#define GPIO_ODR_OD2                   GPIO_ODR_OD2_Msk
#define GPIO_ODR_OD3_Pos               (3U)
#define GPIO_ODR_OD3_Msk               (0x1UL << GPIO_ODR_OD3_Pos)              /*!< 0x00000008 */
#define GPIO_ODR_OD3                   GPIO_ODR_OD3_Msk
#define GPIO_ODR_OD4_Pos               (4U)
#define GPIO_ODR_OD4_Msk               (0x1UL << GPIO_ODR_OD4_Pos)              /*!< 0x00000010 */
#define GPIO_ODR_OD4                   GPIO_ODR_OD4_Msk
#define GPIO_ODR_OD5_Pos               (5U)
#define GPIO_ODR_OD5_Msk               (0x1UL << GPIO_ODR_OD5_Pos)              /*!< 0x00000020 */
#define GPIO_ODR_OD5                   GPIO_ODR_OD5_Msk
#define GPIO_ODR_OD6_Pos               (6U)
#define GPIO_ODR_OD6_Msk               (0x1UL << GPIO_ODR_OD6_Pos)              /*!< 0x00000040 */
#define GPIO_ODR_OD6                   GPIO_ODR_OD6_Msk
#define GPIO_ODR_OD7_Pos               (7U)
#define GPIO_ODR_OD7_Msk               (0x1UL << GPIO_ODR_OD7_Pos)              /*!< 0x00000080 */
#define GPIO_ODR_OD7                   GPIO_ODR_OD7_Msk
#define GPIO_ODR_OD8_Pos               (8U)
#define GPIO_ODR_OD8_Msk               (0x1UL << GPIO_ODR_OD8_Pos)              /*!< 0x00000100 */
#define GPIO_ODR_OD8                   GPIO_ODR_OD8_Msk
#define GPIO_ODR_OD9_Pos               (9U)
#define GPIO_ODR_OD9_Msk               (0x1UL << GPIO_ODR_OD9_Pos)              /*!< 0x00000200 */
#define GPIO_ODR_OD9                   GPIO_ODR_OD9_Msk
#define GPIO_ODR_OD10_Pos              (10U)
#define GPIO_ODR_OD10_Msk              (0x1UL << GPIO_ODR_OD10_Pos)             /*!< 0x00000400 */
#define GPIO_ODR_OD10                  GPIO_ODR_OD10_Msk
#define GPIO_ODR_OD11_Pos              (11U)
#define GPIO_ODR_OD11_Msk              (0x1UL << GPIO_ODR_OD11_Pos)             /*!< 0x00000800 */
#define GPIO_ODR_OD11                  GPIO_ODR_OD11_Msk
#define GPIO_ODR_OD12_Pos              (12U)
#define GPIO_ODR_OD12_Msk              (0x1UL << GPIO_ODR_OD12_Pos)             /*!< 0x00001000 */
#define GPIO_ODR_OD12                  GPIO_ODR_OD12_Msk
#define GPIO_ODR_OD13_Pos              (13U)
#define GPIO_ODR_OD13_Msk              (0x1UL << GPIO_ODR_OD13_Pos)             /*!< 0x00002000 */
#define GPIO_ODR_OD13                  GPIO_ODR_OD13_Msk
#define GPIO_ODR_OD14_Pos              (14U)
#define GPIO_ODR_OD14_Msk              (0x1UL << GPIO_ODR_OD14_Pos)             /*!< 0x00004000 */
#define GPIO_ODR_OD14                  GPIO_ODR_OD14_Msk
#define GPIO_ODR_OD15_Pos              (15U)
#define GPIO_ODR_OD15_Msk              (0x1UL << GPIO_ODR_OD15_Pos)             /*!< 0x00008000 */
#define GPIO_ODR_OD15                  GPIO_ODR_OD15_Msk

/******************  Bits definition for GPIO_BSRR register  ******************/
#define GPIO_BSRR_BS0_Pos              (0U)
#define GPIO_BSRR_BS0_Msk              (0x1UL << GPIO_BSRR_BS0_Pos)             /*!< 0x00000001 */
#define GPIO_BSRR_BS0                  GPIO_BSRR_BS0_Msk
#define GPIO_BSRR_BS1_Pos              (1U)
#define GPIO_BSRR_BS1_Msk              (0x1UL << GPIO_BSRR_BS1_Pos)             /*!< 0x00000002 */
#define GPIO_BSRR_BS1                  GPIO_BSRR_BS1_Msk
#define GPIO_BSRR_BS2_Pos              (2U)
#define GPIO_BSRR_BS2_Msk              (0x1UL << GPIO_BSRR_BS2_Pos)             /*!< 0x00000004 */
#define GPIO_BSRR_BS2                  GPIO_BSRR_BS2_Msk
#define GPIO_BSRR_BS3_Pos              (3U)
#define GPIO_BSRR_BS3_Msk              (0x1UL << GPIO_BSRR_BS3_Pos)             /*!< 0x00000008 */
#define GPIO_BSRR_BS3                  GPIO_BSRR_BS3_Msk
#define GPIO_BSRR_BS4_Pos              (4U)
#define GPIO_BSRR_BS4_Msk              (0x1UL << GPIO_BSRR_BS4_Pos)             /*!< 0x00000010 */
#define GPIO_BSRR_BS4                  GPIO_BSRR_BS4_Msk
#define GPIO_BSRR_BS5_Pos              (5U)
#define GPIO_BSRR_BS5_Msk              (0x1UL << GPIO_BSRR_BS5_Pos)             /*!< 0x00000020 */
#define GPIO_BSRR_BS5                  GPIO_BSRR_BS5_Msk
#define GPIO_BSRR_BS6_Pos              (6U)
#define GPIO_BSRR_BS6_Msk              (0x1UL << GPIO_BSRR_BS6_Pos)             /*!< 0x00000040 */
#define GPIO_BSRR_BS6                  GPIO_BSRR_BS6_Msk
#define GPIO_BSRR_BS7_Pos              (7U)
#define GPIO_BSRR_BS7_Msk              (0x1UL << GPIO_BSRR_BS7_Pos)             /*!< 0x00000080 */
#define GPIO_BSRR_BS7                  GPIO_BSRR_BS7_Msk
#define GPIO_BSRR_BS8_Pos              (8U)
#define GPIO_BSRR_BS8_Msk              (0x1UL << GPIO_BSRR_BS8_Pos)             /*!< 0x00000100 */
#define GPIO_BSRR_BS8                  GPIO_BSRR_BS8_Msk
#define GPIO_BSRR_BS9_Pos              (9U)
#define GPIO_BSRR_BS9_Msk              (0x1UL << GPIO_BSRR_BS9_Pos)             /*!< 0x00000200 */
#define GPIO_BSRR_BS9                  GPIO_BSRR_BS9_Msk
#define GPIO_BSRR_BS10_Pos             (10U)
#define GPIO_BSRR_BS10_Msk             (0x1UL << GPIO_BSRR_BS10_Pos)            /*!< 0x00000400 */
#define GPIO_BSRR_BS10                 GPIO_BSRR_BS10_Msk
#define GPIO_BSRR_BS11_Pos             (11U)
#define GPIO_BSRR_BS11_Msk             (0x1UL << GPIO_BSRR_BS11_Pos)            /*!< 0x00000800 */
#define GPIO_BSRR_BS11                 GPIO_BSRR_BS11_Msk
#define GPIO_BSRR_BS12_Pos             (12U)
#define GPIO_BSRR_BS12_Msk             (0x1UL << GPIO_BSRR_BS12_Pos)            /*!< 0x00001000 */
#define GPIO_BSRR_BS12                 GPIO_BSRR_BS12_Msk
#define GPIO_BSRR_BS13_Pos             (13U)
#define GPIO_BSRR_BS13_Msk             (0x1UL << GPIO_BSRR_BS13_Pos)            /*!< 0x00002000 */
#define GPIO_BSRR_BS13                 GPIO_BSRR_BS13_Msk
#define GPIO_BSRR_BS14_Pos             (14U)
#define GPIO_BSRR_BS14_Msk             (0x1UL << GPIO_BSRR_BS14_Pos)            /*!< 0x00004000 */
#define GPIO_BSRR_BS14                 GPIO_BSRR_BS14_Msk
#define GPIO_BSRR_BS15_Pos             (15U)
#define GPIO_BSRR_BS15_Msk             (0x1UL << GPIO_BSRR_BS15_Pos)            /*!< 0x00008000 */
#define GPIO_BSRR_BS15                 GPIO_BSRR_BS15_Msk
#define GPIO_BSRR_BR0_Pos              (16U)
#define GPIO_BSRR_BR0_Msk              (0x1UL << GPIO_BSRR_BR0_Pos)             /*!< 0x00010000 */
#define GPIO_BSRR_BR0                  GPIO_BSRR_BR0_Msk
#define GPIO_BSRR_BR1_Pos              (17U)
#define GPIO_BSRR_BR1_Msk              (0x1UL << GPIO_BSRR_BR1_Pos)             /*!< 0x00020000 */
#define GPIO_BSRR_BR1                  GPIO_BSRR_BR1_Msk
#define GPIO_BSRR_BR2_Pos              (18U)
#define GPIO_BSRR_BR2_Msk              (0x1UL << GPIO_BSRR_BR2_Pos)             /*!< 0x00040000 */
#define GPIO_BSRR_BR2                  GPIO_BSRR_BR2_Msk
#define GPIO_BSRR_BR3_Pos              (19U)
#define GPIO_BSRR_BR3_Msk              (0x1UL << GPIO_BSRR_BR3_Pos)             /*!< 0x00080000 */
#define GPIO_BSRR_BR3                  GPIO_BSRR_BR3_Msk
#define GPIO_BSRR_BR4_Pos              (20U)
#define GPIO_BSRR_BR4_Msk              (0x1UL << GPIO_BSRR_BR4_Pos)             /*!< 0x00100000 */
#define GPIO_BSRR_BR4                  GPIO_BSRR_BR4_Msk
#define GPIO_BSRR_BR5_Pos              (21U)
#define GPIO_BSRR_BR5_Msk              (0x1UL << GPIO_BSRR_BR5_Pos)             /*!< 0x00200000 */
#define GPIO_BSRR_BR5                  GPIO_BSRR_BR5_Msk
#define GPIO_BSRR_BR6_Pos              (22U)
#define GPIO_BSRR_BR6_Msk              (0x1UL << GPIO_BSRR_BR6_Pos)             /*!< 0x00400000 */
#define GPIO_BSRR_BR6                  GPIO_BSRR_BR6_Msk
#define GPIO_BSRR_BR7_Pos              (23U)
#define GPIO_BSRR_BR7_Msk              (0x1UL << GPIO_BSRR_BR7_Pos)             /*!< 0x00800000 */
#define GPIO_BSRR_BR7                  GPIO_BSRR_BR7_Msk
#define GPIO_BSRR_BR8_Pos              (24U)
#define GPIO_BSRR_BR8_Msk              (0x1UL << GPIO_BSRR_BR8_Pos)             /*!< 0x01000000 */
#define GPIO_BSRR_BR8                  GPIO_BSRR_BR8_Msk
#define GPIO_BSRR_BR9_Pos              (25U)
#define GPIO_BSRR_BR9_Msk              (0x1UL << GPIO_BSRR_BR9_Pos)             /*!< 0x02000000 */
#define GPIO_BSRR_BR9                  GPIO_BSRR_BR9_Msk
#define GPIO_BSRR_BR10_Pos             (26U)
#define GPIO_BSRR_BR10_Msk             (0x1UL << GPIO_BSRR_BR10_Pos)            /*!< 0x04000000 */
#define GPIO_BSRR_BR10                 GPIO_BSRR_BR10_Msk
#define GPIO_BSRR_BR11_Pos             (27U)
#define GPIO_BSRR_BR11_Msk             (0x1UL << GPIO_BSRR_BR11_Pos)            /*!< 0x08000000 */
#define GPIO_BSRR_BR11                 GPIO_BSRR_BR11_Msk
#define GPIO_BSRR_BR12_Pos             (28U)
#define GPIO_BSRR_BR12_Msk             (0x1UL << GPIO_BSRR_BR12_Pos)            /*!< 0x10000000 */
#define GPIO_BSRR_BR12                 GPIO_BSRR_BR12_Msk
#define GPIO_BSRR_BR13_Pos             (29U)
#define GPIO_BSRR_BR13_Msk             (0x1UL << GPIO_BSRR_BR13_Pos)            /*!< 0x20000000 */
#define GPIO_BSRR_BR13                 GPIO_BSRR_BR13_Msk
#define GPIO_BSRR_BR14_Pos             (30U)
#define GPIO_BSRR_BR14_Msk             (0x1UL << GPIO_BSRR_BR14_Pos)            /*!< 0x40000000 */
#define GPIO_BSRR_BR14                 GPIO_BSRR_BR14_Msk
#define GPIO_BSRR_BR15_Pos             (31U)
#define GPIO_BSRR_BR15_Msk             (0x1UL << GPIO_BSRR_BR15_Pos)            /*!< 0x80000000 */
#define GPIO_BSRR_BR15                 GPIO_BSRR_BR15_Msk

/****************** Bit definition for GPIO_LCKR register *********************/
#define GPIO_LCKR_LCK0_Pos             (0U)
#define GPIO_LCKR_LCK0_Msk             (0x1UL << GPIO_LCKR_LCK0_Pos)            /*!< 0x00000001 */
#define GPIO_LCKR_LCK0                 GPIO_LCKR_LCK0_Msk
#define GPIO_LCKR_LCK1_Pos             (1U)
#define GPIO_LCKR_LCK1_Msk             (0x1UL << GPIO_LCKR_LCK1_Pos)            /*!< 0x00000002 */
#define GPIO_LCKR_LCK1                 GPIO_LCKR_LCK1_Msk
#define GPIO_LCKR_LCK2_Pos             (2U)
#define GPIO_LCKR_LCK2_Msk             (0x1UL << GPIO_LCKR_LCK2_Pos)            /*!< 0x00000004 */
#define GPIO_LCKR_LCK2                 GPIO_LCKR_LCK2_Msk
#define GPIO_LCKR_LCK3_Pos             (3U)
#define GPIO_LCKR_LCK3_Msk             (0x1UL << GPIO_LCKR_LCK3_Pos)            /*!< 0x00000008 */
#define GPIO_LCKR_LCK3                 GPIO_LCKR_LCK3_Msk
#define GPIO_LCKR_LCK4_Pos             (4U)
#define GPIO_LCKR_LCK4_Msk             (0x1UL << GPIO_LCKR_LCK4_Pos)            /*!< 0x00000010 */
#define GPIO_LCKR_LCK4                 GPIO_LCKR_LCK4_Msk
#define GPIO_LCKR_LCK5_Pos             (5U)
#define GPIO_LCKR_LCK5_Msk             (0x1UL << GPIO_LCKR_LCK5_Pos)            /*!< 0x00000020 */
#define GPIO_LCKR_LCK5                 GPIO_LCKR_LCK5_Msk
#define GPIO_LCKR_LCK6_Pos             (6U)
#define GPIO_LCKR_LCK6_Msk             (0x1UL << GPIO_LCKR_LCK6_Pos)            /*!< 0x00000040 */
#define GPIO_LCKR_LCK6                 GPIO_LCKR_LCK6_Msk
#define GPIO_LCKR_LCK7_Pos             (7U)
#define GPIO_LCKR_LCK7_Msk             (0x1UL << GPIO_LCKR_LCK7_Pos)            /*!< 0x00000080 */
#define GPIO_LCKR_LCK7                 GPIO_LCKR_LCK7_Msk
#define GPIO_LCKR_LCK8_Pos             (8U)
#define GPIO_LCKR_LCK8_Msk             (0x1UL << GPIO_LCKR_LCK8_Pos)            /*!< 0x00000100 */
#define GPIO_LCKR_LCK8                 GPIO_LCKR_LCK8_Msk
#define GPIO_LCKR_LCK9_Pos             (9U)
#define GPIO_LCKR_LCK9_Msk             (0x1UL << GPIO_LCKR_LCK9_Pos)            /*!< 0x00000200 */
#define GPIO_LCKR_LCK9                 GPIO_LCKR_LCK9_Msk
#define GPIO_LCKR_LCK10_Pos            (10U)
#define GPIO_LCKR_LCK10_Msk            (0x1UL << GPIO_LCKR_LCK10_Pos)           /*!< 0x00000400 */
#define GPIO_LCKR_LCK10                GPIO_LCKR_LCK10_Msk
#define GPIO_LCKR_LCK11_Pos            (11U)
#define GPIO_LCKR_LCK11_Msk            (0x1UL << GPIO_LCKR_LCK11_Pos)           /*!< 0x00000800 */
#define GPIO_LCKR_LCK11                GPIO_LCKR_LCK11_Msk
#define GPIO_LCKR_LCK12_Pos            (12U)
#define GPIO_LCKR_LCK12_Msk            (0x1UL << GPIO_LCKR_LCK12_Pos)           /*!< 0x00001000 */
#define GPIO_LCKR_LCK12                GPIO_LCKR_LCK12_Msk
#define GPIO_LCKR_LCK13_Pos            (13U)
#define GPIO_LCKR_LCK13_Msk            (0x1UL << GPIO_LCKR_LCK13_Pos)           /*!< 0x00002000 */
#define GPIO_LCKR_LCK13                GPIO_LCKR_LCK13_Msk
#define GPIO_LCKR_LCK14_Pos            (14U)
#define GPIO_LCKR_LCK14_Msk            (0x1UL << GPIO_LCKR_LCK14_Pos)           /*!< 0x00004000 */
#define GPIO_LCKR_LCK14                GPIO_LCKR_LCK14_Msk
#define GPIO_LCKR_LCK15_Pos            (15U)
#define GPIO_LCKR_LCK15_Msk            (0x1UL << GPIO_LCKR_LCK15_Pos)           /*!< 0x00008000 */
#define GPIO_LCKR_LCK15                GPIO_LCKR_LCK15_Msk
#define GPIO_LCKR_LCKK_Pos             (16U)
#define GPIO_LCKR_LCKK_Msk             (0x1UL << GPIO_LCKR_LCKK_Pos)            /*!< 0x00010000 */
#define GPIO_LCKR_LCKK                 GPIO_LCKR_LCKK_Msk

/****************** Bit definition for GPIO_AFRL register *********************/
#define GPIO_AFRL_AFSEL0_Pos           (0U)
#define GPIO_AFRL_AFSEL0_Msk           (0xFUL << GPIO_AFRL_AFSEL0_Pos)          /*!< 0x0000000F */
#define GPIO_AFRL_AFSEL0               GPIO_AFRL_AFSEL0_Msk
#define GPIO_AFRL_AFSEL0_0             (0x1UL << GPIO_AFRL_AFSEL0_Pos)          /*!< 0x00000001 */
#define GPIO_AFRL_AFSEL0_1             (0x2UL << GPIO_AFRL_AFSEL0_Pos)          /*!< 0x00000002 */
#define GPIO_AFRL_AFSEL0_2             (0x4UL << GPIO_AFRL_AFSEL0_Pos)          /*!< 0x00000004 */
#define GPIO_AFRL_AFSEL0_3             (0x8UL << GPIO_AFRL_AFSEL0_Pos)          /*!< 0x00000008 */
#define GPIO_AFRL_AFSEL1_Pos           (4U)
#define GPIO_AFRL_AFSEL1_Msk           (0xFUL << GPIO_AFRL_AFSEL1_Pos)          /*!< 0x000000F0 */
#define GPIO_AFRL_AFSEL1               GPIO_AFRL_AFSEL1_Msk
#define GPIO_AFRL_AFSEL1_0             (0x1UL << GPIO_AFRL_AFSEL1_Pos)          /*!< 0x00000010 */
#define GPIO_AFRL_AFSEL1_1             (0x2UL << GPIO_AFRL_AFSEL1_Pos)          /*!< 0x00000020 */
#define GPIO_AFRL_AFSEL1_2             (0x4UL << GPIO_AFRL_AFSEL1_Pos)          /*!< 0x00000040 */
#define GPIO_AFRL_AFSEL1_3             (0x8UL << GPIO_AFRL_AFSEL1_Pos)          /*!< 0x00000080 */
#define GPIO_AFRL_AFSEL2_Pos           (8U)
#define GPIO_AFRL_AFSEL2_Msk           (0xFUL << GPIO_AFRL_AFSEL2_Pos)          /*!< 0x00000F00 */
#define GPIO_AFRL_AFSEL2               GPIO_AFRL_AFSEL2_Msk
#define GPIO_AFRL_AFSEL2_0             (0x1UL << GPIO_AFRL_AFSEL2_Pos)          /*!< 0x00000100 */
#define GPIO_AFRL_AFSEL2_1             (0x2UL << GPIO_AFRL_AFSEL2_Pos)          /*!< 0x00000200 */
#define GPIO_AFRL_AFSEL2_2             (0x4UL << GPIO_AFRL_AFSEL2_Pos)          /*!< 0x00000400 */
#define GPIO_AFRL_AFSEL2_3             (0x8UL << GPIO_AFRL_AFSEL2_Pos)          /*!< 0x00000800 */
#define GPIO_AFRL_AFSEL3_Pos           (12U)
#define GPIO_AFRL_AFSEL3_Msk           (0xFUL << GPIO_AFRL_AFSEL3_Pos)          /*!< 0x0000F000 */
#define GPIO_AFRL_AFSEL3               GPIO_AFRL_AFSEL3_Msk
#define GPIO_AFRL_AFSEL3_0             (0x1UL << GPIO_AFRL_AFSEL3_Pos)          /*!< 0x00001000 */
#define GPIO_AFRL_AFSEL3_1             (0x2UL << GPIO_AFRL_AFSEL3_Pos)          /*!< 0x00002000 */
#define GPIO_AFRL_AFSEL3_2             (0x4UL << GPIO_AFRL_AFSEL3_Pos)          /*!< 0x00004000 */
#define GPIO_AFRL_AFSEL3_3             (0x8UL << GPIO_AFRL_AFSEL3_Pos)          /*!< 0x00008000 */
#define GPIO_AFRL_AFSEL4_Pos           (16U)
#define GPIO_AFRL_AFSEL4_Msk           (0xFUL << GPIO_AFRL_AFSEL4_Pos)          /*!< 0x000F0000 */
#define GPIO_AFRL_AFSEL4               GPIO_AFRL_AFSEL4_Msk
#define GPIO_AFRL_AFSEL4_0             (0x1UL << GPIO_AFRL_AFSEL4_Pos)          /*!< 0x00010000 */
#define GPIO_AFRL_AFSEL4_1             (0x2UL << GPIO_AFRL_AFSEL4_Pos)          /*!< 0x00020000 */
#define GPIO_AFRL_AFSEL4_2             (0x4UL << GPIO_AFRL_AFSEL4_Pos)          /*!< 0x00040000 */
#define GPIO_AFRL_AFSEL4_3             (0x8UL << GPIO_AFRL_AFSEL4_Pos)          /*!< 0x00080000 */
#define GPIO_AFRL_AFSEL5_Pos           (20U)
#define GPIO_AFRL_AFSEL5_Msk           (0xFUL << GPIO_AFRL_AFSEL5_Pos)          /*!< 0x00F00000 */
#define GPIO_AFRL_AFSEL5               GPIO_AFRL_AFSEL5_Msk
#define GPIO_AFRL_AFSEL5_0             (0x1UL << GPIO_AFRL_AFSEL5_Pos)          /*!< 0x00100000 */
#define GPIO_AFRL_AFSEL5_1             (0x2UL << GPIO_AFRL_AFSEL5_Pos)          /*!< 0x00200000 */
#define GPIO_AFRL_AFSEL5_2             (0x4UL << GPIO_AFRL_AFSEL5_Pos)          /*!< 0x00400000 */
#define GPIO_AFRL_AFSEL5_3             (0x8UL << GPIO_AFRL_AFSEL5_Pos)          /*!< 0x00800000 */
#define GPIO_AFRL_AFSEL6_Pos           (24U)
#define GPIO_AFRL_AFSEL6_Msk           (0xFUL << GPIO_AFRL_AFSEL6_Pos)          /*!< 0x0F000000 */
#define GPIO_AFRL_AFSEL6               GPIO_AFRL_AFSEL6_Msk
#define GPIO_AFRL_AFSEL6_0             (0x1UL << GPIO_AFRL_AFSEL6_Pos)          /*!< 0x01000000 */
#define GPIO_AFRL_AFSEL6_1             (0x2UL << GPIO_AFRL_AFSEL6_Pos)          /*!< 0x02000000 */
#define GPIO_AFRL_AFSEL6_2             (0x4UL << GPIO_AFRL_AFSEL6_Pos)          /*!< 0x04000000 */
#define GPIO_AFRL_AFSEL6_3             (0x8UL << GPIO_AFRL_AFSEL6_Pos)          /*!< 0x08000000 */
#define GPIO_AFRL_AFSEL7_Pos           (28U)
#define GPIO_AFRL_AFSEL7_Msk           (0xFUL << GPIO_AFRL_AFSEL7_Pos)          /*!< 0xF0000000 */
#define GPIO_AFRL_AFSEL7               GPIO_AFRL_AFSEL7_Msk
#define GPIO_AFRL_AFSEL7_0             (0x1UL << GPIO_AFRL_AFSEL7_Pos)          /*!< 0x10000000 */
#define GPIO_AFRL_AFSEL7_1             (0x2UL << GPIO_AFRL_AFSEL7_Pos)          /*!< 0x20000000 */
#define GPIO_AFRL_AFSEL7_2             (0x4UL << GPIO_AFRL_AFSEL7_Pos)          /*!< 0x40000000 */
#define GPIO_AFRL_AFSEL7_3             (0x8UL << GPIO_AFRL_AFSEL7_Pos)          /*!< 0x80000000 */

/****************** Bit definition for GPIO_AFRH register *********************/
#define GPIO_AFRH_AFSEL8_Pos           (0U)
#define GPIO_AFRH_AFSEL8_Msk           (0xFUL << GPIO_AFRH_AFSEL8_Pos)          /*!< 0x0000000F */
#define GPIO_AFRH_AFSEL8               GPIO_AFRH_AFSEL8_Msk
#define GPIO_AFRH_AFSEL8_0             (0x1UL << GPIO_AFRH_AFSEL8_Pos)          /*!< 0x00000001 */
#define GPIO_AFRH_AFSEL8_1             (0x2UL << GPIO_AFRH_AFSEL8_Pos)          /*!< 0x00000002 */
#define GPIO_AFRH_AFSEL8_2             (0x4UL << GPIO_AFRH_AFSEL8_Pos)          /*!< 0x00000004 */
#define GPIO_AFRH_AFSEL8_3             (0x8UL << GPIO_AFRH_AFSEL8_Pos)          /*!< 0x00000008 */
#define GPIO_AFRH_AFSEL9_Pos           (4U)
#define GPIO_AFRH_AFSEL9_Msk           (0xFUL << GPIO_AFRH_AFSEL9_Pos)          /*!< 0x000000F0 */
#define GPIO_AFRH_AFSEL9               GPIO_AFRH_AFSEL9_Msk
#define GPIO_AFRH_AFSEL9_0             (0x1UL << GPIO_AFRH_AFSEL9_Pos)          /*!< 0x00000010 */
#define GPIO_AFRH_AFSEL9_1             (0x2UL << GPIO_AFRH_AFSEL9_Pos)          /*!< 0x00000020 */
#define GPIO_AFRH_AFSEL9_2             (0x4UL << GPIO_AFRH_AFSEL9_Pos)          /*!< 0x00000040 */
#define GPIO_AFRH_AFSEL9_3             (0x8UL << GPIO_AFRH_AFSEL9_Pos)          /*!< 0x00000080 */
#define GPIO_AFRH_AFSEL10_Pos          (8U)
#define GPIO_AFRH_AFSEL10_Msk          (0xFUL << GPIO_AFRH_AFSEL10_Pos)         /*!< 0x00000F00 */
#define GPIO_AFRH_AFSEL10              GPIO_AFRH_AFSEL10_Msk
#define GPIO_AFRH_AFSEL10_0            (0x1UL << GPIO_AFRH_AFSEL10_Pos)         /*!< 0x00000100 */
#define GPIO_AFRH_AFSEL10_1            (0x2UL << GPIO_AFRH_AFSEL10_Pos)         /*!< 0x00000200 */
#define GPIO_AFRH_AFSEL10_2            (0x4UL << GPIO_AFRH_AFSEL10_Pos)         /*!< 0x00000400 */
#define GPIO_AFRH_AFSEL10_3            (0x8UL << GPIO_AFRH_AFSEL10_Pos)         /*!< 0x00000800 */
#define GPIO_AFRH_AFSEL11_Pos          (12U)
#define GPIO_AFRH_AFSEL11_Msk          (0xFUL << GPIO_AFRH_AFSEL11_Pos)         /*!< 0x0000F000 */
#define GPIO_AFRH_AFSEL11              GPIO_AFRH_AFSEL11_Msk
#define GPIO_AFRH_AFSEL11_0            (0x1UL << GPIO_AFRH_AFSEL11_Pos)         /*!< 0x00001000 */
#define GPIO_AFRH_AFSEL11_1            (0x2UL << GPIO_AFRH_AFSEL11_Pos)         /*!< 0x00002000 */
#define GPIO_AFRH_AFSEL11_2            (0x4UL << GPIO_AFRH_AFSEL11_Pos)         /*!< 0x00004000 */
#define GPIO_AFRH_AFSEL11_3            (0x8UL << GPIO_AFRH_AFSEL11_Pos)         /*!< 0x00008000 */
#define GPIO_AFRH_AFSEL12_Pos          (16U)
#define GPIO_AFRH_AFSEL12_Msk          (0xFUL << GPIO_AFRH_AFSEL12_Pos)         /*!< 0x000F0000 */
#define GPIO_AFRH_AFSEL12              GPIO_AFRH_AFSEL12_Msk
#define GPIO_AFRH_AFSEL12_0            (0x1UL << GPIO_AFRH_AFSEL12_Pos)         /*!< 0x00010000 */
#define GPIO_AFRH_AFSEL12_1            (0x2UL << GPIO_AFRH_AFSEL12_Pos)         /*!< 0x00020000 */
#define GPIO_AFRH_AFSEL12_2            (0x4UL << GPIO_AFRH_AFSEL12_Pos)         /*!< 0x00040000 */
#define GPIO_AFRH_AFSEL12_3            (0x8UL << GPIO_AFRH_AFSEL12_Pos)         /*!< 0x00080000 */
#define GPIO_AFRH_AFSEL13_Pos          (20U)
#define GPIO_AFRH_AFSEL13_Msk          (0xFUL << GPIO_AFRH_AFSEL13_Pos)         /*!< 0x00F00000 */
#define GPIO_AFRH_AFSEL13              GPIO_AFRH_AFSEL13_Msk
#define GPIO_AFRH_AFSEL13_0            (0x1UL << GPIO_AFRH_AFSEL13_Pos)         /*!< 0x00100000 */
#define GPIO_AFRH_AFSEL13_1            (0x2UL << GPIO_AFRH_AFSEL13_Pos)         /*!< 0x00200000 */
#define GPIO_AFRH_AFSEL13_2            (0x4UL << GPIO_AFRH_AFSEL13_Pos)         /*!< 0x00400000 */
#define GPIO_AFRH_AFSEL13_3            (0x8UL << GPIO_AFRH_AFSEL13_Pos)         /*!< 0x00800000 */
#define GPIO_AFRH_AFSEL14_Pos          (24U)
#define GPIO_AFRH_AFSEL14_Msk          (0xFUL << GPIO_AFRH_AFSEL14_Pos)         /*!< 0x0F000000 */
#define GPIO_AFRH_AFSEL14              GPIO_AFRH_AFSEL14_Msk
#define GPIO_AFRH_AFSEL14_0            (0x1UL << GPIO_AFRH_AFSEL14_Pos)         /*!< 0x01000000 */
#define GPIO_AFRH_AFSEL14_1            (0x2UL << GPIO_AFRH_AFSEL14_Pos)         /*!< 0x02000000 */
#define GPIO_AFRH_AFSEL14_2            (0x4UL << GPIO_AFRH_AFSEL14_Pos)         /*!< 0x04000000 */
#define GPIO_AFRH_AFSEL14_3            (0x8UL << GPIO_AFRH_AFSEL14_Pos)         /*!< 0x08000000 */
#define GPIO_AFRH_AFSEL15_Pos          (28U)
#define GPIO_AFRH_AFSEL15_Msk          (0xFUL << GPIO_AFRH_AFSEL15_Pos)         /*!< 0xF0000000 */
#define GPIO_AFRH_AFSEL15              GPIO_AFRH_AFSEL15_Msk
#define GPIO_AFRH_AFSEL15_0            (0x1UL << GPIO_AFRH_AFSEL15_Pos)         /*!< 0x10000000 */
#define GPIO_AFRH_AFSEL15_1            (0x2UL << GPIO_AFRH_AFSEL15_Pos)         /*!< 0x20000000 */
#define GPIO_AFRH_AFSEL15_2            (0x4UL << GPIO_AFRH_AFSEL15_Pos)         /*!< 0x40000000 */
#define GPIO_AFRH_AFSEL15_3            (0x8UL << GPIO_AFRH_AFSEL15_Pos)         /*!< 0x80000000 */

/******************  Bits definition for GPIO_BRR register  ******************/
#define GPIO_BRR_BR0_Pos               (0U)
#define GPIO_BRR_BR0_Msk               (0x1UL << GPIO_BRR_BR0_Pos)              /*!< 0x00000001 */
#define GPIO_BRR_BR0                   GPIO_BRR_BR0_Msk
#define GPIO_BRR_BR1_Pos               (1U)
#define GPIO_BRR_BR1_Msk               (0x1UL << GPIO_BRR_BR1_Pos)              /*!< 0x00000002 */
#define GPIO_BRR_BR1                   GPIO_BRR_BR1_Msk
#define GPIO_BRR_BR2_Pos               (2U)
#define GPIO_BRR_BR2_Msk               (0x1UL << GPIO_BRR_BR2_Pos)              /*!< 0x00000004 */
#define GPIO_BRR_BR2                   GPIO_BRR_BR2_Msk
#define GPIO_BRR_BR3_Pos               (3U)
#define GPIO_BRR_BR3_Msk               (0x1UL << GPIO_BRR_BR3_Pos)              /*!< 0x00000008 */
#define GPIO_BRR_BR3                   GPIO_BRR_BR3_Msk
#define GPIO_BRR_BR4_Pos               (4U)
#define GPIO_BRR_BR4_Msk               (0x1UL << GPIO_BRR_BR4_Pos)              /*!< 0x00000010 */
#define GPIO_BRR_BR4                   GPIO_BRR_BR4_Msk
#define GPIO_BRR_BR5_Pos               (5U)
#define GPIO_BRR_BR5_Msk               (0x1UL << GPIO_BRR_BR5_Pos)              /*!< 0x00000020 */
#define GPIO_BRR_BR5                   GPIO_BRR_BR5_Msk
#define GPIO_BRR_BR6_Pos               (6U)
#define GPIO_BRR_BR6_Msk               (0x1UL << GPIO_BRR_BR6_Pos)              /*!< 0x00000040 */
#define GPIO_BRR_BR6                   GPIO_BRR_BR6_Msk
#define GPIO_BRR_BR7_Pos               (7U)
#define GPIO_BRR_BR7_Msk               (0x1UL << GPIO_BRR_BR7_Pos)              /*!< 0x00000080 */
#define GPIO_BRR_BR7                   GPIO_BRR_BR7_Msk
#define GPIO_BRR_BR8_Pos               (8U)
#define GPIO_BRR_BR8_Msk               (0x1UL << GPIO_BRR_BR8_Pos)              /*!< 0x00000100 */
#define GPIO_BRR_BR8                   GPIO_BRR_BR8_Msk
#define GPIO_BRR_BR9_Pos               (9U)
#define GPIO_BRR_BR9_Msk               (0x1UL << GPIO_BRR_BR9_Pos)              /*!< 0x00000200 */
#define GPIO_BRR_BR9                   GPIO_BRR_BR9_Msk
#define GPIO_BRR_BR10_Pos              (10U)
#define GPIO_BRR_BR10_Msk              (0x1UL << GPIO_BRR_BR10_Pos)             /*!< 0x00000400 */
#define GPIO_BRR_BR10                  GPIO_BRR_BR10_Msk
#define GPIO_BRR_BR11_Pos              (11U)
#define GPIO_BRR_BR11_Msk              (0x1UL << GPIO_BRR_BR11_Pos)             /*!< 0x00000800 */
#define GPIO_BRR_BR11                  GPIO_BRR_BR11_Msk
#define GPIO_BRR_BR12_Pos              (12U)
#define GPIO_BRR_BR12_Msk              (0x1UL << GPIO_BRR_BR12_Pos)             /*!< 0x00001000 */
#define GPIO_BRR_BR12                  GPIO_BRR_BR12_Msk
#define GPIO_BRR_BR13_Pos              (13U)
#define GPIO_BRR_BR13_Msk              (0x1UL << GPIO_BRR_BR13_Pos)             /*!< 0x00002000 */
#define GPIO_BRR_BR13                  GPIO_BRR_BR13_Msk
#define GPIO_BRR_BR14_Pos              (14U)
#define GPIO_BRR_BR14_Msk              (0x1UL << GPIO_BRR_BR14_Pos)             /*!< 0x00004000 */
#define GPIO_BRR_BR14                  GPIO_BRR_BR14_Msk
#define GPIO_BRR_BR15_Pos              (15U)
#define GPIO_BRR_BR15_Msk              (0x1UL << GPIO_BRR_BR15_Pos)             /*!< 0x00008000 */
#define GPIO_BRR_BR15                  GPIO_BRR_BR15_Msk


/******************************************************************************/
/*                                                                            */
/*                      Inter-integrated Circuit Interface (I2C)              */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for I2C_CR1 register  *******************/
#define I2C_CR1_PE_Pos               (0U)
#define I2C_CR1_PE_Msk               (0x1UL << I2C_CR1_PE_Pos)                 /*!< 0x00000001 */
#define I2C_CR1_PE                   I2C_CR1_PE_Msk                            /*!< Peripheral enable */
#define I2C_CR1_TXIE_Pos             (1U)
#define I2C_CR1_TXIE_Msk             (0x1UL << I2C_CR1_TXIE_Pos)               /*!< 0x00000002 */
#define I2C_CR1_TXIE                 I2C_CR1_TXIE_Msk                          /*!< TX interrupt enable */
#define I2C_CR1_RXIE_Pos             (2U)
#define I2C_CR1_RXIE_Msk             (0x1UL << I2C_CR1_RXIE_Pos)               /*!< 0x00000004 */
#define I2C_CR1_RXIE                 I2C_CR1_RXIE_Msk                          /*!< RX interrupt enable */
#define I2C_CR1_ADDRIE_Pos           (3U)
#define I2C_CR1_ADDRIE_Msk           (0x1UL << I2C_CR1_ADDRIE_Pos)             /*!< 0x00000008 */
#define I2C_CR1_ADDRIE               I2C_CR1_ADDRIE_Msk                        /*!< Address match interrupt enable */
#define I2C_CR1_NACKIE_Pos           (4U)
#define I2C_CR1_NACKIE_Msk           (0x1UL << I2C_CR1_NACKIE_Pos)             /*!< 0x00000010 */
#define I2C_CR1_NACKIE               I2C_CR1_NACKIE_Msk                        /*!< NACK received interrupt enable */
#define I2C_CR1_STOPIE_Pos           (5U)
#define I2C_CR1_STOPIE_Msk           (0x1UL << I2C_CR1_STOPIE_Pos)             /*!< 0x00000020 */
#define I2C_CR1_STOPIE               I2C_CR1_STOPIE_Msk                        /*!< STOP detection interrupt enable */
#define I2C_CR1_TCIE_Pos             (6U)
#define I2C_CR1_TCIE_Msk             (0x1UL << I2C_CR1_TCIE_Pos)               /*!< 0x00000040 */
#define I2C_CR1_TCIE                 I2C_CR1_TCIE_Msk                          /*!< Transfer complete interrupt enable */
#define I2C_CR1_ERRIE_Pos            (7U)
#define I2C_CR1_ERRIE_Msk            (0x1UL << I2C_CR1_ERRIE_Pos)              /*!< 0x00000080 */
#define I2C_CR1_ERRIE                I2C_CR1_ERRIE_Msk                         /*!< Errors interrupt enable */
#define I2C_CR1_DNF_Pos              (8U)
#define I2C_CR1_DNF_Msk              (0xFUL << I2C_CR1_DNF_Pos)                /*!< 0x00000F00 */
#define I2C_CR1_DNF                  I2C_CR1_DNF_Msk                           /*!< Digital noise filter */
#define I2C_CR1_ANFOFF_Pos           (12U)
#define I2C_CR1_ANFOFF_Msk           (0x1UL << I2C_CR1_ANFOFF_Pos)             /*!< 0x00001000 */
#define I2C_CR1_ANFOFF               I2C_CR1_ANFOFF_Msk                        /*!< Analog noise filter OFF */
#define I2C_CR1_SWRST_Pos            (13U)
#define I2C_CR1_SWRST_Msk            (0x1UL << I2C_CR1_SWRST_Pos)              /*!< 0x00002000 */
#define I2C_CR1_SWRST                I2C_CR1_SWRST_Msk                         /*!< Software reset */
#define I2C_CR1_TXDMAEN_Pos          (14U)
#define I2C_CR1_TXDMAEN_Msk          (0x1UL << I2C_CR1_TXDMAEN_Pos)            /*!< 0x00004000 */
#define I2C_CR1_TXDMAEN              I2C_CR1_TXDMAEN_Msk                       /*!< DMA transmission requests enable */
#define I2C_CR1_RXDMAEN_Pos          (15U)
#define I2C_CR1_RXDMAEN_Msk          (0x1UL << I2C_CR1_RXDMAEN_Pos)            /*!< 0x00008000 */
#define I2C_CR1_RXDMAEN              I2C_CR1_RXDMAEN_Msk                       /*!< DMA reception requests enable */
#define I2C_CR1_SBC_Pos              (16U)
#define I2C_CR1_SBC_Msk              (0x1UL << I2C_CR1_SBC_Pos)                /*!< 0x00010000 */
#define I2C_CR1_SBC                  I2C_CR1_SBC_Msk                           /*!< Slave byte control */
#define I2C_CR1_NOSTRETCH_Pos        (17U)
#define I2C_CR1_NOSTRETCH_Msk        (0x1UL << I2C_CR1_NOSTRETCH_Pos)          /*!< 0x00020000 */
#define I2C_CR1_NOSTRETCH            I2C_CR1_NOSTRETCH_Msk                     /*!< Clock stretching disable */
#define I2C_CR1_WUPEN_Pos            (18U)
#define I2C_CR1_WUPEN_Msk            (0x1UL << I2C_CR1_WUPEN_Pos)              /*!< 0x00040000 */
#define I2C_CR1_WUPEN                I2C_CR1_WUPEN_Msk                         /*!< Wakeup from STOP enable */
#define I2C_CR1_GCEN_Pos             (19U)
#define I2C_CR1_GCEN_Msk             (0x1UL << I2C_CR1_GCEN_Pos)               /*!< 0x00080000 */
#define I2C_CR1_GCEN                 I2C_CR1_GCEN_Msk                          /*!< General call enable */
#define I2C_CR1_SMBHEN_Pos           (20U)
#define I2C_CR1_SMBHEN_Msk           (0x1UL << I2C_CR1_SMBHEN_Pos)             /*!< 0x00100000 */
#define I2C_CR1_SMBHEN               I2C_CR1_SMBHEN_Msk                        /*!< SMBus host address enable */
#define I2C_CR1_SMBDEN_Pos           (21U)
#define I2C_CR1_SMBDEN_Msk           (0x1UL << I2C_CR1_SMBDEN_Pos)             /*!< 0x00200000 */
#define I2C_CR1_SMBDEN               I2C_CR1_SMBDEN_Msk                        /*!< SMBus device default address enable */
#define I2C_CR1_ALERTEN_Pos          (22U)
#define I2C_CR1_ALERTEN_Msk          (0x1UL << I2C_CR1_ALERTEN_Pos)            /*!< 0x00400000 */
#define I2C_CR1_ALERTEN              I2C_CR1_ALERTEN_Msk                       /*!< SMBus alert enable */
#define I2C_CR1_PECEN_Pos            (23U)
#define I2C_CR1_PECEN_Msk            (0x1UL << I2C_CR1_PECEN_Pos)              /*!< 0x00800000 */
#define I2C_CR1_PECEN                I2C_CR1_PECEN_Msk                         /*!< PEC enable */

/******************  Bit definition for I2C_CR2 register  ********************/
#define I2C_CR2_SADD_Pos             (0U)
#define I2C_CR2_SADD_Msk             (0x3FFUL << I2C_CR2_SADD_Pos)             /*!< 0x000003FF */
#define I2C_CR2_SADD                 I2C_CR2_SADD_Msk                          /*!< Slave address (master mode) */
#define I2C_CR2_RD_WRN_Pos           (10U)
#define I2C_CR2_RD_WRN_Msk           (0x1UL << I2C_CR2_RD_WRN_Pos)             /*!< 0x00000400 */
#define I2C_CR2_RD_WRN               I2C_CR2_RD_WRN_Msk                        /*!< Transfer direction (master mode) */
#define I2C_CR2_ADD10_Pos            (11U)
#define I2C_CR2_ADD10_Msk            (0x1UL << I2C_CR2_ADD10_Pos)              /*!< 0x00000800 */
#define I2C_CR2_ADD10                I2C_CR2_ADD10_Msk                         /*!< 10-bit addressing mode (master mode) */
#define I2C_CR2_HEAD10R_Pos          (12U)
#define I2C_CR2_HEAD10R_Msk          (0x1UL << I2C_CR2_HEAD10R_Pos)            /*!< 0x00001000 */
#define I2C_CR2_HEAD10R              I2C_CR2_HEAD10R_Msk                       /*!< 10-bit address header only read direction (master mode) */
#define I2C_CR2_START_Pos            (13U)
#define I2C_CR2_START_Msk            (0x1UL << I2C_CR2_START_Pos)              /*!< 0x00002000 */
#define I2C_CR2_START                I2C_CR2_START_Msk                         /*!< START generation */
#define I2C_CR2_STOP_Pos             (14U)
#define I2C_CR2_STOP_Msk             (0x1UL << I2C_CR2_STOP_Pos)               /*!< 0x00004000 */
#define I2C_CR2_STOP                 I2C_CR2_STOP_Msk                          /*!< STOP generation (master mode) */
#define I2C_CR2_NACK_Pos             (15U)
#define I2C_CR2_NACK_Msk             (0x1UL << I2C_CR2_NACK_Pos)               /*!< 0x00008000 */
#define I2C_CR2_NACK                 I2C_CR2_NACK_Msk                          /*!< NACK generation (slave mode) */
#define I2C_CR2_NBYTES_Pos           (16U)
#define I2C_CR2_NBYTES_Msk           (0xFFUL << I2C_CR2_NBYTES_Pos)            /*!< 0x00FF0000 */
#define I2C_CR2_NBYTES               I2C_CR2_NBYTES_Msk                        /*!< Number of bytes */
#define I2C_CR2_RELOAD_Pos           (24U)
#define I2C_CR2_RELOAD_Msk           (0x1UL << I2C_CR2_RELOAD_Pos)             /*!< 0x01000000 */
#define I2C_CR2_RELOAD               I2C_CR2_RELOAD_Msk                        /*!< NBYTES reload mode */
#define I2C_CR2_AUTOEND_Pos          (25U)
#define I2C_CR2_AUTOEND_Msk          (0x1UL << I2C_CR2_AUTOEND_Pos)            /*!< 0x02000000 */
#define I2C_CR2_AUTOEND              I2C_CR2_AUTOEND_Msk                       /*!< Automatic end mode (master mode) */
#define I2C_CR2_PECBYTE_Pos          (26U)
#define I2C_CR2_PECBYTE_Msk          (0x1UL << I2C_CR2_PECBYTE_Pos)            /*!< 0x04000000 */
#define I2C_CR2_PECBYTE              I2C_CR2_PECBYTE_Msk                       /*!< Packet error checking byte */

/*******************  Bit definition for I2C_OAR1 register  ******************/
#define I2C_OAR1_OA1_Pos             (0U)
#define I2C_OAR1_OA1_Msk             (0x3FFUL << I2C_OAR1_OA1_Pos)             /*!< 0x000003FF */
#define I2C_OAR1_OA1                 I2C_OAR1_OA1_Msk                          /*!< Interface own address 1 */
#define I2C_OAR1_OA1MODE_Pos         (10U)
#define I2C_OAR1_OA1MODE_Msk         (0x1UL << I2C_OAR1_OA1MODE_Pos)           /*!< 0x00000400 */
#define I2C_OAR1_OA1MODE             I2C_OAR1_OA1MODE_Msk                      /*!< Own address 1 10-bit mode */
#define I2C_OAR1_OA1EN_Pos           (15U)
#define I2C_OAR1_OA1EN_Msk           (0x1UL << I2C_OAR1_OA1EN_Pos)             /*!< 0x00008000 */
#define I2C_OAR1_OA1EN               I2C_OAR1_OA1EN_Msk                        /*!< Own address 1 enable */

/*******************  Bit definition for I2C_OAR2 register  ******************/
#define I2C_OAR2_OA2_Pos             (1U)
#define I2C_OAR2_OA2_Msk             (0x7FUL << I2C_OAR2_OA2_Pos)              /*!< 0x000000FE */
#define I2C_OAR2_OA2                 I2C_OAR2_OA2_Msk                          /*!< Interface own address 2 */
#define I2C_OAR2_OA2MSK_Pos          (8U)
#define I2C_OAR2_OA2MSK_Msk          (0x7UL << I2C_OAR2_OA2MSK_Pos)            /*!< 0x00000700 */
#define I2C_OAR2_OA2MSK              I2C_OAR2_OA2MSK_Msk                       /*!< Own address 2 masks */
#define I2C_OAR2_OA2NOMASK           (0U)                                      /*!< No mask                                        */
#define I2C_OAR2_OA2MASK01_Pos       (8U)
#define I2C_OAR2_OA2MASK01_Msk       (0x1UL << I2C_OAR2_OA2MASK01_Pos)         /*!< 0x00000100 */
#define I2C_OAR2_OA2MASK01           I2C_OAR2_OA2MASK01_Msk                    /*!< OA2[1] is masked, Only OA2[7:2] are compared   */
#define I2C_OAR2_OA2MASK02_Pos       (9U)
#define I2C_OAR2_OA2MASK02_Msk       (0x1UL << I2C_OAR2_OA2MASK02_Pos)         /*!< 0x00000200 */
#define I2C_OAR2_OA2MASK02           I2C_OAR2_OA2MASK02_Msk                    /*!< OA2[2:1] is masked, Only OA2[7:3] are compared */
#define I2C_OAR2_OA2MASK03_Pos       (8U)
#define I2C_OAR2_OA2MASK03_Msk       (0x3UL << I2C_OAR2_OA2MASK03_Pos)         /*!< 0x00000300 */
#define I2C_OAR2_OA2MASK03           I2C_OAR2_OA2MASK03_Msk                    /*!< OA2[3:1] is masked, Only OA2[7:4] are compared */
#define I2C_OAR2_OA2MASK04_Pos       (10U)
#define I2C_OAR2_OA2MASK04_Msk       (0x1UL << I2C_OAR2_OA2MASK04_Pos)         /*!< 0x00000400 */
#define I2C_OAR2_OA2MASK04           I2C_OAR2_OA2MASK04_Msk                    /*!< OA2[4:1] is masked, Only OA2[7:5] are compared */
#define I2C_OAR2_OA2MASK05_Pos       (8U)
#define I2C_OAR2_OA2MASK05_Msk       (0x5UL << I2C_OAR2_OA2MASK05_Pos)         /*!< 0x00000500 */
#define I2C_OAR2_OA2MASK05           I2C_OAR2_OA2MASK05_Msk                    /*!< OA2[5:1] is masked, Only OA2[7:6] are compared */
#define I2C_OAR2_OA2MASK06_Pos       (9U)
#define I2C_OAR2_OA2MASK06_Msk       (0x3UL << I2C_OAR2_OA2MASK06_Pos)         /*!< 0x00000600 */
#define I2C_OAR2_OA2MASK06           I2C_OAR2_OA2MASK06_Msk                    /*!< OA2[6:1] is masked, Only OA2[7] are compared   */
#define I2C_OAR2_OA2MASK07_Pos       (8U)
#define I2C_OAR2_OA2MASK07_Msk       (0x7UL << I2C_OAR2_OA2MASK07_Pos)         /*!< 0x00000700 */
#define I2C_OAR2_OA2MASK07           I2C_OAR2_OA2MASK07_Msk                    /*!< OA2[7:1] is masked, No comparison is done      */
#define I2C_OAR2_OA2EN_Pos           (15U)
#define I2C_OAR2_OA2EN_Msk           (0x1UL << I2C_OAR2_OA2EN_Pos)             /*!< 0x00008000 */
#define I2C_OAR2_OA2EN               I2C_OAR2_OA2EN_Msk                        /*!< Own address 2 enable */

/*******************  Bit definition for I2C_TIMINGR register *******************/
#define I2C_TIMINGR_SCLL_Pos         (0U)
#define I2C_TIMINGR_SCLL_Msk         (0xFFUL << I2C_TIMINGR_SCLL_Pos)          /*!< 0x000000FF */
#define I2C_TIMINGR_SCLL             I2C_TIMINGR_SCLL_Msk                      /*!< SCL low period (master mode) */
#define I2C_TIMINGR_SCLH_Pos         (8U)
#define I2C_TIMINGR_SCLH_Msk         (0xFFUL << I2C_TIMINGR_SCLH_Pos)          /*!< 0x0000FF00 */
#define I2C_TIMINGR_SCLH             I2C_TIMINGR_SCLH_Msk                      /*!< SCL high period (master mode) */
#define I2C_TIMINGR_SDADEL_Pos       (16U)
#define I2C_TIMINGR_SDADEL_Msk       (0xFUL << I2C_TIMINGR_SDADEL_Pos)         /*!< 0x000F0000 */
#define I2C_TIMINGR_SDADEL           I2C_TIMINGR_SDADEL_Msk                    /*!< Data hold time */
#define I2C_TIMINGR_SCLDEL_Pos       (20U)
#define I2C_TIMINGR_SCLDEL_Msk       (0xFUL << I2C_TIMINGR_SCLDEL_Pos)         /*!< 0x00F00000 */
#define I2C_TIMINGR_SCLDEL           I2C_TIMINGR_SCLDEL_Msk                    /*!< Data setup time */
#define I2C_TIMINGR_PRESC_Pos        (28U)
#define I2C_TIMINGR_PRESC_Msk        (0xFUL << I2C_TIMINGR_PRESC_Pos)          /*!< 0xF0000000 */
#define I2C_TIMINGR_PRESC            I2C_TIMINGR_PRESC_Msk                     /*!< Timings prescaler */

/******************* Bit definition for I2C_TIMEOUTR register *******************/
#define I2C_TIMEOUTR_TIMEOUTA_Pos    (0U)
#define I2C_TIMEOUTR_TIMEOUTA_Msk    (0xFFFUL << I2C_TIMEOUTR_TIMEOUTA_Pos)    /*!< 0x00000FFF */
#define I2C_TIMEOUTR_TIMEOUTA        I2C_TIMEOUTR_TIMEOUTA_Msk                 /*!< Bus timeout A */
#define I2C_TIMEOUTR_TIDLE_Pos       (12U)
#define I2C_TIMEOUTR_TIDLE_Msk       (0x1UL << I2C_TIMEOUTR_TIDLE_Pos)         /*!< 0x00001000 */
#define I2C_TIMEOUTR_TIDLE           I2C_TIMEOUTR_TIDLE_Msk                    /*!< Idle clock timeout detection */
#define I2C_TIMEOUTR_TIMOUTEN_Pos    (15U)
#define I2C_TIMEOUTR_TIMOUTEN_Msk    (0x1UL << I2C_TIMEOUTR_TIMOUTEN_Pos)      /*!< 0x00008000 */
#define I2C_TIMEOUTR_TIMOUTEN        I2C_TIMEOUTR_TIMOUTEN_Msk                 /*!< Clock timeout enable */
#define I2C_TIMEOUTR_TIMEOUTB_Pos    (16U)
#define I2C_TIMEOUTR_TIMEOUTB_Msk    (0xFFFUL << I2C_TIMEOUTR_TIMEOUTB_Pos)    /*!< 0x0FFF0000 */
#define I2C_TIMEOUTR_TIMEOUTB        I2C_TIMEOUTR_TIMEOUTB_Msk                 /*!< Bus timeout B*/
#define I2C_TIMEOUTR_TEXTEN_Pos      (31U)
#define I2C_TIMEOUTR_TEXTEN_Msk      (0x1UL << I2C_TIMEOUTR_TEXTEN_Pos)        /*!< 0x80000000 */
#define I2C_TIMEOUTR_TEXTEN          I2C_TIMEOUTR_TEXTEN_Msk                   /*!< Extended clock timeout enable */

/******************  Bit definition for I2C_ISR register  *********************/
#define I2C_ISR_TXE_Pos              (0U)
#define I2C_ISR_TXE_Msk              (0x1UL << I2C_ISR_TXE_Pos)                /*!< 0x00000001 */
#define I2C_ISR_TXE                  I2C_ISR_TXE_Msk                           /*!< Transmit data register empty */
#define I2C_ISR_TXIS_Pos             (1U)
#define I2C_ISR_TXIS_Msk             (0x1UL << I2C_ISR_TXIS_Pos)               /*!< 0x00000002 */
#define I2C_ISR_TXIS                 I2C_ISR_TXIS_Msk                          /*!< Transmit interrupt status */
#define I2C_ISR_RXNE_Pos             (2U)
#define I2C_ISR_RXNE_Msk             (0x1UL << I2C_ISR_RXNE_Pos)               /*!< 0x00000004 */
#define I2C_ISR_RXNE                 I2C_ISR_RXNE_Msk                          /*!< Receive data register not empty */
#define I2C_ISR_ADDR_Pos             (3U)
#define I2C_ISR_ADDR_Msk             (0x1UL << I2C_ISR_ADDR_Pos)               /*!< 0x00000008 */
#define I2C_ISR_ADDR                 I2C_ISR_ADDR_Msk                          /*!< Address matched (slave mode)*/
#define I2C_ISR_NACKF_Pos            (4U)
#define I2C_ISR_NACKF_Msk            (0x1UL << I2C_ISR_NACKF_Pos)              /*!< 0x00000010 */
#define I2C_ISR_NACKF                I2C_ISR_NACKF_Msk                         /*!< NACK received flag */
#define I2C_ISR_STOPF_Pos            (5U)
#define I2C_ISR_STOPF_Msk            (0x1UL << I2C_ISR_STOPF_Pos)              /*!< 0x00000020 */
#define I2C_ISR_STOPF                I2C_ISR_STOPF_Msk                         /*!< STOP detection flag */
#define I2C_ISR_TC_Pos               (6U)
#define I2C_ISR_TC_Msk               (0x1UL << I2C_ISR_TC_Pos)                 /*!< 0x00000040 */
#define I2C_ISR_TC                   I2C_ISR_TC_Msk                            /*!< Transfer complete (master mode) */
#define I2C_ISR_TCR_Pos              (7U)
#define I2C_ISR_TCR_Msk              (0x1UL << I2C_ISR_TCR_Pos)                /*!< 0x00000080 */
#define I2C_ISR_TCR                  I2C_ISR_TCR_Msk                           /*!< Transfer complete reload */
#define I2C_ISR_BERR_Pos             (8U)
#define I2C_ISR_BERR_Msk             (0x1UL << I2C_ISR_BERR_Pos)               /*!< 0x00000100 */
#define I2C_ISR_BERR                 I2C_ISR_BERR_Msk                          /*!< Bus error */
#define I2C_ISR_ARLO_Pos             (9U)
#define I2C_ISR_ARLO_Msk             (0x1UL << I2C_ISR_ARLO_Pos)               /*!< 0x00000200 */
#define I2C_ISR_ARLO                 I2C_ISR_ARLO_Msk                          /*!< Arbitration lost */
#define I2C_ISR_OVR_Pos              (10U)
#define I2C_ISR_OVR_Msk              (0x1UL << I2C_ISR_OVR_Pos)                /*!< 0x00000400 */
#define I2C_ISR_OVR                  I2C_ISR_OVR_Msk                           /*!< Overrun/Underrun */
#define I2C_ISR_PECERR_Pos           (11U)
#define I2C_ISR_PECERR_Msk           (0x1UL << I2C_ISR_PECERR_Pos)             /*!< 0x00000800 */
#define I2C_ISR_PECERR               I2C_ISR_PECERR_Msk                        /*!< PEC error in reception */
#define I2C_ISR_TIMEOUT_Pos          (12U)
#define I2C_ISR_TIMEOUT_Msk          (0x1UL << I2C_ISR_TIMEOUT_Pos)            /*!< 0x00001000 */
#define I2C_ISR_TIMEOUT              I2C_ISR_TIMEOUT_Msk                       /*!< Timeout or Tlow detection flag */
#define I2C_ISR_ALERT_Pos            (13U)
#define I2C_ISR_ALERT_Msk            (0x1UL << I2C_ISR_ALERT_Pos)              /*!< 0x00002000 */
#define I2C_ISR_ALERT                I2C_ISR_ALERT_Msk                         /*!< SMBus alert */
#define I2C_ISR_BUSY_Pos             (15U)
#define I2C_ISR_BUSY_Msk             (0x1UL << I2C_ISR_BUSY_Pos)               /*!< 0x00008000 */
#define I2C_ISR_BUSY                 I2C_ISR_BUSY_Msk                          /*!< Bus busy */
#define I2C_ISR_DIR_Pos              (16U)
#define I2C_ISR_DIR_Msk              (0x1UL << I2C_ISR_DIR_Pos)                /*!< 0x00010000 */
#define I2C_ISR_DIR                  I2C_ISR_DIR_Msk                           /*!< Transfer direction (slave mode) */
#define I2C_ISR_ADDCODE_Pos          (17U)
#define I2C_ISR_ADDCODE_Msk          (0x7FUL << I2C_ISR_ADDCODE_Pos)           /*!< 0x00FE0000 */
#define I2C_ISR_ADDCODE              I2C_ISR_ADDCODE_Msk                       /*!< Address match code (slave mode) */

/******************  Bit definition for I2C_ICR register  *********************/
#define I2C_ICR_ADDRCF_Pos           (3U)
#define I2C_ICR_ADDRCF_Msk           (0x1UL << I2C_ICR_ADDRCF_Pos)             /*!< 0x00000008 */
#define I2C_ICR_ADDRCF               I2C_ICR_ADDRCF_Msk                        /*!< Address matched clear flag */
#define I2C_ICR_NACKCF_Pos           (4U)
#define I2C_ICR_NACKCF_Msk           (0x1UL << I2C_ICR_NACKCF_Pos)             /*!< 0x00000010 */
#define I2C_ICR_NACKCF               I2C_ICR_NACKCF_Msk                        /*!< NACK clear flag */
#define I2C_ICR_STOPCF_Pos           (5U)
#define I2C_ICR_STOPCF_Msk           (0x1UL << I2C_ICR_STOPCF_Pos)             /*!< 0x00000020 */
#define I2C_ICR_STOPCF               I2C_ICR_STOPCF_Msk                        /*!< STOP detection clear flag */
#define I2C_ICR_BERRCF_Pos           (8U)
#define I2C_ICR_BERRCF_Msk           (0x1UL << I2C_ICR_BERRCF_Pos)             /*!< 0x00000100 */
#define I2C_ICR_BERRCF               I2C_ICR_BERRCF_Msk                        /*!< Bus error clear flag */
#define I2C_ICR_ARLOCF_Pos           (9U)
#define I2C_ICR_ARLOCF_Msk           (0x1UL << I2C_ICR_ARLOCF_Pos)             /*!< 0x00000200 */
#define I2C_ICR_ARLOCF               I2C_ICR_ARLOCF_Msk                        /*!< Arbitration lost clear flag */
#define I2C_ICR_OVRCF_Pos            (10U)
#define I2C_ICR_OVRCF_Msk            (0x1UL << I2C_ICR_OVRCF_Pos)              /*!< 0x00000400 */
#define I2C_ICR_OVRCF                I2C_ICR_OVRCF_Msk                         /*!< Overrun/Underrun clear flag */
#define I2C_ICR_PECCF_Pos            (11U)
#define I2C_ICR_PECCF_Msk            (0x1UL << I2C_ICR_PECCF_Pos)              /*!< 0x00000800 */
#define I2C_ICR_PECCF                I2C_ICR_PECCF_Msk                         /*!< PAC error clear flag */
#define I2C_ICR_TIMOUTCF_Pos         (12U)
#define I2C_ICR_TIMOUTCF_Msk         (0x1UL << I2C_ICR_TIMOUTCF_Pos)           /*!< 0x00001000 */
#define I2C_ICR_TIMOUTCF             I2C_ICR_TIMOUTCF_Msk                      /*!< Timeout clear flag */
#define I2C_ICR_ALERTCF_Pos          (13U)
#define I2C_ICR_ALERTCF_Msk          (0x1UL << I2C_ICR_ALERTCF_Pos)            /*!< 0x00002000 */
#define I2C_ICR_ALERTCF              I2C_ICR_ALERTCF_Msk                       /*!< Alert clear flag */

/******************  Bit definition for I2C_PECR register  *********************/
#define I2C_PECR_PEC_Pos             (0U)
#define I2C_PECR_PEC_Msk             (0xFFUL << I2C_PECR_PEC_Pos)              /*!< 0x000000FF */
#define I2C_PECR_PEC                 I2C_PECR_PEC_Msk                          /*!< PEC register */

/******************  Bit definition for I2C_RXDR register  *********************/
#define I2C_RXDR_RXDATA_Pos          (0U)
#define I2C_RXDR_RXDATA_Msk          (0xFFUL << I2C_RXDR_RXDATA_Pos)           /*!< 0x000000FF */
#define I2C_RXDR_RXDATA              I2C_RXDR_RXDATA_Msk                       /*!< 8-bit receive data */

/******************  Bit definition for I2C_TXDR register  *********************/
#define I2C_TXDR_TXDATA_Pos          (0U)
#define I2C_TXDR_TXDATA_Msk          (0xFFUL << I2C_TXDR_TXDATA_Pos)           /*!< 0x000000FF */
#define I2C_TXDR_TXDATA              I2C_TXDR_TXDATA_Msk                       /*!< 8-bit transmit data */


/******************************************************************************/
/*                                                                            */
/*                        Independent WATCHDOG (IWDG)                         */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_KR_KEY_Pos      (0U)
#define IWDG_KR_KEY_Msk      (0xFFFFUL << IWDG_KR_KEY_Pos)                     /*!< 0x0000FFFF */
#define IWDG_KR_KEY          IWDG_KR_KEY_Msk                                   /*!<Key value (write only, read 0000h)  */

/*******************  Bit definition for IWDG_PR register  ********************/
#define IWDG_PR_PR_Pos       (0U)
#define IWDG_PR_PR_Msk       (0x7UL << IWDG_PR_PR_Pos)                         /*!< 0x00000007 */
#define IWDG_PR_PR           IWDG_PR_PR_Msk                                    /*!<PR[2:0] (Prescaler divider)         */
#define IWDG_PR_PR_0         (0x1UL << IWDG_PR_PR_Pos)                         /*!< 0x00000001 */
#define IWDG_PR_PR_1         (0x2UL << IWDG_PR_PR_Pos)                         /*!< 0x00000002 */
#define IWDG_PR_PR_2         (0x4UL << IWDG_PR_PR_Pos)                         /*!< 0x00000004 */

/*******************  Bit definition for IWDG_RLR register  *******************/
#define IWDG_RLR_RL_Pos      (0U)
#define IWDG_RLR_RL_Msk      (0xFFFUL << IWDG_RLR_RL_Pos)                      /*!< 0x00000FFF */
#define IWDG_RLR_RL          IWDG_RLR_RL_Msk                                   /*!<Watchdog counter reload value        */

/*******************  Bit definition for IWDG_SR register  ********************/
#define IWDG_SR_PVU_Pos      (0U)
#define IWDG_SR_PVU_Msk      (0x1UL << IWDG_SR_PVU_Pos)                        /*!< 0x00000001 */
#define IWDG_SR_PVU          IWDG_SR_PVU_Msk                                   /*!< Watchdog prescaler value update */
#define IWDG_SR_RVU_Pos      (1U)
#define IWDG_SR_RVU_Msk      (0x1UL << IWDG_SR_RVU_Pos)                        /*!< 0x00000002 */
#define IWDG_SR_RVU          IWDG_SR_RVU_Msk                                   /*!< Watchdog counter reload value update */
#define IWDG_SR_WVU_Pos      (2U)
#define IWDG_SR_WVU_Msk      (0x1UL << IWDG_SR_WVU_Pos)                        /*!< 0x00000004 */
#define IWDG_SR_WVU          IWDG_SR_WVU_Msk                                   /*!< Watchdog counter window value update */

/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_WINR_WIN_Pos    (0U)
#define IWDG_WINR_WIN_Msk    (0xFFFUL << IWDG_WINR_WIN_Pos)                    /*!< 0x00000FFF */
#define IWDG_WINR_WIN        IWDG_WINR_WIN_Msk                                 /*!< Watchdog counter window value */


/******************************************************************************/
/*                                                                            */
/*                        Power Control                                       */
/*                                                                            */
/******************************************************************************/
#define PWR_PVD_SUPPORT                       /*!< PWR feature available only on specific devices: Power Voltage Detection feature */
#define PWR_BOR_SUPPORT                       /*!< PWR feature available only on specific devices: Brown-Out Reset feature         */
#define PWR_SHDW_SUPPORT                      /*!< PWR feature available only on specific devices: Shutdown mode */

/********************  Bit definition for PWR_CR1 register  ********************/
#define PWR_CR1_LPMS_Pos          (0U)
#define PWR_CR1_LPMS_Msk          (0x7UL << PWR_CR1_LPMS_Pos)                  /*!< 0x00000007 */
#define PWR_CR1_LPMS              PWR_CR1_LPMS_Msk                             /*!< Low Power Mode Selection */
#define PWR_CR1_LPMS_0            (0x1UL << PWR_CR1_LPMS_Pos)                  /*!< 0x00000001 */
#define PWR_CR1_LPMS_1            (0x2UL << PWR_CR1_LPMS_Pos)                  /*!< 0x00000002 */
#define PWR_CR1_LPMS_2            (0x4UL << PWR_CR1_LPMS_Pos)                   /*!< 0x00000004 */
#define PWR_CR1_FPD_STOP_Pos      (3U)
#define PWR_CR1_FPD_STOP_Msk      (0x1UL << PWR_CR1_FPD_STOP_Pos)              /*!< 0x00000008 */
#define PWR_CR1_FPD_STOP          PWR_CR1_FPD_STOP_Msk                         /*!< Flash power down mode during stop */
#define PWR_CR1_FPD_LPRUN_Pos     (4U)
#define PWR_CR1_FPD_LPRUN_Msk     (0x1UL << PWR_CR1_FPD_LPRUN_Pos)             /*!< 0x00000010 */
#define PWR_CR1_FPD_LPRUN         PWR_CR1_FPD_LPRUN_Msk                        /*!< Flash power down mode during run */
#define PWR_CR1_FPD_LPSLP_Pos     (5U)
#define PWR_CR1_FPD_LPSLP_Msk     (0x1UL << PWR_CR1_FPD_LPSLP_Pos)             /*!< 0x00000020 */
#define PWR_CR1_FPD_LPSLP         PWR_CR1_FPD_LPSLP_Msk                        /*!< Flash power down mode during sleep */
#define PWR_CR1_DBP_Pos           (8U)
#define PWR_CR1_DBP_Msk           (0x1UL << PWR_CR1_DBP_Pos)                   /*!< 0x00000100 */
#define PWR_CR1_DBP               PWR_CR1_DBP_Msk                              /*!< Disable Backup Domain write protection */
#define PWR_CR1_VOS_Pos           (9U)
#define PWR_CR1_VOS_Msk           (0x3UL << PWR_CR1_VOS_Pos)                   /*!< 0x00000600 */
#define PWR_CR1_VOS               PWR_CR1_VOS_Msk                              /*!< Voltage scaling */
#define PWR_CR1_VOS_0             (0x1UL << PWR_CR1_VOS_Pos)                   /*!< Voltage scaling bit 0 */
#define PWR_CR1_VOS_1             (0x2UL << PWR_CR1_VOS_Pos)                   /*!< Voltage scaling bit 1 */
#define PWR_CR1_LPR_Pos           (14U)
#define PWR_CR1_LPR_Msk           (0x1UL << PWR_CR1_LPR_Pos)                   /*!< 0x00004000 */
#define PWR_CR1_LPR               PWR_CR1_LPR_Msk                              /*!< Regulator Low-Power Run mode */

/********************  Bit definition for PWR_CR2 register  ********************/
#define PWR_CR2_PVDE_Pos          (0U)
#define PWR_CR2_PVDE_Msk          (0x1UL << PWR_CR2_PVDE_Pos)                  /*!< 0x00000001 */
#define PWR_CR2_PVDE              PWR_CR2_PVDE_Msk                             /*!< Programmable Voltage Detector Enable */
#define PWR_CR2_PVDFT_Pos         (1U)
#define PWR_CR2_PVDFT_Msk         (0x7UL << PWR_CR2_PVDFT_Pos)                 /*!< 0x0000000E */
#define PWR_CR2_PVDFT             PWR_CR2_PVDFT_Msk                            /*!< PVD Falling Threshold Selection bit field */
#define PWR_CR2_PVDFT_0           (0x1UL << PWR_CR2_PVDFT_Pos)                 /*!< 0x00000002 */
#define PWR_CR2_PVDFT_1           (0x2UL << PWR_CR2_PVDFT_Pos)                 /*!< 0x00000004 */
#define PWR_CR2_PVDFT_2           (0x4UL << PWR_CR2_PVDFT_Pos)                 /*!< 0x00000008 */
#define PWR_CR2_PVDRT_Pos         (4U)
#define PWR_CR2_PVDRT_Msk         (0x7UL << PWR_CR2_PVDRT_Pos)                 /*!< 0x00000070 */
#define PWR_CR2_PVDRT             PWR_CR2_PVDRT_Msk                            /*!< PVD Rising Threshold Selection bit field */
#define PWR_CR2_PVDRT_0           (0x1UL << PWR_CR2_PVDRT_Pos)                 /*!< 0x00000010 */
#define PWR_CR2_PVDRT_1           (0x2UL << PWR_CR2_PVDRT_Pos)                 /*!< 0x00000020 */
#define PWR_CR2_PVDRT_2           (0x4UL << PWR_CR2_PVDRT_Pos)                 /*!< 0x00000040 */

/********************  Bit definition for PWR_CR3 register  ********************/
#define PWR_CR3_EWUP_Pos          (0U)
#define PWR_CR3_EWUP_Msk          (0x3BUL << PWR_CR3_EWUP_Pos)                 /*!< 0x0000003B */
#define PWR_CR3_EWUP              PWR_CR3_EWUP_Msk                             /*!< Enable all Wake-Up Pins  */
#define PWR_CR3_EWUP1_Pos         (0U)
#define PWR_CR3_EWUP1_Msk         (0x1UL << PWR_CR3_EWUP1_Pos)                 /*!< 0x00000001 */
#define PWR_CR3_EWUP1             PWR_CR3_EWUP1_Msk                            /*!< Enable WKUP pin 1 */
#define PWR_CR3_EWUP2_Pos         (1U)
#define PWR_CR3_EWUP2_Msk         (0x1UL << PWR_CR3_EWUP2_Pos)                 /*!< 0x00000002 */
#define PWR_CR3_EWUP2             PWR_CR3_EWUP2_Msk                            /*!< Enable WKUP pin 2 */
#define PWR_CR3_EWUP4_Pos         (3U)
#define PWR_CR3_EWUP4_Msk         (0x1UL << PWR_CR3_EWUP4_Pos)                 /*!< 0x00000008 */
#define PWR_CR3_EWUP4             PWR_CR3_EWUP4_Msk                            /*!< Enable WKUP pin 4 */
#define PWR_CR3_EWUP5_Pos         (4U)
#define PWR_CR3_EWUP5_Msk         (0x1UL << PWR_CR3_EWUP5_Pos)                 /*!< 0x00000010 */
#define PWR_CR3_EWUP5             PWR_CR3_EWUP5_Msk                            /*!< Enable WKUP pin 5 */
#define PWR_CR3_EWUP6_Pos         (5U)
#define PWR_CR3_EWUP6_Msk         (0x1UL << PWR_CR3_EWUP6_Pos)                 /*!< 0x00000020 */
#define PWR_CR3_EWUP6             PWR_CR3_EWUP6_Msk                            /*!< Enable WKUP pin 6 */
#define PWR_CR3_RRS_Pos           (8U)
#define PWR_CR3_RRS_Msk           (0x1UL << PWR_CR3_RRS_Pos)                   /*!< 0x00000100 */
#define PWR_CR3_RRS               PWR_CR3_RRS_Msk                              /*!< RAM retention in Standby mode */
#define PWR_CR3_ENB_ULP_Pos       (9U)
#define PWR_CR3_ENB_ULP_Msk       (0x1UL << PWR_CR3_ENB_ULP_Pos)               /*!< 0x00000200 */
#define PWR_CR3_ENB_ULP           PWR_CR3_ENB_ULP_Msk                          /*!< Enable sampling resistor bridge in the LPMU_RESET block */
#define PWR_CR3_APC_Pos           (10U)
#define PWR_CR3_APC_Msk           (0x1UL << PWR_CR3_APC_Pos)                   /*!< 0x00000400 */
#define PWR_CR3_APC               PWR_CR3_APC_Msk                              /*!< Apply pull-up and pull-down configuration */
#define PWR_CR3_EIWUL_Pos         (15U)
#define PWR_CR3_EIWUL_Msk         (0x1UL << PWR_CR3_EIWUL_Pos)                 /*!< 0x00008000 */
#define PWR_CR3_EIWUL             PWR_CR3_EIWUL_Msk                            /*!< Enable Internal Wake-up line */

/********************  Bit definition for PWR_CR4 register  ********************/
#define PWR_CR4_WP_Pos            (0U)
#define PWR_CR4_WP_Msk            (0x3BUL << PWR_CR4_WP_Pos)                   /*!< 0x0000003B */
#define PWR_CR4_WP                PWR_CR4_WP_Msk                               /*!< all Wake-Up Pin polarity */
#define PWR_CR4_WP1_Pos           (0U)
#define PWR_CR4_WP1_Msk           (0x1UL << PWR_CR4_WP1_Pos)                   /*!< 0x00000001 */
#define PWR_CR4_WP1               PWR_CR4_WP1_Msk                              /*!< Wake-Up Pin 1 polarity */
#define PWR_CR4_WP2_Pos           (1U)
#define PWR_CR4_WP2_Msk           (0x1UL << PWR_CR4_WP2_Pos)                   /*!< 0x00000002 */
#define PWR_CR4_WP2               PWR_CR4_WP2_Msk                              /*!< Wake-Up Pin 2 polarity */
#define PWR_CR4_WP4_Pos           (3U)
#define PWR_CR4_WP4_Msk           (0x1UL << PWR_CR4_WP4_Pos)                   /*!< 0x00000008 */
#define PWR_CR4_WP4               PWR_CR4_WP4_Msk                              /*!< Wake-Up Pin 4 polarity */
#define PWR_CR4_WP5_Pos           (4U)
#define PWR_CR4_WP5_Msk           (0x1UL << PWR_CR4_WP5_Pos)                   /*!< 0x00000010 */
#define PWR_CR4_WP5               PWR_CR4_WP5_Msk                              /*!< Wake-Up Pin 5 polarity */
#define PWR_CR4_WP6_Pos           (5U)
#define PWR_CR4_WP6_Msk           (0x1UL << PWR_CR4_WP6_Pos)                   /*!< 0x00000020 */
#define PWR_CR4_WP6               PWR_CR4_WP6_Msk                              /*!< Wake-Up Pin 6 polarity */
#define PWR_CR4_VBE_Pos           (8U)
#define PWR_CR4_VBE_Msk           (0x1UL << PWR_CR4_VBE_Pos)                   /*!< 0x00000100 */
#define PWR_CR4_VBE               PWR_CR4_VBE_Msk                              /*!< VBAT Battery charging Enable  */
#define PWR_CR4_VBRS_Pos          (9U)
#define PWR_CR4_VBRS_Msk          (0x1UL << PWR_CR4_VBRS_Pos)                  /*!< 0x00000200 */
#define PWR_CR4_VBRS              PWR_CR4_VBRS_Msk                             /*!< VBAT Battery charging Resistor Selection */

/********************  Bit definition for PWR_SR1 register  ********************/
#define PWR_SR1_WUF_Pos           (0U)
#define PWR_SR1_WUF_Msk           (0x3BUL << PWR_SR1_WUF_Pos)                  /*!< 0x0000003B */
#define PWR_SR1_WUF               PWR_SR1_WUF_Msk                              /*!< Wakeup Flags  */
#define PWR_SR1_WUF1_Pos          (0U)
#define PWR_SR1_WUF1_Msk          (0x1UL << PWR_SR1_WUF1_Pos)                  /*!< 0x00000001 */
#define PWR_SR1_WUF1              PWR_SR1_WUF1_Msk                             /*!< Wakeup Flag 1 */
#define PWR_SR1_WUF2_Pos          (1U)
#define PWR_SR1_WUF2_Msk          (0x1UL << PWR_SR1_WUF2_Pos)                  /*!< 0x00000002 */
#define PWR_SR1_WUF2              PWR_SR1_WUF2_Msk                             /*!< Wakeup Flag 2 */
#define PWR_SR1_WUF4_Pos          (3U)
#define PWR_SR1_WUF4_Msk          (0x1UL << PWR_SR1_WUF4_Pos)                  /*!< 0x00000008 */
#define PWR_SR1_WUF4              PWR_SR1_WUF4_Msk                             /*!< Wakeup Flag 4 */
#define PWR_SR1_WUF5_Pos          (4U)
#define PWR_SR1_WUF5_Msk          (0x1UL << PWR_SR1_WUF5_Pos)                  /*!< 0x00000010 */
#define PWR_SR1_WUF5              PWR_SR1_WUF5_Msk                             /*!< Wakeup Flag 5 */
#define PWR_SR1_WUF6_Pos          (5U)
#define PWR_SR1_WUF6_Msk          (0x1UL << PWR_SR1_WUF6_Pos)                  /*!< 0x00000020 */
#define PWR_SR1_WUF6              PWR_SR1_WUF6_Msk                             /*!< Wakeup Flag 6 */
#define PWR_SR1_SBF_Pos           (8U)
#define PWR_SR1_SBF_Msk           (0x1UL << PWR_SR1_SBF_Pos)                   /*!< 0x00000100 */
#define PWR_SR1_SBF               PWR_SR1_SBF_Msk                              /*!< Standby Flag  */
#define PWR_SR1_WUFI_Pos          (15U)
#define PWR_SR1_WUFI_Msk          (0x1UL << PWR_SR1_WUFI_Pos)                  /*!< 0x00008000 */
#define PWR_SR1_WUFI              PWR_SR1_WUFI_Msk                             /*!< Wakeup Flag Internal */

/********************  Bit definition for PWR_SR2 register  ********************/
#define PWR_SR2_FLASH_RDY_Pos     (7U)
#define PWR_SR2_FLASH_RDY_Msk     (0x1UL << PWR_SR2_FLASH_RDY_Pos)             /*!< 0x00000080 */
#define PWR_SR2_FLASH_RDY         PWR_SR2_FLASH_RDY_Msk                        /*!< Flash Ready */
#define PWR_SR2_REGLPS_Pos        (8U)
#define PWR_SR2_REGLPS_Msk        (0x1UL << PWR_SR2_REGLPS_Pos)                /*!< 0x00000100 */
#define PWR_SR2_REGLPS            PWR_SR2_REGLPS_Msk                           /*!< Regulator Low Power started */
#define PWR_SR2_REGLPF_Pos        (9U)
#define PWR_SR2_REGLPF_Msk        (0x1UL << PWR_SR2_REGLPF_Pos)                /*!< 0x00000200 */
#define PWR_SR2_REGLPF            PWR_SR2_REGLPF_Msk                           /*!< Regulator Low Power flag    */
#define PWR_SR2_VOSF_Pos          (10U)
#define PWR_SR2_VOSF_Msk          (0x1UL << PWR_SR2_VOSF_Pos)                  /*!< 0x00000400 */
#define PWR_SR2_VOSF              PWR_SR2_VOSF_Msk                             /*!< Voltage Scaling Flag */
#define PWR_SR2_PVDO_Pos          (11U)
#define PWR_SR2_PVDO_Msk          (0x1UL << PWR_SR2_PVDO_Pos)                  /*!< 0x00000800 */
#define PWR_SR2_PVDO              PWR_SR2_PVDO_Msk                             /*!< Power voltage detector output */

/********************  Bit definition for PWR_SCR register  ********************/
#define PWR_SCR_CWUF_Pos          (0U)
#define PWR_SCR_CWUF_Msk          (0x3BUL << PWR_SCR_CWUF_Pos)                 /*!< 0x0000003B */
#define PWR_SCR_CWUF              PWR_SCR_CWUF_Msk                             /*!< Clear Wake-up Flags  */
#define PWR_SCR_CWUF1_Pos         (0U)
#define PWR_SCR_CWUF1_Msk         (0x1UL << PWR_SCR_CWUF1_Pos)                 /*!< 0x00000001 */
#define PWR_SCR_CWUF1             PWR_SCR_CWUF1_Msk                            /*!< Clear Wake-up Flag 1 */
#define PWR_SCR_CWUF2_Pos         (1U)
#define PWR_SCR_CWUF2_Msk         (0x1UL << PWR_SCR_CWUF2_Pos)                 /*!< 0x00000002 */
#define PWR_SCR_CWUF2             PWR_SCR_CWUF2_Msk                            /*!< Clear Wake-up Flag 2 */
#define PWR_SCR_CWUF4_Pos         (3U)
#define PWR_SCR_CWUF4_Msk         (0x1UL << PWR_SCR_CWUF4_Pos)                 /*!< 0x00000008 */
#define PWR_SCR_CWUF4             PWR_SCR_CWUF4_Msk                            /*!< Clear Wake-up Flag 4 */
#define PWR_SCR_CWUF5_Pos         (4U)
#define PWR_SCR_CWUF5_Msk         (0x1UL << PWR_SCR_CWUF5_Pos)                 /*!< 0x00000010 */
#define PWR_SCR_CWUF5             PWR_SCR_CWUF5_Msk                            /*!< Clear Wake-up Flag 5 */
#define PWR_SCR_CWUF6_Pos         (5U)
#define PWR_SCR_CWUF6_Msk         (0x1UL << PWR_SCR_CWUF6_Pos)                 /*!< 0x00000020 */
#define PWR_SCR_CWUF6             PWR_SCR_CWUF6_Msk                            /*!< Clear Wake-up Flag 6 */
#define PWR_SCR_CSBF_Pos          (8U)
#define PWR_SCR_CSBF_Msk          (0x1UL << PWR_SCR_CSBF_Pos)                  /*!< 0x00000100 */
#define PWR_SCR_CSBF              PWR_SCR_CSBF_Msk                             /*!< Clear Standby Flag  */

/********************  Bit definition for PWR_PUCRA register  *****************/
#define PWR_PUCRA_PU0_Pos         (0U)
#define PWR_PUCRA_PU0_Msk         (0x1UL << PWR_PUCRA_PU0_Pos)                 /*!< 0x00000001 */
#define PWR_PUCRA_PU0             PWR_PUCRA_PU0_Msk                            /*!< Pin PA0 Pull-Up set */
#define PWR_PUCRA_PU1_Pos         (1U)
#define PWR_PUCRA_PU1_Msk         (0x1UL << PWR_PUCRA_PU1_Pos)                 /*!< 0x00000002 */
#define PWR_PUCRA_PU1             PWR_PUCRA_PU1_Msk                            /*!< Pin PA1 Pull-Up set */
#define PWR_PUCRA_PU2_Pos         (2U)
#define PWR_PUCRA_PU2_Msk         (0x1UL << PWR_PUCRA_PU2_Pos)                 /*!< 0x00000004 */
#define PWR_PUCRA_PU2             PWR_PUCRA_PU2_Msk                            /*!< Pin PA2 Pull-Up set */
#define PWR_PUCRA_PU3_Pos         (3U)
#define PWR_PUCRA_PU3_Msk         (0x1UL << PWR_PUCRA_PU3_Pos)                 /*!< 0x00000008 */
#define PWR_PUCRA_PU3             PWR_PUCRA_PU3_Msk                            /*!< Pin PA3 Pull-Up set */
#define PWR_PUCRA_PU4_Pos         (4U)
#define PWR_PUCRA_PU4_Msk         (0x1UL << PWR_PUCRA_PU4_Pos)                 /*!< 0x00000010 */
#define PWR_PUCRA_PU4             PWR_PUCRA_PU4_Msk                            /*!< Pin PA4 Pull-Up set */
#define PWR_PUCRA_PU5_Pos         (5U)
#define PWR_PUCRA_PU5_Msk         (0x1UL << PWR_PUCRA_PU5_Pos)                 /*!< 0x00000020 */
#define PWR_PUCRA_PU5             PWR_PUCRA_PU5_Msk                            /*!< Pin PA5 Pull-Up set */
#define PWR_PUCRA_PU6_Pos         (6U)
#define PWR_PUCRA_PU6_Msk         (0x1UL << PWR_PUCRA_PU6_Pos)                 /*!< 0x00000040 */
#define PWR_PUCRA_PU6             PWR_PUCRA_PU6_Msk                            /*!< Pin PA6 Pull-Up set */
#define PWR_PUCRA_PU7_Pos         (7U)
#define PWR_PUCRA_PU7_Msk         (0x1UL << PWR_PUCRA_PU7_Pos)                 /*!< 0x00000080 */
#define PWR_PUCRA_PU7             PWR_PUCRA_PU7_Msk                            /*!< Pin PA7 Pull-Up set */
#define PWR_PUCRA_PU8_Pos         (8U)
#define PWR_PUCRA_PU8_Msk         (0x1UL << PWR_PUCRA_PU8_Pos)                 /*!< 0x00000100 */
#define PWR_PUCRA_PU8             PWR_PUCRA_PU8_Msk                            /*!< Pin PA8 Pull-Up set */
#define PWR_PUCRA_PU9_Pos         (9U)
#define PWR_PUCRA_PU9_Msk         (0x1UL << PWR_PUCRA_PU9_Pos)                 /*!< 0x00000200 */
#define PWR_PUCRA_PU9             PWR_PUCRA_PU9_Msk                            /*!< Pin PA9 Pull-Up set */
#define PWR_PUCRA_PU10_Pos        (10U)
#define PWR_PUCRA_PU10_Msk        (0x1UL << PWR_PUCRA_PU10_Pos)                /*!< 0x00000400 */
#define PWR_PUCRA_PU10            PWR_PUCRA_PU10_Msk                           /*!< Pin PA10 Pull-Up set */
#define PWR_PUCRA_PU11_Pos        (11U)
#define PWR_PUCRA_PU11_Msk        (0x1UL << PWR_PUCRA_PU11_Pos)                /*!< 0x00000800 */
#define PWR_PUCRA_PU11            PWR_PUCRA_PU11_Msk                           /*!< Pin PA11 Pull-Up set */
#define PWR_PUCRA_PU12_Pos        (12U)
#define PWR_PUCRA_PU12_Msk        (0x1UL << PWR_PUCRA_PU12_Pos)                /*!< 0x00001000 */
#define PWR_PUCRA_PU12            PWR_PUCRA_PU12_Msk                           /*!< Pin PA12 Pull-Up set */
#define PWR_PUCRA_PU13_Pos        (13U)
#define PWR_PUCRA_PU13_Msk        (0x1UL << PWR_PUCRA_PU13_Pos)                /*!< 0x00002000 */
#define PWR_PUCRA_PU13            PWR_PUCRA_PU13_Msk                           /*!< Pin PA13 Pull-Up set */
#define PWR_PUCRA_PU14_Pos        (14U)
#define PWR_PUCRA_PU14_Msk        (0x1UL << PWR_PUCRA_PU14_Pos)                /*!< 0x00004000 */
#define PWR_PUCRA_PU14            PWR_PUCRA_PU14_Msk                           /*!< Pin PA14 Pull-Up set */
#define PWR_PUCRA_PU15_Pos        (15U)
#define PWR_PUCRA_PU15_Msk        (0x1UL << PWR_PUCRA_PU15_Pos)                /*!< 0x00008000 */
#define PWR_PUCRA_PU15            PWR_PUCRA_PU15_Msk                           /*!< Pin PA15 Pull-Up set */

/********************  Bit definition for PWR_PDCRA register  *****************/
#define PWR_PDCRA_PD0_Pos         (0U)
#define PWR_PDCRA_PD0_Msk         (0x1UL << PWR_PDCRA_PD0_Pos)                 /*!< 0x00000001 */
#define PWR_PDCRA_PD0             PWR_PDCRA_PD0_Msk                            /*!< Pin PA0 Pull-Down set */
#define PWR_PDCRA_PD1_Pos         (1U)
#define PWR_PDCRA_PD1_Msk         (0x1UL << PWR_PDCRA_PD1_Pos)                 /*!< 0x00000002 */
#define PWR_PDCRA_PD1             PWR_PDCRA_PD1_Msk                            /*!< Pin PA1 Pull-Down set */
#define PWR_PDCRA_PD2_Pos         (2U)
#define PWR_PDCRA_PD2_Msk         (0x1UL << PWR_PDCRA_PD2_Pos)                 /*!< 0x00000004 */
#define PWR_PDCRA_PD2             PWR_PDCRA_PD2_Msk                            /*!< Pin PA2 Pull-Down set */
#define PWR_PDCRA_PD3_Pos         (3U)
#define PWR_PDCRA_PD3_Msk         (0x1UL << PWR_PDCRA_PD3_Pos)                 /*!< 0x00000008 */
#define PWR_PDCRA_PD3             PWR_PDCRA_PD3_Msk                            /*!< Pin PA3 Pull-Down set */
#define PWR_PDCRA_PD4_Pos         (4U)
#define PWR_PDCRA_PD4_Msk         (0x1UL << PWR_PDCRA_PD4_Pos)                 /*!< 0x00000010 */
#define PWR_PDCRA_PD4             PWR_PDCRA_PD4_Msk                            /*!< Pin PA4 Pull-Down set */
#define PWR_PDCRA_PD5_Pos         (5U)
#define PWR_PDCRA_PD5_Msk         (0x1UL << PWR_PDCRA_PD5_Pos)                 /*!< 0x00000020 */
#define PWR_PDCRA_PD5             PWR_PDCRA_PD5_Msk                            /*!< Pin PA5 Pull-Down set */
#define PWR_PDCRA_PD6_Pos         (6U)
#define PWR_PDCRA_PD6_Msk         (0x1UL << PWR_PDCRA_PD6_Pos)                 /*!< 0x00000040 */
#define PWR_PDCRA_PD6             PWR_PDCRA_PD6_Msk                            /*!< Pin PA6 Pull-Down set */
#define PWR_PDCRA_PD7_Pos         (7U)
#define PWR_PDCRA_PD7_Msk         (0x1UL << PWR_PDCRA_PD7_Pos)                 /*!< 0x00000080 */
#define PWR_PDCRA_PD7             PWR_PDCRA_PD7_Msk                            /*!< Pin PA7 Pull-Down set */
#define PWR_PDCRA_PD8_Pos         (8U)
#define PWR_PDCRA_PD8_Msk         (0x1UL << PWR_PDCRA_PD8_Pos)                 /*!< 0x00000100 */
#define PWR_PDCRA_PD8             PWR_PDCRA_PD8_Msk                            /*!< Pin PA8 Pull-Down set */
#define PWR_PDCRA_PD9_Pos         (9U)
#define PWR_PDCRA_PD9_Msk         (0x1UL << PWR_PDCRA_PD9_Pos)                 /*!< 0x00000200 */
#define PWR_PDCRA_PD9             PWR_PDCRA_PD9_Msk                            /*!< Pin PA9 Pull-Down set */
#define PWR_PDCRA_PD10_Pos        (10U)
#define PWR_PDCRA_PD10_Msk        (0x1UL << PWR_PDCRA_PD10_Pos)                /*!< 0x00000400 */
#define PWR_PDCRA_PD10            PWR_PDCRA_PD10_Msk                           /*!< Pin PA10 Pull-Down set */
#define PWR_PDCRA_PD11_Pos        (11U)
#define PWR_PDCRA_PD11_Msk        (0x1UL << PWR_PDCRA_PD11_Pos)                /*!< 0x00000800 */
#define PWR_PDCRA_PD11            PWR_PDCRA_PD11_Msk                           /*!< Pin PA11 Pull-Down set */
#define PWR_PDCRA_PD12_Pos        (12U)
#define PWR_PDCRA_PD12_Msk        (0x1UL << PWR_PDCRA_PD12_Pos)                /*!< 0x00001000 */
#define PWR_PDCRA_PD12            PWR_PDCRA_PD12_Msk                           /*!< Pin PA12 Pull-Down set */
#define PWR_PDCRA_PD13_Pos        (13U)
#define PWR_PDCRA_PD13_Msk        (0x1UL << PWR_PDCRA_PD13_Pos)                /*!< 0x00002000 */
#define PWR_PDCRA_PD13            PWR_PDCRA_PD13_Msk                           /*!< Pin PA13 Pull-Down set */
#define PWR_PDCRA_PD14_Pos        (14U)
#define PWR_PDCRA_PD14_Msk        (0x1UL << PWR_PDCRA_PD14_Pos)                /*!< 0x00004000 */
#define PWR_PDCRA_PD14            PWR_PDCRA_PD14_Msk                           /*!< Pin PA14 Pull-Down set */
#define PWR_PDCRA_PD15_Pos        (15U)
#define PWR_PDCRA_PD15_Msk        (0x1UL << PWR_PDCRA_PD15_Pos)                /*!< 0x00008000 */
#define PWR_PDCRA_PD15            PWR_PDCRA_PD15_Msk                           /*!< Pin PA15 Pull-Down set */

/********************  Bit definition for PWR_PUCRB register  *****************/
#define PWR_PUCRB_PU0_Pos         (0U)
#define PWR_PUCRB_PU0_Msk         (0x1UL << PWR_PUCRB_PU0_Pos)                 /*!< 0x00000001 */
#define PWR_PUCRB_PU0             PWR_PUCRB_PU0_Msk                            /*!< Pin PB0 Pull-Up set */
#define PWR_PUCRB_PU1_Pos         (1U)
#define PWR_PUCRB_PU1_Msk         (0x1UL << PWR_PUCRB_PU1_Pos)                 /*!< 0x00000002 */
#define PWR_PUCRB_PU1             PWR_PUCRB_PU1_Msk                            /*!< Pin PB1 Pull-Up set */
#define PWR_PUCRB_PU2_Pos         (2U)
#define PWR_PUCRB_PU2_Msk         (0x1UL << PWR_PUCRB_PU2_Pos)                 /*!< 0x00000004 */
#define PWR_PUCRB_PU2             PWR_PUCRB_PU2_Msk                            /*!< Pin PB2 Pull-Up set */
#define PWR_PUCRB_PU3_Pos         (3U)
#define PWR_PUCRB_PU3_Msk         (0x1UL << PWR_PUCRB_PU3_Pos)                 /*!< 0x00000008 */
#define PWR_PUCRB_PU3             PWR_PUCRB_PU3_Msk                            /*!< Pin PB3 Pull-Up set */
#define PWR_PUCRB_PU4_Pos         (4U)
#define PWR_PUCRB_PU4_Msk         (0x1UL << PWR_PUCRB_PU4_Pos)                 /*!< 0x00000010 */
#define PWR_PUCRB_PU4             PWR_PUCRB_PU4_Msk                            /*!< Pin PB4 Pull-Up set */
#define PWR_PUCRB_PU5_Pos         (5U)
#define PWR_PUCRB_PU5_Msk         (0x1UL << PWR_PUCRB_PU5_Pos)                 /*!< 0x00000020 */
#define PWR_PUCRB_PU5             PWR_PUCRB_PU5_Msk                            /*!< Pin PB5 Pull-Up set */
#define PWR_PUCRB_PU6_Pos         (6U)
#define PWR_PUCRB_PU6_Msk         (0x1UL << PWR_PUCRB_PU6_Pos)                 /*!< 0x00000040 */
#define PWR_PUCRB_PU6             PWR_PUCRB_PU6_Msk                            /*!< Pin PB6 Pull-Up set */
#define PWR_PUCRB_PU7_Pos         (7U)
#define PWR_PUCRB_PU7_Msk         (0x1UL << PWR_PUCRB_PU7_Pos)                 /*!< 0x00000080 */
#define PWR_PUCRB_PU7             PWR_PUCRB_PU7_Msk                            /*!< Pin PB7 Pull-Up set */
#define PWR_PUCRB_PU8_Pos         (8U)
#define PWR_PUCRB_PU8_Msk         (0x1UL << PWR_PUCRB_PU8_Pos)                 /*!< 0x00000100 */
#define PWR_PUCRB_PU8             PWR_PUCRB_PU8_Msk                            /*!< Pin PB8 Pull-Up set */
#define PWR_PUCRB_PU9_Pos         (9U)
#define PWR_PUCRB_PU9_Msk         (0x1UL << PWR_PUCRB_PU9_Pos)                 /*!< 0x00000200 */
#define PWR_PUCRB_PU9             PWR_PUCRB_PU9_Msk                            /*!< Pin PB9 Pull-Up set */
#define PWR_PUCRB_PU10_Pos        (10U)
#define PWR_PUCRB_PU10_Msk        (0x1UL << PWR_PUCRB_PU10_Pos)                /*!< 0x00000400 */
#define PWR_PUCRB_PU10            PWR_PUCRB_PU10_Msk                           /*!< Pin PB10 Pull-Up set */
#define PWR_PUCRB_PU11_Pos        (11U)
#define PWR_PUCRB_PU11_Msk        (0x1UL << PWR_PUCRB_PU11_Pos)                /*!< 0x00000800 */
#define PWR_PUCRB_PU11            PWR_PUCRB_PU11_Msk                           /*!< Pin PB11 Pull-Up set */
#define PWR_PUCRB_PU12_Pos        (12U)
#define PWR_PUCRB_PU12_Msk        (0x1UL << PWR_PUCRB_PU12_Pos)                /*!< 0x00001000 */
#define PWR_PUCRB_PU12            PWR_PUCRB_PU12_Msk                           /*!< Pin PB12 Pull-Up set */
#define PWR_PUCRB_PU13_Pos        (13U)
#define PWR_PUCRB_PU13_Msk        (0x1UL << PWR_PUCRB_PU13_Pos)                /*!< 0x00002000 */
#define PWR_PUCRB_PU13            PWR_PUCRB_PU13_Msk                           /*!< Pin PB13 Pull-Up set */
#define PWR_PUCRB_PU14_Pos        (14U)
#define PWR_PUCRB_PU14_Msk        (0x1UL << PWR_PUCRB_PU14_Pos)                /*!< 0x00004000 */
#define PWR_PUCRB_PU14            PWR_PUCRB_PU14_Msk                           /*!< Pin PB14 Pull-Up set */
#define PWR_PUCRB_PU15_Pos        (15U)
#define PWR_PUCRB_PU15_Msk        (0x1UL << PWR_PUCRB_PU15_Pos)                /*!< 0x00008000 */
#define PWR_PUCRB_PU15            PWR_PUCRB_PU15_Msk                           /*!< Pin PB15 Pull-Up set */

/********************  Bit definition for PWR_PDCRB register  *****************/
#define PWR_PDCRB_PD0_Pos         (0U)
#define PWR_PDCRB_PD0_Msk         (0x1UL << PWR_PDCRB_PD0_Pos)                 /*!< 0x00000001 */
#define PWR_PDCRB_PD0             PWR_PDCRB_PD0_Msk                            /*!< Pin PB0 Pull-Down set */
#define PWR_PDCRB_PD1_Pos         (1U)
#define PWR_PDCRB_PD1_Msk         (0x1UL << PWR_PDCRB_PD1_Pos)                 /*!< 0x00000002 */
#define PWR_PDCRB_PD1             PWR_PDCRB_PD1_Msk                            /*!< Pin PB1 Pull-Down set */
#define PWR_PDCRB_PD2_Pos         (2U)
#define PWR_PDCRB_PD2_Msk         (0x1UL << PWR_PDCRB_PD2_Pos)                 /*!< 0x00000004 */
#define PWR_PDCRB_PD2             PWR_PDCRB_PD2_Msk                            /*!< Pin PB2 Pull-Down set */
#define PWR_PDCRB_PD3_Pos         (3U)
#define PWR_PDCRB_PD3_Msk         (0x1UL << PWR_PDCRB_PD3_Pos)                 /*!< 0x00000008 */
#define PWR_PDCRB_PD3             PWR_PDCRB_PD3_Msk                            /*!< Pin PB3 Pull-Down set */
#define PWR_PDCRB_PD4_Pos         (4U)
#define PWR_PDCRB_PD4_Msk         (0x1UL << PWR_PDCRB_PD4_Pos)                 /*!< 0x00000010 */
#define PWR_PDCRB_PD4             PWR_PDCRB_PD4_Msk                            /*!< Pin PB4 Pull-Down set */
#define PWR_PDCRB_PD5_Pos         (5U)
#define PWR_PDCRB_PD5_Msk         (0x1UL << PWR_PDCRB_PD5_Pos)                 /*!< 0x00000020 */
#define PWR_PDCRB_PD5             PWR_PDCRB_PD5_Msk                            /*!< Pin PB5 Pull-Down set */
#define PWR_PDCRB_PD6_Pos         (6U)
#define PWR_PDCRB_PD6_Msk         (0x1UL << PWR_PDCRB_PD6_Pos)                 /*!< 0x00000040 */
#define PWR_PDCRB_PD6             PWR_PDCRB_PD6_Msk                            /*!< Pin PB6 Pull-Down set */
#define PWR_PDCRB_PD7_Pos         (7U)
#define PWR_PDCRB_PD7_Msk         (0x1UL << PWR_PDCRB_PD7_Pos)                 /*!< 0x00000080 */
#define PWR_PDCRB_PD7             PWR_PDCRB_PD7_Msk                            /*!< Pin PB7 Pull-Down set */
#define PWR_PDCRB_PD8_Pos         (8U)
#define PWR_PDCRB_PD8_Msk         (0x1UL << PWR_PDCRB_PD8_Pos)                 /*!< 0x00000100 */
#define PWR_PDCRB_PD8             PWR_PDCRB_PD8_Msk                            /*!< Pin PB8 Pull-Down set */
#define PWR_PDCRB_PD9_Pos         (9U)
#define PWR_PDCRB_PD9_Msk         (0x1UL << PWR_PDCRB_PD9_Pos)                 /*!< 0x00000200 */
#define PWR_PDCRB_PD9             PWR_PDCRB_PD9_Msk                            /*!< Pin PB9 Pull-Down set */
#define PWR_PDCRB_PD10_Pos        (10U)
#define PWR_PDCRB_PD10_Msk        (0x1UL << PWR_PDCRB_PD10_Pos)                /*!< 0x00000400 */
#define PWR_PDCRB_PD10            PWR_PDCRB_PD10_Msk                           /*!< Pin PB10 Pull-Down set */
#define PWR_PDCRB_PD11_Pos        (11U)
#define PWR_PDCRB_PD11_Msk        (0x1UL << PWR_PDCRB_PD11_Pos)                /*!< 0x00000800 */
#define PWR_PDCRB_PD11            PWR_PDCRB_PD11_Msk                           /*!< Pin PB11 Pull-Down set */
#define PWR_PDCRB_PD12_Pos        (12U)
#define PWR_PDCRB_PD12_Msk        (0x1UL << PWR_PDCRB_PD12_Pos)                /*!< 0x00001000 */
#define PWR_PDCRB_PD12            PWR_PDCRB_PD12_Msk                           /*!< Pin PB12 Pull-Down set */
#define PWR_PDCRB_PD13_Pos        (13U)
#define PWR_PDCRB_PD13_Msk        (0x1UL << PWR_PDCRB_PD13_Pos)                /*!< 0x00002000 */
#define PWR_PDCRB_PD13            PWR_PDCRB_PD13_Msk                           /*!< Pin PB13 Pull-Down set */
#define PWR_PDCRB_PD14_Pos        (14U)
#define PWR_PDCRB_PD14_Msk        (0x1UL << PWR_PDCRB_PD14_Pos)                /*!< 0x00004000 */
#define PWR_PDCRB_PD14            PWR_PDCRB_PD14_Msk                           /*!< Pin PB14 Pull-Down set */
#define PWR_PDCRB_PD15_Pos        (15U)
#define PWR_PDCRB_PD15_Msk        (0x1UL << PWR_PDCRB_PD15_Pos)                /*!< 0x00008000 */
#define PWR_PDCRB_PD15            PWR_PDCRB_PD15_Msk                           /*!< Pin PB15 Pull-Down set */

/********************  Bit definition for PWR_PUCRC register  *****************/
#define PWR_PUCRC_PU0_Pos         (0U)
#define PWR_PUCRC_PU0_Msk         (0x1UL << PWR_PUCRC_PU0_Pos)                 /*!< 0x00000001 */
#define PWR_PUCRC_PU0             PWR_PUCRC_PU0_Msk                            /*!< Pin PC0 Pull-Up set */
#define PWR_PUCRC_PU1_Pos         (1U)
#define PWR_PUCRC_PU1_Msk         (0x1UL << PWR_PUCRC_PU1_Pos)                 /*!< 0x00000002 */
#define PWR_PUCRC_PU1             PWR_PUCRC_PU1_Msk                            /*!< Pin PC1 Pull-Up set */
#define PWR_PUCRC_PU2_Pos         (2U)
#define PWR_PUCRC_PU2_Msk         (0x1UL << PWR_PUCRC_PU2_Pos)                 /*!< 0x00000004 */
#define PWR_PUCRC_PU2             PWR_PUCRC_PU2_Msk                            /*!< Pin PC2 Pull-Up set */
#define PWR_PUCRC_PU3_Pos         (3U)
#define PWR_PUCRC_PU3_Msk         (0x1UL << PWR_PUCRC_PU3_Pos)                 /*!< 0x00000008 */
#define PWR_PUCRC_PU3             PWR_PUCRC_PU3_Msk                            /*!< Pin PC3 Pull-Up set */
#define PWR_PUCRC_PU4_Pos         (4U)
#define PWR_PUCRC_PU4_Msk         (0x1UL << PWR_PUCRC_PU4_Pos)                 /*!< 0x00000010 */
#define PWR_PUCRC_PU4             PWR_PUCRC_PU4_Msk                            /*!< Pin PC4 Pull-Up set */
#define PWR_PUCRC_PU5_Pos         (5U)
#define PWR_PUCRC_PU5_Msk         (0x1UL << PWR_PUCRC_PU5_Pos)                 /*!< 0x00000020 */
#define PWR_PUCRC_PU5             PWR_PUCRC_PU5_Msk                            /*!< Pin PC5 Pull-Up set */
#define PWR_PUCRC_PU6_Pos         (6U)
#define PWR_PUCRC_PU6_Msk         (0x1UL << PWR_PUCRC_PU6_Pos)                 /*!< 0x00000040 */
#define PWR_PUCRC_PU6             PWR_PUCRC_PU6_Msk                            /*!< Pin PC6 Pull-Up set */
#define PWR_PUCRC_PU7_Pos         (7U)
#define PWR_PUCRC_PU7_Msk         (0x1UL << PWR_PUCRC_PU7_Pos)                 /*!< 0x00000080 */
#define PWR_PUCRC_PU7             PWR_PUCRC_PU7_Msk                            /*!< Pin PC7 Pull-Up set */
#define PWR_PUCRC_PU8_Pos         (8U)
#define PWR_PUCRC_PU8_Msk         (0x1UL << PWR_PUCRC_PU8_Pos)                 /*!< 0x00000100 */
#define PWR_PUCRC_PU8             PWR_PUCRC_PU8_Msk                            /*!< Pin PC8 Pull-Up set */
#define PWR_PUCRC_PU9_Pos         (9U)
#define PWR_PUCRC_PU9_Msk         (0x1UL << PWR_PUCRC_PU9_Pos)                 /*!< 0x00000200 */
#define PWR_PUCRC_PU9             PWR_PUCRC_PU9_Msk                            /*!< Pin PC9 Pull-Up set */
#define PWR_PUCRC_PU10_Pos        (10U)
#define PWR_PUCRC_PU10_Msk        (0x1UL << PWR_PUCRC_PU10_Pos)                /*!< 0x00000400 */
#define PWR_PUCRC_PU10            PWR_PUCRC_PU10_Msk                           /*!< Pin PC10 Pull-Up set */
#define PWR_PUCRC_PU11_Pos        (11U)
#define PWR_PUCRC_PU11_Msk        (0x1UL << PWR_PUCRC_PU11_Pos)                /*!< 0x00000800 */
#define PWR_PUCRC_PU11            PWR_PUCRC_PU11_Msk                           /*!< Pin PC11 Pull-Up set */
#define PWR_PUCRC_PU12_Pos        (12U)
#define PWR_PUCRC_PU12_Msk        (0x1UL << PWR_PUCRC_PU12_Pos)                /*!< 0x00001000 */
#define PWR_PUCRC_PU12            PWR_PUCRC_PU12_Msk                           /*!< Pin PC12 Pull-Up set */
#define PWR_PUCRC_PU13_Pos        (13U)
#define PWR_PUCRC_PU13_Msk        (0x1UL << PWR_PUCRC_PU13_Pos)                /*!< 0x00002000 */
#define PWR_PUCRC_PU13            PWR_PUCRC_PU13_Msk                           /*!< Pin PC13 Pull-Up set */
#define PWR_PUCRC_PU14_Pos        (14U)
#define PWR_PUCRC_PU14_Msk        (0x1UL << PWR_PUCRC_PU14_Pos)                /*!< 0x00004000 */
#define PWR_PUCRC_PU14            PWR_PUCRC_PU14_Msk                           /*!< Pin PC14 Pull-Up set */
#define PWR_PUCRC_PU15_Pos        (15U)
#define PWR_PUCRC_PU15_Msk        (0x1UL << PWR_PUCRC_PU15_Pos)                /*!< 0x00008000 */
#define PWR_PUCRC_PU15            PWR_PUCRC_PU15_Msk                           /*!< Pin PC15 Pull-Up set */

/********************  Bit definition for PWR_PDCRC register  *****************/
#define PWR_PDCRC_PD0_Pos         (0U)
#define PWR_PDCRC_PD0_Msk         (0x1UL << PWR_PDCRC_PD0_Pos)                 /*!< 0x00000001 */
#define PWR_PDCRC_PD0             PWR_PDCRC_PD0_Msk                            /*!< Pin PC0 Pull-Down set */
#define PWR_PDCRC_PD1_Pos         (1U)
#define PWR_PDCRC_PD1_Msk         (0x1UL << PWR_PDCRC_PD1_Pos)                 /*!< 0x00000002 */
#define PWR_PDCRC_PD1             PWR_PDCRC_PD1_Msk                            /*!< Pin PC1 Pull-Down set */
#define PWR_PDCRC_PD2_Pos         (2U)
#define PWR_PDCRC_PD2_Msk         (0x1UL << PWR_PDCRC_PD2_Pos)                 /*!< 0x00000004 */
#define PWR_PDCRC_PD2             PWR_PDCRC_PD2_Msk                            /*!< Pin PC2 Pull-Down set */
#define PWR_PDCRC_PD3_Pos         (3U)
#define PWR_PDCRC_PD3_Msk         (0x1UL << PWR_PDCRC_PD3_Pos)                 /*!< 0x00000008 */
#define PWR_PDCRC_PD3             PWR_PDCRC_PD3_Msk                            /*!< Pin PC3 Pull-Down set */
#define PWR_PDCRC_PD4_Pos         (4U)
#define PWR_PDCRC_PD4_Msk         (0x1UL << PWR_PDCRC_PD4_Pos)                 /*!< 0x00000010 */
#define PWR_PDCRC_PD4             PWR_PDCRC_PD4_Msk                            /*!< Pin PC4 Pull-Down set */
#define PWR_PDCRC_PD5_Pos         (5U)
#define PWR_PDCRC_PD5_Msk         (0x1UL << PWR_PDCRC_PD5_Pos)                 /*!< 0x00000020 */
#define PWR_PDCRC_PD5             PWR_PDCRC_PD5_Msk                            /*!< Pin PC5 Pull-Down set */
#define PWR_PDCRC_PD6_Pos         (6U)
#define PWR_PDCRC_PD6_Msk         (0x1UL << PWR_PDCRC_PD6_Pos)                 /*!< 0x00000040 */
#define PWR_PDCRC_PD6             PWR_PDCRC_PD6_Msk                            /*!< Pin PC6 Pull-Down set */
#define PWR_PDCRC_PD7_Pos         (7U)
#define PWR_PDCRC_PD7_Msk         (0x1UL << PWR_PDCRC_PD7_Pos)                 /*!< 0x00000080 */
#define PWR_PDCRC_PD7             PWR_PDCRC_PD7_Msk                            /*!< Pin PC7 Pull-Down set */
#define PWR_PDCRC_PD8_Pos         (8U)
#define PWR_PDCRC_PD8_Msk         (0x1UL << PWR_PDCRC_PD8_Pos)                 /*!< 0x00000100 */
#define PWR_PDCRC_PD8             PWR_PDCRC_PD8_Msk                            /*!< Pin PC8 Pull-Down set */
#define PWR_PDCRC_PD9_Pos         (9U)
#define PWR_PDCRC_PD9_Msk         (0x1UL << PWR_PDCRC_PD9_Pos)                 /*!< 0x00000200 */
#define PWR_PDCRC_PD9             PWR_PDCRC_PD9_Msk                            /*!< Pin PC9 Pull-Down set */
#define PWR_PDCRC_PD10_Pos        (10U)
#define PWR_PDCRC_PD10_Msk        (0x1UL << PWR_PDCRC_PD10_Pos)                /*!< 0x00000400 */
#define PWR_PDCRC_PD10            PWR_PDCRC_PD10_Msk                           /*!< Pin PC10 Pull-Down set */
#define PWR_PDCRC_PD11_Pos        (11U)
#define PWR_PDCRC_PD11_Msk        (0x1UL << PWR_PDCRC_PD11_Pos)                /*!< 0x00000800 */
#define PWR_PDCRC_PD11            PWR_PDCRC_PD11_Msk                           /*!< Pin PC11 Pull-Down set */
#define PWR_PDCRC_PD12_Pos        (12U)
#define PWR_PDCRC_PD12_Msk        (0x1UL << PWR_PDCRC_PD12_Pos)                /*!< 0x00001000 */
#define PWR_PDCRC_PD12            PWR_PDCRC_PD12_Msk                           /*!< Pin PC12 Pull-Down set */
#define PWR_PDCRC_PD13_Pos        (13U)
#define PWR_PDCRC_PD13_Msk        (0x1UL << PWR_PDCRC_PD13_Pos)                /*!< 0x00002000 */
#define PWR_PDCRC_PD13            PWR_PDCRC_PD13_Msk                           /*!< Pin PC13 Pull-Down set */
#define PWR_PDCRC_PD14_Pos        (14U)
#define PWR_PDCRC_PD14_Msk        (0x1UL << PWR_PDCRC_PD14_Pos)                /*!< 0x00004000 */
#define PWR_PDCRC_PD14            PWR_PDCRC_PD14_Msk                           /*!< Pin PC14 Pull-Down set */
#define PWR_PDCRC_PD15_Pos        (15U)
#define PWR_PDCRC_PD15_Msk        (0x1UL << PWR_PDCRC_PD15_Pos)                /*!< 0x00008000 */
#define PWR_PDCRC_PD15            PWR_PDCRC_PD15_Msk                           /*!< Pin PC15 Pull-Down set */

/********************  Bit definition for PWR_PUCRD register  *****************/
#define PWR_PUCRD_PU0_Pos         (0U)
#define PWR_PUCRD_PU0_Msk         (0x1UL << PWR_PUCRD_PU0_Pos)                 /*!< 0x00000001 */
#define PWR_PUCRD_PU0             PWR_PUCRD_PU0_Msk                            /*!< Pin PD0 Pull-Up set */
#define PWR_PUCRD_PU1_Pos         (1U)
#define PWR_PUCRD_PU1_Msk         (0x1UL << PWR_PUCRD_PU1_Pos)                 /*!< 0x00000002 */
#define PWR_PUCRD_PU1             PWR_PUCRD_PU1_Msk                            /*!< Pin PD1 Pull-Up set */
#define PWR_PUCRD_PU2_Pos         (2U)
#define PWR_PUCRD_PU2_Msk         (0x1UL << PWR_PUCRD_PU2_Pos)                 /*!< 0x00000004 */
#define PWR_PUCRD_PU2             PWR_PUCRD_PU2_Msk                            /*!< Pin PD2 Pull-Up set */
#define PWR_PUCRD_PU3_Pos         (3U)
#define PWR_PUCRD_PU3_Msk         (0x1UL << PWR_PUCRD_PU3_Pos)                 /*!< 0x00000008 */
#define PWR_PUCRD_PU3             PWR_PUCRD_PU3_Msk                            /*!< Pin PD3 Pull-Up set */
#define PWR_PUCRD_PU4_Pos         (4U)
#define PWR_PUCRD_PU4_Msk         (0x1UL << PWR_PUCRD_PU4_Pos)                 /*!< 0x00000010 */
#define PWR_PUCRD_PU4             PWR_PUCRD_PU4_Msk                            /*!< Pin PD4 Pull-Up set */
#define PWR_PUCRD_PU5_Pos         (5U)
#define PWR_PUCRD_PU5_Msk         (0x1UL << PWR_PUCRD_PU5_Pos)                 /*!< 0x00000020 */
#define PWR_PUCRD_PU5             PWR_PUCRD_PU5_Msk                            /*!< Pin PD5 Pull-Up set */
#define PWR_PUCRD_PU6_Pos         (6U)
#define PWR_PUCRD_PU6_Msk         (0x1UL << PWR_PUCRD_PU6_Pos)                 /*!< 0x00000040 */
#define PWR_PUCRD_PU6             PWR_PUCRD_PU6_Msk                            /*!< Pin PD6 Pull-Up set */
#define PWR_PUCRD_PU8_Pos         (8U)
#define PWR_PUCRD_PU8_Msk         (0x1UL << PWR_PUCRD_PU8_Pos)                 /*!< 0x00000100 */
#define PWR_PUCRD_PU8             PWR_PUCRD_PU8_Msk                            /*!< Pin PD8 Pull-Up set */
#define PWR_PUCRD_PU9_Pos         (9U)
#define PWR_PUCRD_PU9_Msk         (0x1UL << PWR_PUCRD_PU9_Pos)                 /*!< 0x00000200 */
#define PWR_PUCRD_PU9             PWR_PUCRD_PU9_Msk                            /*!< Pin PD9 Pull-Up set */

/********************  Bit definition for PWR_PDCRD register  *****************/
#define PWR_PDCRD_PD0_Pos         (0U)
#define PWR_PDCRD_PD0_Msk         (0x1UL << PWR_PDCRD_PD0_Pos)                 /*!< 0x00000001 */
#define PWR_PDCRD_PD0             PWR_PDCRD_PD0_Msk                            /*!< Pin PD0 Pull-Down set */
#define PWR_PDCRD_PD1_Pos         (1U)
#define PWR_PDCRD_PD1_Msk         (0x1UL << PWR_PDCRD_PD1_Pos)                 /*!< 0x00000002 */
#define PWR_PDCRD_PD1             PWR_PDCRD_PD1_Msk                            /*!< Pin PD1 Pull-Down set */
#define PWR_PDCRD_PD2_Pos         (2U)
#define PWR_PDCRD_PD2_Msk         (0x1UL << PWR_PDCRD_PD2_Pos)                 /*!< 0x00000004 */
#define PWR_PDCRD_PD2             PWR_PDCRD_PD2_Msk                            /*!< Pin PD2 Pull-Down set */
#define PWR_PDCRD_PD3_Pos         (3U)
#define PWR_PDCRD_PD3_Msk         (0x1UL << PWR_PDCRD_PD3_Pos)                 /*!< 0x00000008 */
#define PWR_PDCRD_PD3             PWR_PDCRD_PD3_Msk                            /*!< Pin PD3 Pull-Down set */
#define PWR_PDCRD_PD4_Pos         (4U)
#define PWR_PDCRD_PD4_Msk         (0x1UL << PWR_PDCRD_PD4_Pos)                 /*!< 0x00000010 */
#define PWR_PDCRD_PD4             PWR_PDCRD_PD4_Msk                            /*!< Pin PD4 Pull-Down set */
#define PWR_PDCRD_PD5_Pos         (5U)
#define PWR_PDCRD_PD5_Msk         (0x1UL << PWR_PDCRD_PD5_Pos)                 /*!< 0x00000020 */
#define PWR_PDCRD_PD5             PWR_PDCRD_PD5_Msk                            /*!< Pin PD5 Pull-Down set */
#define PWR_PDCRD_PD6_Pos         (6U)
#define PWR_PDCRD_PD6_Msk         (0x1UL << PWR_PDCRD_PD6_Pos)                 /*!< 0x00000040 */
#define PWR_PDCRD_PD6             PWR_PDCRD_PD6_Msk                            /*!< Pin PD6 Pull-Down set */
#define PWR_PDCRD_PD8_Pos         (8U)
#define PWR_PDCRD_PD8_Msk         (0x1UL << PWR_PDCRD_PD8_Pos)                 /*!< 0x00000100 */
#define PWR_PDCRD_PD8             PWR_PDCRD_PD8_Msk                            /*!< Pin PD8 Pull-Down set */
#define PWR_PDCRD_PD9_Pos         (9U)
#define PWR_PDCRD_PD9_Msk         (0x1UL << PWR_PDCRD_PD9_Pos)                 /*!< 0x00000200 */
#define PWR_PDCRD_PD9             PWR_PDCRD_PD9_Msk                            /*!< Pin PD9 Pull-Down set */

/********************  Bit definition for PWR_PUCRF register  *****************/
#define PWR_PUCRF_PU0_Pos         (0U)
#define PWR_PUCRF_PU0_Msk         (0x1UL << PWR_PUCRF_PU0_Pos)                 /*!< 0x00000001 */
#define PWR_PUCRF_PU0             PWR_PUCRF_PU0_Msk                            /*!< Pin PF0 Pull-Up set */
#define PWR_PUCRF_PU1_Pos         (1U)
#define PWR_PUCRF_PU1_Msk         (0x1UL << PWR_PUCRF_PU1_Pos)                 /*!< 0x00000002 */
#define PWR_PUCRF_PU1             PWR_PUCRF_PU1_Msk                            /*!< Pin PF1 Pull-Up set */
#define PWR_PUCRF_PU2_Pos         (2U)
#define PWR_PUCRF_PU2_Msk         (0x1UL << PWR_PUCRF_PU2_Pos)                 /*!< 0x00000004 */
#define PWR_PUCRF_PU2             PWR_PUCRF_PU2_Msk                            /*!< Pin PF2 Pull-Up set */
#define PWR_PUCRF_PU3_Pos         (3U)
#define PWR_PUCRF_PU3_Msk         (0x1UL << PWR_PUCRF_PU3_Pos)                 /*!< 0x00000008 */
#define PWR_PUCRF_PU3             PWR_PUCRF_PU3_Msk                            /*!< Pin PF3 Pull-Up set */
#define PWR_PUCRF_PU4_Pos         (4U)
#define PWR_PUCRF_PU4_Msk         (0x1UL << PWR_PUCRF_PU4_Pos)                 /*!< 0x00000010 */
#define PWR_PUCRF_PU4             PWR_PUCRF_PU4_Msk                            /*!< Pin PF4 Pull-Up set */

/********************  Bit definition for PWR_PDCRF register  *****************/
#define PWR_PDCRF_PD0_Pos         (0U)
#define PWR_PDCRF_PD0_Msk         (0x1UL << PWR_PDCRF_PD0_Pos)                 /*!< 0x00000001 */
#define PWR_PDCRF_PD0             PWR_PDCRF_PD0_Msk                            /*!< Pin PF0 Pull-Down set */
#define PWR_PDCRF_PD1_Pos         (1U)
#define PWR_PDCRF_PD1_Msk         (0x1UL << PWR_PDCRF_PD1_Pos)                 /*!< 0x00000002 */
#define PWR_PDCRF_PD1             PWR_PDCRF_PD1_Msk                            /*!< Pin PF1 Pull-Down set */
#define PWR_PDCRF_PD2_Pos         (2U)
#define PWR_PDCRF_PD2_Msk         (0x1UL << PWR_PDCRF_PD2_Pos)                 /*!< 0x00000004 */
#define PWR_PDCRF_PD2             PWR_PDCRF_PD2_Msk                            /*!< Pin PF2 Pull-Down set */
#define PWR_PDCRF_PD3_Pos         (3U)
#define PWR_PDCRF_PD3_Msk         (0x1UL << PWR_PDCRF_PD3_Pos)                 /*!< 0x00000008 */
#define PWR_PDCRF_PD3             PWR_PDCRF_PD3_Msk                            /*!< Pin PF3 Pull-Down set */
#define PWR_PDCRF_PD4_Pos         (4U)
#define PWR_PDCRF_PD4_Msk         (0x1UL << PWR_PDCRF_PD4_Pos)                 /*!< 0x00000010 */
#define PWR_PDCRF_PD4             PWR_PDCRF_PD4_Msk                            /*!< Pin PF4 Pull-Down set */

/******************************************************************************/
/*                                                                            */
/*                           Reset and Clock Control                          */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions  (not present on all devices in the STM32G0 serie)
*/
#define RCC_PLLQ_SUPPORT

/********************  Bit definition for RCC_CR register  *****************/
#define RCC_CR_HSION_Pos                 (8U)
#define RCC_CR_HSION_Msk                 (0x1UL << RCC_CR_HSION_Pos)           /*!< 0x00000100 */
#define RCC_CR_HSION                     RCC_CR_HSION_Msk                      /*!< Internal High Speed clock enable */
#define RCC_CR_HSIKERON_Pos              (9U)
#define RCC_CR_HSIKERON_Msk              (0x1UL << RCC_CR_HSIKERON_Pos)        /*!< 0x00000200 */
#define RCC_CR_HSIKERON                  RCC_CR_HSIKERON_Msk                   /*!< Internal High Speed clock enable for some IPs Kernel */
#define RCC_CR_HSIRDY_Pos                (10U)
#define RCC_CR_HSIRDY_Msk                (0x1UL << RCC_CR_HSIRDY_Pos)          /*!< 0x00000400 */
#define RCC_CR_HSIRDY                    RCC_CR_HSIRDY_Msk                     /*!< Internal High Speed clock ready flag */
#define RCC_CR_HSIDIV_Pos                (11U)
#define RCC_CR_HSIDIV_Msk                (0x7UL << RCC_CR_HSIDIV_Pos)          /*!< 0x00003800 */
#define RCC_CR_HSIDIV                    RCC_CR_HSIDIV_Msk                     /*!< HSIDIV[13:11] Internal High Speed clock division factor */
#define RCC_CR_HSIDIV_0                  (0x1UL << RCC_CR_HSIDIV_Pos)          /*!< 0x00000800 */
#define RCC_CR_HSIDIV_1                  (0x2UL << RCC_CR_HSIDIV_Pos)          /*!< 0x00001000 */
#define RCC_CR_HSIDIV_2                  (0x4UL << RCC_CR_HSIDIV_Pos)          /*!< 0x00002000 */
#define RCC_CR_HSEON_Pos                 (16U)
#define RCC_CR_HSEON_Msk                 (0x1UL << RCC_CR_HSEON_Pos)           /*!< 0x00010000 */
#define RCC_CR_HSEON                     RCC_CR_HSEON_Msk                      /*!< External High Speed clock enable */
#define RCC_CR_HSERDY_Pos                (17U)
#define RCC_CR_HSERDY_Msk                (0x1UL << RCC_CR_HSERDY_Pos)          /*!< 0x00020000 */
#define RCC_CR_HSERDY                    RCC_CR_HSERDY_Msk                     /*!< External High Speed clock ready */
#define RCC_CR_HSEBYP_Pos                (18U)
#define RCC_CR_HSEBYP_Msk                (0x1UL << RCC_CR_HSEBYP_Pos)          /*!< 0x00040000 */
#define RCC_CR_HSEBYP                    RCC_CR_HSEBYP_Msk                     /*!< External High Speed clock Bypass */
#define RCC_CR_CSSON_Pos                 (19U)
#define RCC_CR_CSSON_Msk                 (0x1UL << RCC_CR_CSSON_Pos)           /*!< 0x00080000 */
#define RCC_CR_CSSON                     RCC_CR_CSSON_Msk                      /*!< HSE Clock Security System enable */

#define RCC_CR_PLLON_Pos                 (24U)
#define RCC_CR_PLLON_Msk                 (0x1UL << RCC_CR_PLLON_Pos)           /*!< 0x01000000 */
#define RCC_CR_PLLON                     RCC_CR_PLLON_Msk                      /*!< System PLL clock enable */
#define RCC_CR_PLLRDY_Pos                (25U)
#define RCC_CR_PLLRDY_Msk                (0x1UL << RCC_CR_PLLRDY_Pos)          /*!< 0x02000000 */
#define RCC_CR_PLLRDY                    RCC_CR_PLLRDY_Msk                     /*!< System PLL clock ready */

/********************  Bit definition for RCC_ICSCR register  ***************/
/*!< HSICAL configuration */
#define RCC_ICSCR_HSICAL_Pos             (0U)
#define RCC_ICSCR_HSICAL_Msk             (0xFFUL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x000000FF */
#define RCC_ICSCR_HSICAL                 RCC_ICSCR_HSICAL_Msk                  /*!< HSICAL[7:0] bits */
#define RCC_ICSCR_HSICAL_0               (0x01UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000001 */
#define RCC_ICSCR_HSICAL_1               (0x02UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000002 */
#define RCC_ICSCR_HSICAL_2               (0x04UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000004 */
#define RCC_ICSCR_HSICAL_3               (0x08UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000008 */
#define RCC_ICSCR_HSICAL_4               (0x10UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000010 */
#define RCC_ICSCR_HSICAL_5               (0x20UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000020 */
#define RCC_ICSCR_HSICAL_6               (0x40UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000040 */
#define RCC_ICSCR_HSICAL_7               (0x80UL << RCC_ICSCR_HSICAL_Pos)      /*!< 0x00000080 */

/*!< HSITRIM configuration */
#define RCC_ICSCR_HSITRIM_Pos            (8U)
#define RCC_ICSCR_HSITRIM_Msk            (0x7FUL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00007F00 */
#define RCC_ICSCR_HSITRIM                RCC_ICSCR_HSITRIM_Msk                 /*!< HSITRIM[14:8] bits */
#define RCC_ICSCR_HSITRIM_0              (0x01UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00000100 */
#define RCC_ICSCR_HSITRIM_1              (0x02UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00000200 */
#define RCC_ICSCR_HSITRIM_2              (0x04UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00000400 */
#define RCC_ICSCR_HSITRIM_3              (0x08UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00000800 */
#define RCC_ICSCR_HSITRIM_4              (0x10UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00001000 */
#define RCC_ICSCR_HSITRIM_5              (0x20UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00002000 */
#define RCC_ICSCR_HSITRIM_6              (0x40UL << RCC_ICSCR_HSITRIM_Pos)     /*!< 0x00004000 */

/********************  Bit definition for RCC_CFGR register  ***************/
/*!< SW configuration */
#define RCC_CFGR_SW_Pos                (0U)
#define RCC_CFGR_SW_Msk                (0x7UL << RCC_CFGR_SW_Pos)              /*!< 0x00000007 */
#define RCC_CFGR_SW                    RCC_CFGR_SW_Msk                         /*!< SW[2:0] bits (System clock Switch) */
#define RCC_CFGR_SW_0                  (0x1UL << RCC_CFGR_SW_Pos)              /*!< 0x00000001 */
#define RCC_CFGR_SW_1                  (0x2UL << RCC_CFGR_SW_Pos)              /*!< 0x00000002 */
#define RCC_CFGR_SW_2                  (0x4UL << RCC_CFGR_SW_Pos)              /*!< 0x00000004 */

/*!< SWS configuration */
#define RCC_CFGR_SWS_Pos               (3U)
#define RCC_CFGR_SWS_Msk               (0x7UL << RCC_CFGR_SWS_Pos)             /*!< 0x00000038 */
#define RCC_CFGR_SWS                   RCC_CFGR_SWS_Msk                        /*!< SWS[2:0] bits (System Clock Switch Status) */
#define RCC_CFGR_SWS_0                 (0x1UL << RCC_CFGR_SWS_Pos)             /*!< 0x00000008 */
#define RCC_CFGR_SWS_1                 (0x2UL << RCC_CFGR_SWS_Pos)             /*!< 0x00000010 */
#define RCC_CFGR_SWS_2                 (0x4UL << RCC_CFGR_SWS_Pos)             /*!< 0x00000020 */

#define RCC_CFGR_SWS_HSI               (0UL)                                   /*!< HSI used as system clock */
#define RCC_CFGR_SWS_HSE               (0x00000008UL)                          /*!< HSE used as system clock */
#define RCC_CFGR_SWS_PLL               (0x00000010UL)                          /*!< PLL used as system clock */
#define RCC_CFGR_SWS_LSI               (0x00000018UL)                          /*!< LSI used as system clock */
#define RCC_CFGR_SWS_LSE               (0x00000020UL)                          /*!< LSE used as system clock */

/*!< HPRE configuration */
#define RCC_CFGR_HPRE_Pos              (8U)
#define RCC_CFGR_HPRE_Msk              (0xFUL << RCC_CFGR_HPRE_Pos)            /*!< 0x00000F00 */
#define RCC_CFGR_HPRE                  RCC_CFGR_HPRE_Msk                       /*!< HPRE[3:0] bits (AHB prescaler) */
#define RCC_CFGR_HPRE_0                (0x1UL << RCC_CFGR_HPRE_Pos)            /*!< 0x00000100 */
#define RCC_CFGR_HPRE_1                (0x2UL << RCC_CFGR_HPRE_Pos)            /*!< 0x00000200 */
#define RCC_CFGR_HPRE_2                (0x4UL << RCC_CFGR_HPRE_Pos)            /*!< 0x00000400 */
#define RCC_CFGR_HPRE_3                (0x8UL << RCC_CFGR_HPRE_Pos)            /*!< 0x00000800 */

/*!< PPRE configuration */
#define RCC_CFGR_PPRE_Pos              (12U)
#define RCC_CFGR_PPRE_Msk              (0x7UL << RCC_CFGR_PPRE_Pos)            /*!< 0x00007000 */
#define RCC_CFGR_PPRE                  RCC_CFGR_PPRE_Msk                       /*!< PRE1[2:0] bits (APB prescaler) */
#define RCC_CFGR_PPRE_0                (0x1UL << RCC_CFGR_PPRE_Pos)            /*!< 0x00001000 */
#define RCC_CFGR_PPRE_1                (0x2UL << RCC_CFGR_PPRE_Pos)            /*!< 0x00002000 */
#define RCC_CFGR_PPRE_2                (0x4UL << RCC_CFGR_PPRE_Pos)            /*!< 0x00004000 */

/*!< MCOSEL configuration */
#define RCC_CFGR_MCOSEL_Pos            (24U)
#define RCC_CFGR_MCOSEL_Msk            (0x7UL << RCC_CFGR_MCOSEL_Pos)          /*!< 0x0F000000 */
#define RCC_CFGR_MCOSEL                RCC_CFGR_MCOSEL_Msk                     /*!< MCOSEL [2:0] bits (Clock output selection) */
#define RCC_CFGR_MCOSEL_0              (0x1UL << RCC_CFGR_MCOSEL_Pos)          /*!< 0x01000000 */
#define RCC_CFGR_MCOSEL_1              (0x2UL << RCC_CFGR_MCOSEL_Pos)          /*!< 0x02000000 */
#define RCC_CFGR_MCOSEL_2              (0x4UL << RCC_CFGR_MCOSEL_Pos)          /*!< 0x04000000 */

/*!< MCO Prescaler configuration */
#define RCC_CFGR_MCOPRE_Pos            (28U)
#define RCC_CFGR_MCOPRE_Msk            (0x7UL << RCC_CFGR_MCOPRE_Pos)          /*!< 0x70000000 */
#define RCC_CFGR_MCOPRE                RCC_CFGR_MCOPRE_Msk                     /*!< MCO prescaler [2:0] */
#define RCC_CFGR_MCOPRE_0              (0x1UL << RCC_CFGR_MCOPRE_Pos)          /*!< 0x10000000 */
#define RCC_CFGR_MCOPRE_1              (0x2UL << RCC_CFGR_MCOPRE_Pos)          /*!< 0x20000000 */
#define RCC_CFGR_MCOPRE_2              (0x4UL << RCC_CFGR_MCOPRE_Pos)          /*!< 0x40000000 */

/********************  Bit definition for RCC_PLLCFGR register  ***************/
#define RCC_PLLCFGR_PLLSRC_Pos           (0U)
#define RCC_PLLCFGR_PLLSRC_Msk           (0x3UL << RCC_PLLCFGR_PLLSRC_Pos)     /*!< 0x00000003 */
#define RCC_PLLCFGR_PLLSRC               RCC_PLLCFGR_PLLSRC_Msk
#define RCC_PLLCFGR_PLLSRC_0             (0x1UL << RCC_PLLCFGR_PLLSRC_Pos)     /*!< 0x00000001 */
#define RCC_PLLCFGR_PLLSRC_1             (0x2UL << RCC_PLLCFGR_PLLSRC_Pos)     /*!< 0x00000002 */

#define RCC_PLLCFGR_PLLSRC_NONE          (0x00000000UL)                        /*!< No clock sent to PLL      */
#define RCC_PLLCFGR_PLLSRC_HSI_Pos       (1U)
#define RCC_PLLCFGR_PLLSRC_HSI_Msk       (0x1UL << RCC_PLLCFGR_PLLSRC_HSI_Pos) /*!< 0x00000002 */
#define RCC_PLLCFGR_PLLSRC_HSI           RCC_PLLCFGR_PLLSRC_HSI_Msk            /*!< HSI source clock selected */
#define RCC_PLLCFGR_PLLSRC_HSE_Pos       (0U)
#define RCC_PLLCFGR_PLLSRC_HSE_Msk       (0x3UL << RCC_PLLCFGR_PLLSRC_HSE_Pos) /*!< 0x00000003 */
#define RCC_PLLCFGR_PLLSRC_HSE           RCC_PLLCFGR_PLLSRC_HSE_Msk            /*!< HSE source clock selected */

#define RCC_PLLCFGR_PLLM_Pos             (4U)
#define RCC_PLLCFGR_PLLM_Msk             (0x7UL << RCC_PLLCFGR_PLLM_Pos)       /*!< 0x00000070 */
#define RCC_PLLCFGR_PLLM                 RCC_PLLCFGR_PLLM_Msk
#define RCC_PLLCFGR_PLLM_0               (0x1UL << RCC_PLLCFGR_PLLM_Pos)       /*!< 0x00000010 */
#define RCC_PLLCFGR_PLLM_1               (0x2UL << RCC_PLLCFGR_PLLM_Pos)       /*!< 0x00000020 */
#define RCC_PLLCFGR_PLLM_2               (0x4UL << RCC_PLLCFGR_PLLM_Pos)       /*!< 0x00000040 */

#define RCC_PLLCFGR_PLLN_Pos             (8U)
#define RCC_PLLCFGR_PLLN_Msk             (0x7FUL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00007F00 */
#define RCC_PLLCFGR_PLLN                 RCC_PLLCFGR_PLLN_Msk
#define RCC_PLLCFGR_PLLN_0               (0x01UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00000100 */
#define RCC_PLLCFGR_PLLN_1               (0x02UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00000200 */
#define RCC_PLLCFGR_PLLN_2               (0x04UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00000400 */
#define RCC_PLLCFGR_PLLN_3               (0x08UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00000800 */
#define RCC_PLLCFGR_PLLN_4               (0x10UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00001000 */
#define RCC_PLLCFGR_PLLN_5               (0x20UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00002000 */
#define RCC_PLLCFGR_PLLN_6               (0x40UL << RCC_PLLCFGR_PLLN_Pos)      /*!< 0x00004000 */

#define RCC_PLLCFGR_PLLPEN_Pos           (16U)
#define RCC_PLLCFGR_PLLPEN_Msk           (0x1UL << RCC_PLLCFGR_PLLPEN_Pos)     /*!< 0x00010000 */
#define RCC_PLLCFGR_PLLPEN               RCC_PLLCFGR_PLLPEN_Msk

#define RCC_PLLCFGR_PLLP_Pos              (17U)
#define RCC_PLLCFGR_PLLP_Msk              (0x1FUL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x003E0000 */
#define RCC_PLLCFGR_PLLP                  RCC_PLLCFGR_PLLP_Msk
#define RCC_PLLCFGR_PLLP_0                (0x01UL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x00020000 */
#define RCC_PLLCFGR_PLLP_1                (0x02UL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x00040000 */
#define RCC_PLLCFGR_PLLP_2                (0x04UL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x00080000 */
#define RCC_PLLCFGR_PLLP_3                (0x08UL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x00100000 */
#define RCC_PLLCFGR_PLLP_4                (0x10UL << RCC_PLLCFGR_PLLP_Pos)     /*!< 0x00200000 */

#define RCC_PLLCFGR_PLLQEN_Pos           (24U)
#define RCC_PLLCFGR_PLLQEN_Msk           (0x1UL << RCC_PLLCFGR_PLLQEN_Pos)     /*!< 0x01000000 */
#define RCC_PLLCFGR_PLLQEN               RCC_PLLCFGR_PLLQEN_Msk

#define RCC_PLLCFGR_PLLQ_Pos             (25U)
#define RCC_PLLCFGR_PLLQ_Msk             (0x7UL << RCC_PLLCFGR_PLLQ_Pos)       /*!< 0x0E000000 */
#define RCC_PLLCFGR_PLLQ                 RCC_PLLCFGR_PLLQ_Msk
#define RCC_PLLCFGR_PLLQ_0               (0x1UL << RCC_PLLCFGR_PLLQ_Pos)       /*!< 0x02000000 */
#define RCC_PLLCFGR_PLLQ_1               (0x2UL << RCC_PLLCFGR_PLLQ_Pos)       /*!< 0x04000000 */
#define RCC_PLLCFGR_PLLQ_2               (0x4UL << RCC_PLLCFGR_PLLQ_Pos)       /*!< 0x08000000 */

#define RCC_PLLCFGR_PLLREN_Pos           (28U)
#define RCC_PLLCFGR_PLLREN_Msk           (0x1UL << RCC_PLLCFGR_PLLREN_Pos)     /*!< 0x10000000 */
#define RCC_PLLCFGR_PLLREN               RCC_PLLCFGR_PLLREN_Msk

#define RCC_PLLCFGR_PLLR_Pos             (29U)
#define RCC_PLLCFGR_PLLR_Msk             (0x7UL << RCC_PLLCFGR_PLLR_Pos)       /*!< 0xE0000000 */
#define RCC_PLLCFGR_PLLR                 RCC_PLLCFGR_PLLR_Msk
#define RCC_PLLCFGR_PLLR_0               (0x1UL << RCC_PLLCFGR_PLLR_Pos)       /*!< 0x20000000 */
#define RCC_PLLCFGR_PLLR_1               (0x2UL << RCC_PLLCFGR_PLLR_Pos)       /*!< 0x40000000 */
#define RCC_PLLCFGR_PLLR_2               (0x4UL << RCC_PLLCFGR_PLLR_Pos)       /*!< 0x80000000 */

/********************  Bit definition for RCC_CIER register  ******************/
#define RCC_CIER_LSIRDYIE_Pos            (0U)
#define RCC_CIER_LSIRDYIE_Msk            (0x1UL << RCC_CIER_LSIRDYIE_Pos)      /*!< 0x00000001 */
#define RCC_CIER_LSIRDYIE                RCC_CIER_LSIRDYIE_Msk
#define RCC_CIER_LSERDYIE_Pos            (1U)
#define RCC_CIER_LSERDYIE_Msk            (0x1UL << RCC_CIER_LSERDYIE_Pos)      /*!< 0x00000002 */
#define RCC_CIER_LSERDYIE                RCC_CIER_LSERDYIE_Msk
#define RCC_CIER_HSIRDYIE_Pos            (3U)
#define RCC_CIER_HSIRDYIE_Msk            (0x1UL << RCC_CIER_HSIRDYIE_Pos)      /*!< 0x00000008 */
#define RCC_CIER_HSIRDYIE                RCC_CIER_HSIRDYIE_Msk
#define RCC_CIER_HSERDYIE_Pos            (4U)
#define RCC_CIER_HSERDYIE_Msk            (0x1UL << RCC_CIER_HSERDYIE_Pos)      /*!< 0x00000010 */
#define RCC_CIER_HSERDYIE                RCC_CIER_HSERDYIE_Msk
#define RCC_CIER_PLLRDYIE_Pos            (5U)
#define RCC_CIER_PLLRDYIE_Msk            (0x1UL << RCC_CIER_PLLRDYIE_Pos)      /*!< 0x00000020 */
#define RCC_CIER_PLLRDYIE                RCC_CIER_PLLRDYIE_Msk

/********************  Bit definition for RCC_CIFR register  ******************/
#define RCC_CIFR_LSIRDYF_Pos             (0U)
#define RCC_CIFR_LSIRDYF_Msk             (0x1UL << RCC_CIFR_LSIRDYF_Pos)       /*!< 0x00000001 */
#define RCC_CIFR_LSIRDYF                 RCC_CIFR_LSIRDYF_Msk
#define RCC_CIFR_LSERDYF_Pos             (1U)
#define RCC_CIFR_LSERDYF_Msk             (0x1UL << RCC_CIFR_LSERDYF_Pos)       /*!< 0x00000002 */
#define RCC_CIFR_LSERDYF                 RCC_CIFR_LSERDYF_Msk
#define RCC_CIFR_HSIRDYF_Pos             (3U)
#define RCC_CIFR_HSIRDYF_Msk             (0x1UL << RCC_CIFR_HSIRDYF_Pos)       /*!< 0x00000008 */
#define RCC_CIFR_HSIRDYF                 RCC_CIFR_HSIRDYF_Msk
#define RCC_CIFR_HSERDYF_Pos             (4U)
#define RCC_CIFR_HSERDYF_Msk             (0x1UL << RCC_CIFR_HSERDYF_Pos)       /*!< 0x00000010 */
#define RCC_CIFR_HSERDYF                 RCC_CIFR_HSERDYF_Msk
#define RCC_CIFR_PLLRDYF_Pos             (5U)
#define RCC_CIFR_PLLRDYF_Msk             (0x1UL << RCC_CIFR_PLLRDYF_Pos)       /*!< 0x00000020 */
#define RCC_CIFR_PLLRDYF                 RCC_CIFR_PLLRDYF_Msk
#define RCC_CIFR_CSSF_Pos                (8U)
#define RCC_CIFR_CSSF_Msk                (0x1UL << RCC_CIFR_CSSF_Pos)          /*!< 0x00000100 */
#define RCC_CIFR_CSSF                    RCC_CIFR_CSSF_Msk
#define RCC_CIFR_LSECSSF_Pos             (9U)
#define RCC_CIFR_LSECSSF_Msk             (0x1UL << RCC_CIFR_LSECSSF_Pos)       /*!< 0x00000200 */
#define RCC_CIFR_LSECSSF                 RCC_CIFR_LSECSSF_Msk

/********************  Bit definition for RCC_CICR register  ******************/
#define RCC_CICR_LSIRDYC_Pos             (0U)
#define RCC_CICR_LSIRDYC_Msk             (0x1UL << RCC_CICR_LSIRDYC_Pos)       /*!< 0x00000001 */
#define RCC_CICR_LSIRDYC                 RCC_CICR_LSIRDYC_Msk
#define RCC_CICR_LSERDYC_Pos             (1U)
#define RCC_CICR_LSERDYC_Msk             (0x1UL << RCC_CICR_LSERDYC_Pos)       /*!< 0x00000002 */
#define RCC_CICR_LSERDYC                 RCC_CICR_LSERDYC_Msk
#define RCC_CICR_HSIRDYC_Pos             (3U)
#define RCC_CICR_HSIRDYC_Msk             (0x1UL << RCC_CICR_HSIRDYC_Pos)       /*!< 0x00000008 */
#define RCC_CICR_HSIRDYC                 RCC_CICR_HSIRDYC_Msk
#define RCC_CICR_HSERDYC_Pos             (4U)
#define RCC_CICR_HSERDYC_Msk             (0x1UL << RCC_CICR_HSERDYC_Pos)       /*!< 0x00000010 */
#define RCC_CICR_HSERDYC                 RCC_CICR_HSERDYC_Msk
#define RCC_CICR_PLLRDYC_Pos             (5U)
#define RCC_CICR_PLLRDYC_Msk             (0x1UL << RCC_CICR_PLLRDYC_Pos)       /*!< 0x00000020 */
#define RCC_CICR_PLLRDYC                 RCC_CICR_PLLRDYC_Msk
#define RCC_CICR_CSSC_Pos                (8U)
#define RCC_CICR_CSSC_Msk                (0x1UL << RCC_CICR_CSSC_Pos)          /*!< 0x00000100 */
#define RCC_CICR_CSSC                    RCC_CICR_CSSC_Msk
#define RCC_CICR_LSECSSC_Pos             (9U)
#define RCC_CICR_LSECSSC_Msk             (0x1UL << RCC_CICR_LSECSSC_Pos)       /*!< 0x00000200 */
#define RCC_CICR_LSECSSC                 RCC_CICR_LSECSSC_Msk

/********************  Bit definition for RCC_IOPRSTR register  ****************/
#define RCC_IOPRSTR_GPIOARST_Pos         (0U)
#define RCC_IOPRSTR_GPIOARST_Msk         (0x1UL << RCC_IOPRSTR_GPIOARST_Pos)   /*!< 0x00000001 */
#define RCC_IOPRSTR_GPIOARST             RCC_IOPRSTR_GPIOARST_Msk
#define RCC_IOPRSTR_GPIOBRST_Pos         (1U)
#define RCC_IOPRSTR_GPIOBRST_Msk         (0x1UL << RCC_IOPRSTR_GPIOBRST_Pos)   /*!< 0x00000002 */
#define RCC_IOPRSTR_GPIOBRST             RCC_IOPRSTR_GPIOBRST_Msk
#define RCC_IOPRSTR_GPIOCRST_Pos         (2U)
#define RCC_IOPRSTR_GPIOCRST_Msk         (0x1UL << RCC_IOPRSTR_GPIOCRST_Pos)   /*!< 0x00000004 */
#define RCC_IOPRSTR_GPIOCRST             RCC_IOPRSTR_GPIOCRST_Msk
#define RCC_IOPRSTR_GPIODRST_Pos         (3U)
#define RCC_IOPRSTR_GPIODRST_Msk         (0x1UL << RCC_IOPRSTR_GPIODRST_Pos)   /*!< 0x00000008 */
#define RCC_IOPRSTR_GPIODRST             RCC_IOPRSTR_GPIODRST_Msk
#define RCC_IOPRSTR_GPIOFRST_Pos         (5U)
#define RCC_IOPRSTR_GPIOFRST_Msk         (0x1UL << RCC_IOPRSTR_GPIOFRST_Pos)   /*!< 0x00000020 */
#define RCC_IOPRSTR_GPIOFRST             RCC_IOPRSTR_GPIOFRST_Msk

/********************  Bit definition for RCC_AHBRSTR register  ***************/
#define RCC_AHBRSTR_DMA1RST_Pos          (0U)
#define RCC_AHBRSTR_DMA1RST_Msk          (0x1UL << RCC_AHBRSTR_DMA1RST_Pos)    /*!< 0x00000001 */
#define RCC_AHBRSTR_DMA1RST              RCC_AHBRSTR_DMA1RST_Msk
#define RCC_AHBRSTR_FLASHRST_Pos         (8U)
#define RCC_AHBRSTR_FLASHRST_Msk         (0x1UL << RCC_AHBRSTR_FLASHRST_Pos)   /*!< 0x00000100 */
#define RCC_AHBRSTR_FLASHRST             RCC_AHBRSTR_FLASHRST_Msk
#define RCC_AHBRSTR_CRCRST_Pos           (12U)
#define RCC_AHBRSTR_CRCRST_Msk           (0x1UL << RCC_AHBRSTR_CRCRST_Pos)     /*!< 0x00001000 */
#define RCC_AHBRSTR_CRCRST               RCC_AHBRSTR_CRCRST_Msk
#define RCC_AHBRSTR_AESRST_Pos           (16U)
#define RCC_AHBRSTR_AESRST_Msk           (0x1UL << RCC_AHBRSTR_AESRST_Pos)     /*!< 0x00010000 */
#define RCC_AHBRSTR_AESRST               RCC_AHBRSTR_AESRST_Msk
#define RCC_AHBRSTR_RNGRST_Pos           (18U)
#define RCC_AHBRSTR_RNGRST_Msk           (0x1UL << RCC_AHBRSTR_RNGRST_Pos)     /*!< 0x00040000 */
#define RCC_AHBRSTR_RNGRST               RCC_AHBRSTR_RNGRST_Msk

/********************  Bit definition for RCC_APBRSTR1 register  **************/
#define RCC_APBRSTR1_TIM2RST_Pos         (0U)
#define RCC_APBRSTR1_TIM2RST_Msk         (0x1UL << RCC_APBRSTR1_TIM2RST_Pos)   /*!< 0x00000001 */
#define RCC_APBRSTR1_TIM2RST             RCC_APBRSTR1_TIM2RST_Msk
#define RCC_APBRSTR1_TIM3RST_Pos         (1U)
#define RCC_APBRSTR1_TIM3RST_Msk         (0x1UL << RCC_APBRSTR1_TIM3RST_Pos)   /*!< 0x00000002 */
#define RCC_APBRSTR1_TIM3RST             RCC_APBRSTR1_TIM3RST_Msk
#define RCC_APBRSTR1_TIM6RST_Pos         (4U)
#define RCC_APBRSTR1_TIM6RST_Msk         (0x1UL << RCC_APBRSTR1_TIM6RST_Pos)   /*!< 0x00000010 */
#define RCC_APBRSTR1_TIM6RST             RCC_APBRSTR1_TIM6RST_Msk
#define RCC_APBRSTR1_TIM7RST_Pos         (5U)
#define RCC_APBRSTR1_TIM7RST_Msk         (0x1UL << RCC_APBRSTR1_TIM7RST_Pos)   /*!< 0x00000020 */
#define RCC_APBRSTR1_TIM7RST             RCC_APBRSTR1_TIM7RST_Msk
#define RCC_APBRSTR1_SPI2RST_Pos         (14U)
#define RCC_APBRSTR1_SPI2RST_Msk         (0x1UL << RCC_APBRSTR1_SPI2RST_Pos)   /*!< 0x00004000 */
#define RCC_APBRSTR1_SPI2RST             RCC_APBRSTR1_SPI2RST_Msk
#define RCC_APBRSTR1_USART2RST_Pos       (17U)
#define RCC_APBRSTR1_USART2RST_Msk       (0x1UL << RCC_APBRSTR1_USART2RST_Pos) /*!< 0x00020000 */
#define RCC_APBRSTR1_USART2RST           RCC_APBRSTR1_USART2RST_Msk
#define RCC_APBRSTR1_USART3RST_Pos       (18U)
#define RCC_APBRSTR1_USART3RST_Msk       (0x1UL << RCC_APBRSTR1_USART3RST_Pos) /*!< 0x00040000 */
#define RCC_APBRSTR1_USART3RST           RCC_APBRSTR1_USART3RST_Msk
#define RCC_APBRSTR1_USART4RST_Pos       (19U)
#define RCC_APBRSTR1_USART4RST_Msk       (0x1UL << RCC_APBRSTR1_USART4RST_Pos) /*!< 0x00080000 */
#define RCC_APBRSTR1_USART4RST           RCC_APBRSTR1_USART4RST_Msk
#define RCC_APBRSTR1_LPUART1RST_Pos      (20U)
#define RCC_APBRSTR1_LPUART1RST_Msk      (0x1UL << RCC_APBRSTR1_LPUART1RST_Pos) /*!< 0x00100000 */
#define RCC_APBRSTR1_LPUART1RST          RCC_APBRSTR1_LPUART1RST_Msk
#define RCC_APBRSTR1_I2C1RST_Pos         (21U)
#define RCC_APBRSTR1_I2C1RST_Msk         (0x1UL << RCC_APBRSTR1_I2C1RST_Pos)    /*!< 0x00200000 */
#define RCC_APBRSTR1_I2C1RST             RCC_APBRSTR1_I2C1RST_Msk
#define RCC_APBRSTR1_I2C2RST_Pos         (22U)
#define RCC_APBRSTR1_I2C2RST_Msk         (0x1UL << RCC_APBRSTR1_I2C2RST_Pos)    /*!< 0x00400000 */
#define RCC_APBRSTR1_I2C2RST             RCC_APBRSTR1_I2C2RST_Msk
#define RCC_APBRSTR1_CECRST_Pos          (24U)
#define RCC_APBRSTR1_CECRST_Msk          (0x1UL << RCC_APBRSTR1_CECRST_Pos)     /*!< 0x01000000 */
#define RCC_APBRSTR1_CECRST              RCC_APBRSTR1_CECRST_Msk
#define RCC_APBRSTR1_UCPD1RST_Pos        (25U)
#define RCC_APBRSTR1_UCPD1RST_Msk        (0x1UL << RCC_APBRSTR1_UCPD1RST_Pos)   /*!< 0x02000000 */
#define RCC_APBRSTR1_UCPD1RST            RCC_APBRSTR1_UCPD1RST_Msk
#define RCC_APBRSTR1_UCPD2RST_Pos        (26U)
#define RCC_APBRSTR1_UCPD2RST_Msk        (0x1UL << RCC_APBRSTR1_UCPD2RST_Pos)   /*!< 0x04000000 */
#define RCC_APBRSTR1_UCPD2RST            RCC_APBRSTR1_UCPD2RST_Msk
#define RCC_APBRSTR1_DBGRST_Pos          (27U)
#define RCC_APBRSTR1_DBGRST_Msk          (0x1UL << RCC_APBRSTR1_DBGRST_Pos)     /*!< 0x08000000 */
#define RCC_APBRSTR1_DBGRST              RCC_APBRSTR1_DBGRST_Msk
#define RCC_APBRSTR1_PWRRST_Pos          (28U)
#define RCC_APBRSTR1_PWRRST_Msk          (0x1UL << RCC_APBRSTR1_PWRRST_Pos)     /*!< 0x10000000 */
#define RCC_APBRSTR1_PWRRST              RCC_APBRSTR1_PWRRST_Msk
#define RCC_APBRSTR1_DAC1RST_Pos         (29U)
#define RCC_APBRSTR1_DAC1RST_Msk         (0x1UL << RCC_APBRSTR1_DAC1RST_Pos)    /*!< 0x20000000 */
#define RCC_APBRSTR1_DAC1RST             RCC_APBRSTR1_DAC1RST_Msk
#define RCC_APBRSTR1_LPTIM2RST_Pos       (30U)
#define RCC_APBRSTR1_LPTIM2RST_Msk       (0x1UL << RCC_APBRSTR1_LPTIM2RST_Pos)  /*!< 0x40000000 */
#define RCC_APBRSTR1_LPTIM2RST           RCC_APBRSTR1_LPTIM2RST_Msk
#define RCC_APBRSTR1_LPTIM1RST_Pos       (31U)
#define RCC_APBRSTR1_LPTIM1RST_Msk       (0x1UL << RCC_APBRSTR1_LPTIM1RST_Pos)  /*!< 0x80000000 */
#define RCC_APBRSTR1_LPTIM1RST           RCC_APBRSTR1_LPTIM1RST_Msk

/********************  Bit definition for RCC_APBRSTR2 register  **************/
#define RCC_APBRSTR2_SYSCFGRST_Pos       (0U)
#define RCC_APBRSTR2_SYSCFGRST_Msk       (0x1UL << RCC_APBRSTR2_SYSCFGRST_Pos)  /*!< 0x00000001 */
#define RCC_APBRSTR2_SYSCFGRST           RCC_APBRSTR2_SYSCFGRST_Msk
#define RCC_APBRSTR2_TIM1RST_Pos         (11U)
#define RCC_APBRSTR2_TIM1RST_Msk         (0x1UL << RCC_APBRSTR2_TIM1RST_Pos)    /*!< 0x00000800 */
#define RCC_APBRSTR2_TIM1RST             RCC_APBRSTR2_TIM1RST_Msk
#define RCC_APBRSTR2_SPI1RST_Pos         (12U)
#define RCC_APBRSTR2_SPI1RST_Msk         (0x1UL << RCC_APBRSTR2_SPI1RST_Pos)    /*!< 0x00001000 */
#define RCC_APBRSTR2_SPI1RST             RCC_APBRSTR2_SPI1RST_Msk
#define RCC_APBRSTR2_USART1RST_Pos       (14U)
#define RCC_APBRSTR2_USART1RST_Msk       (0x1UL << RCC_APBRSTR2_USART1RST_Pos)  /*!< 0x00004000 */
#define RCC_APBRSTR2_USART1RST           RCC_APBRSTR2_USART1RST_Msk
#define RCC_APBRSTR2_TIM14RST_Pos        (15U)
#define RCC_APBRSTR2_TIM14RST_Msk        (0x1UL << RCC_APBRSTR2_TIM14RST_Pos)   /*!< 0x00008000 */
#define RCC_APBRSTR2_TIM14RST            RCC_APBRSTR2_TIM14RST_Msk
#define RCC_APBRSTR2_TIM15RST_Pos        (16U)
#define RCC_APBRSTR2_TIM15RST_Msk        (0x1UL << RCC_APBRSTR2_TIM15RST_Pos)   /*!< 0x00010000 */
#define RCC_APBRSTR2_TIM15RST            RCC_APBRSTR2_TIM15RST_Msk
#define RCC_APBRSTR2_TIM16RST_Pos        (17U)
#define RCC_APBRSTR2_TIM16RST_Msk        (0x1UL << RCC_APBRSTR2_TIM16RST_Pos)   /*!< 0x00020000 */
#define RCC_APBRSTR2_TIM16RST            RCC_APBRSTR2_TIM16RST_Msk
#define RCC_APBRSTR2_TIM17RST_Pos        (18U)
#define RCC_APBRSTR2_TIM17RST_Msk        (0x1UL << RCC_APBRSTR2_TIM17RST_Pos)   /*!< 0x00040000 */
#define RCC_APBRSTR2_TIM17RST            RCC_APBRSTR2_TIM17RST_Msk
#define RCC_APBRSTR2_ADCRST_Pos          (20U)
#define RCC_APBRSTR2_ADCRST_Msk          (0x1UL << RCC_APBRSTR2_ADCRST_Pos)     /*!< 0x00100000 */
#define RCC_APBRSTR2_ADCRST              RCC_APBRSTR2_ADCRST_Msk

/********************  Bit definition for RCC_IOPENR register  ****************/
#define RCC_IOPENR_GPIOAEN_Pos           (0U)
#define RCC_IOPENR_GPIOAEN_Msk           (0x1UL << RCC_IOPENR_GPIOAEN_Pos)      /*!< 0x00000001 */
#define RCC_IOPENR_GPIOAEN               RCC_IOPENR_GPIOAEN_Msk
#define RCC_IOPENR_GPIOBEN_Pos           (1U)
#define RCC_IOPENR_GPIOBEN_Msk           (0x1UL << RCC_IOPENR_GPIOBEN_Pos)      /*!< 0x00000002 */
#define RCC_IOPENR_GPIOBEN               RCC_IOPENR_GPIOBEN_Msk
#define RCC_IOPENR_GPIOCEN_Pos           (2U)
#define RCC_IOPENR_GPIOCEN_Msk           (0x1UL << RCC_IOPENR_GPIOCEN_Pos)      /*!< 0x00000004 */
#define RCC_IOPENR_GPIOCEN               RCC_IOPENR_GPIOCEN_Msk
#define RCC_IOPENR_GPIODEN_Pos           (3U)
#define RCC_IOPENR_GPIODEN_Msk           (0x1UL << RCC_IOPENR_GPIODEN_Pos)      /*!< 0x00000008 */
#define RCC_IOPENR_GPIODEN               RCC_IOPENR_GPIODEN_Msk
#define RCC_IOPENR_GPIOFEN_Pos           (5U)
#define RCC_IOPENR_GPIOFEN_Msk           (0x1UL << RCC_IOPENR_GPIOFEN_Pos)      /*!< 0x00000020 */
#define RCC_IOPENR_GPIOFEN               RCC_IOPENR_GPIOFEN_Msk

/********************  Bit definition for RCC_AHBENR register  ****************/
#define RCC_AHBENR_DMA1EN_Pos            (0U)
#define RCC_AHBENR_DMA1EN_Msk            (0x1UL << RCC_AHBENR_DMA1EN_Pos)       /*!< 0x00000001 */
#define RCC_AHBENR_DMA1EN                RCC_AHBENR_DMA1EN_Msk
#define RCC_AHBENR_FLASHEN_Pos           (8U)
#define RCC_AHBENR_FLASHEN_Msk           (0x1UL << RCC_AHBENR_FLASHEN_Pos)      /*!< 0x00000100 */
#define RCC_AHBENR_FLASHEN               RCC_AHBENR_FLASHEN_Msk
#define RCC_AHBENR_CRCEN_Pos             (12U)
#define RCC_AHBENR_CRCEN_Msk             (0x1UL << RCC_AHBENR_CRCEN_Pos)        /*!< 0x00001000 */
#define RCC_AHBENR_CRCEN                 RCC_AHBENR_CRCEN_Msk
#define RCC_AHBENR_AESEN_Pos             (16U)
#define RCC_AHBENR_AESEN_Msk             (0x1UL << RCC_AHBENR_AESEN_Pos)        /*!< 0x00010000 */
#define RCC_AHBENR_AESEN                 RCC_AHBENR_AESEN_Msk
#define RCC_AHBENR_RNGEN_Pos             (18U)
#define RCC_AHBENR_RNGEN_Msk             (0x1UL << RCC_AHBENR_RNGEN_Pos)        /*!< 0x00040000 */
#define RCC_AHBENR_RNGEN                 RCC_AHBENR_RNGEN_Msk

/********************  Bit definition for RCC_APBENR1 register  ***************/
#define RCC_APBENR1_TIM2EN_Pos           (0U)
#define RCC_APBENR1_TIM2EN_Msk           (0x1UL << RCC_APBENR1_TIM2EN_Pos)      /*!< 0x00000001 */
#define RCC_APBENR1_TIM2EN               RCC_APBENR1_TIM2EN_Msk
#define RCC_APBENR1_TIM3EN_Pos           (1U)
#define RCC_APBENR1_TIM3EN_Msk           (0x1UL << RCC_APBENR1_TIM3EN_Pos)      /*!< 0x00000002 */
#define RCC_APBENR1_TIM3EN               RCC_APBENR1_TIM3EN_Msk
#define RCC_APBENR1_TIM6EN_Pos           (4U)
#define RCC_APBENR1_TIM6EN_Msk           (0x1UL << RCC_APBENR1_TIM6EN_Pos)      /*!< 0x00000010 */
#define RCC_APBENR1_TIM6EN               RCC_APBENR1_TIM6EN_Msk
#define RCC_APBENR1_TIM7EN_Pos           (5U)
#define RCC_APBENR1_TIM7EN_Msk           (0x1UL << RCC_APBENR1_TIM7EN_Pos)      /*!< 0x00000020 */
#define RCC_APBENR1_TIM7EN               RCC_APBENR1_TIM7EN_Msk
#define RCC_APBENR1_RTCAPBEN_Pos         (10U)
#define RCC_APBENR1_RTCAPBEN_Msk         (0x1UL << RCC_APBENR1_RTCAPBEN_Pos)    /*!< 0x00000400 */
#define RCC_APBENR1_RTCAPBEN             RCC_APBENR1_RTCAPBEN_Msk
#define RCC_APBENR1_WWDGEN_Pos           (11U)
#define RCC_APBENR1_WWDGEN_Msk           (0x1UL << RCC_APBENR1_WWDGEN_Pos)      /*!< 0x00000800 */
#define RCC_APBENR1_WWDGEN               RCC_APBENR1_WWDGEN_Msk
#define RCC_APBENR1_SPI2EN_Pos           (14U)
#define RCC_APBENR1_SPI2EN_Msk           (0x1UL << RCC_APBENR1_SPI2EN_Pos)      /*!< 0x00004000 */
#define RCC_APBENR1_SPI2EN               RCC_APBENR1_SPI2EN_Msk
#define RCC_APBENR1_USART2EN_Pos         (17U)
#define RCC_APBENR1_USART2EN_Msk         (0x1UL << RCC_APBENR1_USART2EN_Pos)    /*!< 0x00020000 */
#define RCC_APBENR1_USART2EN             RCC_APBENR1_USART2EN_Msk
#define RCC_APBENR1_USART3EN_Pos         (18U)
#define RCC_APBENR1_USART3EN_Msk         (0x1UL << RCC_APBENR1_USART3EN_Pos)    /*!< 0x00040000 */
#define RCC_APBENR1_USART3EN             RCC_APBENR1_USART3EN_Msk
#define RCC_APBENR1_USART4EN_Pos         (19U)
#define RCC_APBENR1_USART4EN_Msk         (0x1UL << RCC_APBENR1_USART4EN_Pos)    /*!< 0x00080000 */
#define RCC_APBENR1_USART4EN             RCC_APBENR1_USART4EN_Msk
#define RCC_APBENR1_LPUART1EN_Pos        (20U)
#define RCC_APBENR1_LPUART1EN_Msk        (0x1UL << RCC_APBENR1_LPUART1EN_Pos)   /*!< 0x00100000 */
#define RCC_APBENR1_LPUART1EN            RCC_APBENR1_LPUART1EN_Msk
#define RCC_APBENR1_I2C1EN_Pos           (21U)
#define RCC_APBENR1_I2C1EN_Msk           (0x1UL << RCC_APBENR1_I2C1EN_Pos)      /*!< 0x00200000 */
#define RCC_APBENR1_I2C1EN               RCC_APBENR1_I2C1EN_Msk
#define RCC_APBENR1_I2C2EN_Pos           (22U)
#define RCC_APBENR1_I2C2EN_Msk           (0x1UL << RCC_APBENR1_I2C2EN_Pos)      /*!< 0x00400000 */
#define RCC_APBENR1_I2C2EN               RCC_APBENR1_I2C2EN_Msk
#define RCC_APBENR1_CECEN_Pos            (24U)
#define RCC_APBENR1_CECEN_Msk            (0x1UL << RCC_APBENR1_CECEN_Pos)       /*!< 0x01000000 */
#define RCC_APBENR1_CECEN                RCC_APBENR1_CECEN_Msk
#define RCC_APBENR1_UCPD1EN_Pos          (25U)
#define RCC_APBENR1_UCPD1EN_Msk          (0x1UL << RCC_APBENR1_UCPD1EN_Pos)     /*!< 0x02000000 */
#define RCC_APBENR1_UCPD1EN              RCC_APBENR1_UCPD1EN_Msk
#define RCC_APBENR1_UCPD2EN_Pos          (26U)
#define RCC_APBENR1_UCPD2EN_Msk          (0x1UL << RCC_APBENR1_UCPD2EN_Pos)     /*!< 0x04000000 */
#define RCC_APBENR1_UCPD2EN              RCC_APBENR1_UCPD2EN_Msk
#define RCC_APBENR1_DBGEN_Pos            (27U)
#define RCC_APBENR1_DBGEN_Msk            (0x1UL << RCC_APBENR1_DBGEN_Pos)       /*!< 0x08000000 */
#define RCC_APBENR1_DBGEN                RCC_APBENR1_DBGEN_Msk
#define RCC_APBENR1_PWREN_Pos            (28U)
#define RCC_APBENR1_PWREN_Msk            (0x1UL << RCC_APBENR1_PWREN_Pos)       /*!< 0x10000000 */
#define RCC_APBENR1_PWREN                RCC_APBENR1_PWREN_Msk
#define RCC_APBENR1_DAC1EN_Pos           (29U)
#define RCC_APBENR1_DAC1EN_Msk           (0x1UL << RCC_APBENR1_DAC1EN_Pos)      /*!< 0x20000000 */
#define RCC_APBENR1_DAC1EN               RCC_APBENR1_DAC1EN_Msk
#define RCC_APBENR1_LPTIM2EN_Pos         (30U)
#define RCC_APBENR1_LPTIM2EN_Msk         (0x1UL << RCC_APBENR1_LPTIM2EN_Pos)    /*!< 0x40000000 */
#define RCC_APBENR1_LPTIM2EN             RCC_APBENR1_LPTIM2EN_Msk
#define RCC_APBENR1_LPTIM1EN_Pos         (31U)
#define RCC_APBENR1_LPTIM1EN_Msk         (0x1UL << RCC_APBENR1_LPTIM1EN_Pos)    /*!< 0x80000000 */
#define RCC_APBENR1_LPTIM1EN             RCC_APBENR1_LPTIM1EN_Msk

/********************  Bit definition for RCC_APBENR2 register  **************/
#define RCC_APBENR2_SYSCFGEN_Pos         (0U)
#define RCC_APBENR2_SYSCFGEN_Msk         (0x1UL << RCC_APBENR2_SYSCFGEN_Pos)    /*!< 0x00000001 */
#define RCC_APBENR2_SYSCFGEN             RCC_APBENR2_SYSCFGEN_Msk
#define RCC_APBENR2_TIM1EN_Pos           (11U)
#define RCC_APBENR2_TIM1EN_Msk           (0x1UL << RCC_APBENR2_TIM1EN_Pos)      /*!< 0x00000800 */
#define RCC_APBENR2_TIM1EN               RCC_APBENR2_TIM1EN_Msk
#define RCC_APBENR2_SPI1EN_Pos           (12U)
#define RCC_APBENR2_SPI1EN_Msk           (0x1UL << RCC_APBENR2_SPI1EN_Pos)      /*!< 0x00001000 */
#define RCC_APBENR2_SPI1EN               RCC_APBENR2_SPI1EN_Msk
#define RCC_APBENR2_USART1EN_Pos         (14U)
#define RCC_APBENR2_USART1EN_Msk         (0x1UL << RCC_APBENR2_USART1EN_Pos)    /*!< 0x00004000 */
#define RCC_APBENR2_USART1EN             RCC_APBENR2_USART1EN_Msk
#define RCC_APBENR2_TIM14EN_Pos          (15U)
#define RCC_APBENR2_TIM14EN_Msk          (0x1UL << RCC_APBENR2_TIM14EN_Pos)     /*!< 0x00008000 */
#define RCC_APBENR2_TIM14EN              RCC_APBENR2_TIM14EN_Msk
#define RCC_APBENR2_TIM15EN_Pos          (16U)
#define RCC_APBENR2_TIM15EN_Msk          (0x1UL << RCC_APBENR2_TIM15EN_Pos)     /*!< 0x00010000 */
#define RCC_APBENR2_TIM15EN              RCC_APBENR2_TIM15EN_Msk
#define RCC_APBENR2_TIM16EN_Pos          (17U)
#define RCC_APBENR2_TIM16EN_Msk          (0x1UL << RCC_APBENR2_TIM16EN_Pos)     /*!< 0x00020000 */
#define RCC_APBENR2_TIM16EN              RCC_APBENR2_TIM16EN_Msk
#define RCC_APBENR2_TIM17EN_Pos          (18U)
#define RCC_APBENR2_TIM17EN_Msk          (0x1UL << RCC_APBENR2_TIM17EN_Pos)     /*!< 0x00040000 */
#define RCC_APBENR2_TIM17EN              RCC_APBENR2_TIM17EN_Msk
#define RCC_APBENR2_ADCEN_Pos            (20U)
#define RCC_APBENR2_ADCEN_Msk            (0x1UL << RCC_APBENR2_ADCEN_Pos)       /*!< 0x00100000 */
#define RCC_APBENR2_ADCEN                RCC_APBENR2_ADCEN_Msk

/********************  Bit definition for RCC_IOPSMENR register  *************/
#define RCC_IOPSMENR_GPIOASMEN_Pos       (0U)
#define RCC_IOPSMENR_GPIOASMEN_Msk       (0x1UL << RCC_IOPSMENR_GPIOASMEN_Pos)  /*!< 0x00000001 */
#define RCC_IOPSMENR_GPIOASMEN           RCC_IOPSMENR_GPIOASMEN_Msk
#define RCC_IOPSMENR_GPIOBSMEN_Pos       (1U)
#define RCC_IOPSMENR_GPIOBSMEN_Msk       (0x1UL << RCC_IOPSMENR_GPIOBSMEN_Pos)  /*!< 0x00000002 */
#define RCC_IOPSMENR_GPIOBSMEN           RCC_IOPSMENR_GPIOBSMEN_Msk
#define RCC_IOPSMENR_GPIOCSMEN_Pos       (2U)
#define RCC_IOPSMENR_GPIOCSMEN_Msk       (0x1UL << RCC_IOPSMENR_GPIOCSMEN_Pos)  /*!< 0x00000004 */
#define RCC_IOPSMENR_GPIOCSMEN           RCC_IOPSMENR_GPIOCSMEN_Msk
#define RCC_IOPSMENR_GPIODSMEN_Pos       (3U)
#define RCC_IOPSMENR_GPIODSMEN_Msk       (0x1UL << RCC_IOPSMENR_GPIODSMEN_Pos)  /*!< 0x00000008 */
#define RCC_IOPSMENR_GPIODSMEN           RCC_IOPSMENR_GPIODSMEN_Msk
#define RCC_IOPSMENR_GPIOFSMEN_Pos       (5U)
#define RCC_IOPSMENR_GPIOFSMEN_Msk       (0x1UL << RCC_IOPSMENR_GPIOFSMEN_Pos)  /*!< 0x00000020 */
#define RCC_IOPSMENR_GPIOFSMEN           RCC_IOPSMENR_GPIOFSMEN_Msk

/********************  Bit definition for RCC_AHBSMENR register  *************/
#define RCC_AHBSMENR_DMA1SMEN_Pos        (0U)
#define RCC_AHBSMENR_DMA1SMEN_Msk        (0x1UL << RCC_AHBSMENR_DMA1SMEN_Pos)   /*!< 0x00000001 */
#define RCC_AHBSMENR_DMA1SMEN            RCC_AHBSMENR_DMA1SMEN_Msk
#define RCC_AHBSMENR_FLASHSMEN_Pos       (8U)
#define RCC_AHBSMENR_FLASHSMEN_Msk       (0x1UL << RCC_AHBSMENR_FLASHSMEN_Pos)  /*!< 0x00000100 */
#define RCC_AHBSMENR_FLASHSMEN           RCC_AHBSMENR_FLASHSMEN_Msk
#define RCC_AHBSMENR_SRAMSMEN_Pos        (9U)
#define RCC_AHBSMENR_SRAMSMEN_Msk        (0x1UL << RCC_AHBSMENR_SRAMSMEN_Pos)   /*!< 0x00000200 */
#define RCC_AHBSMENR_SRAMSMEN            RCC_AHBSMENR_SRAMSMEN_Msk
#define RCC_AHBSMENR_CRCSMEN_Pos         (12U)
#define RCC_AHBSMENR_CRCSMEN_Msk         (0x1UL << RCC_AHBSMENR_CRCSMEN_Pos)    /*!< 0x00001000 */
#define RCC_AHBSMENR_CRCSMEN             RCC_AHBSMENR_CRCSMEN_Msk
#define RCC_AHBSMENR_AESSMEN_Pos         (16U)
#define RCC_AHBSMENR_AESSMEN_Msk         (0x1UL << RCC_AHBSMENR_AESSMEN_Pos)    /*!< 0x00010000 */
#define RCC_AHBSMENR_AESSMEN             RCC_AHBSMENR_AESSMEN_Msk
#define RCC_AHBSMENR_RNGSMEN_Pos         (18U)
#define RCC_AHBSMENR_RNGSMEN_Msk         (0x1UL << RCC_AHBSMENR_RNGSMEN_Pos)    /*!< 0x00040000 */
#define RCC_AHBSMENR_RNGSMEN             RCC_AHBSMENR_RNGSMEN_Msk

/********************  Bit definition for RCC_APBSMENR1 register  *************/
#define RCC_APBSMENR1_TIM2SMEN_Pos       (0U)
#define RCC_APBSMENR1_TIM2SMEN_Msk       (0x1UL << RCC_APBSMENR1_TIM2SMEN_Pos)  /*!< 0x00000001 */
#define RCC_APBSMENR1_TIM2SMEN           RCC_APBSMENR1_TIM2SMEN_Msk
#define RCC_APBSMENR1_TIM3SMEN_Pos       (1U)
#define RCC_APBSMENR1_TIM3SMEN_Msk       (0x1UL << RCC_APBSMENR1_TIM3SMEN_Pos)  /*!< 0x00000002 */
#define RCC_APBSMENR1_TIM3SMEN           RCC_APBSMENR1_TIM3SMEN_Msk
#define RCC_APBSMENR1_TIM6SMEN_Pos       (4U)
#define RCC_APBSMENR1_TIM6SMEN_Msk       (0x1UL << RCC_APBSMENR1_TIM6SMEN_Pos)  /*!< 0x00000010 */
#define RCC_APBSMENR1_TIM6SMEN           RCC_APBSMENR1_TIM6SMEN_Msk
#define RCC_APBSMENR1_TIM7SMEN_Pos       (5U)
#define RCC_APBSMENR1_TIM7SMEN_Msk       (0x1UL << RCC_APBSMENR1_TIM7SMEN_Pos)  /*!< 0x00000020 */
#define RCC_APBSMENR1_TIM7SMEN           RCC_APBSMENR1_TIM7SMEN_Msk
#define RCC_APBSMENR1_RTCAPBSMEN_Pos     (10U)
#define RCC_APBSMENR1_RTCAPBSMEN_Msk     (0x1UL << RCC_APBSMENR1_RTCAPBSMEN_Pos) /*!< 0x00000400 */
#define RCC_APBSMENR1_RTCAPBSMEN         RCC_APBSMENR1_RTCAPBSMEN_Msk
#define RCC_APBSMENR1_WWDGSMEN_Pos       (11U)
#define RCC_APBSMENR1_WWDGSMEN_Msk       (0x1UL << RCC_APBSMENR1_WWDGSMEN_Pos)  /*!< 0x00000800 */
#define RCC_APBSMENR1_WWDGSMEN           RCC_APBSMENR1_WWDGSMEN_Msk
#define RCC_APBSMENR1_SPI2SMEN_Pos       (14U)
#define RCC_APBSMENR1_SPI2SMEN_Msk       (0x1UL << RCC_APBSMENR1_SPI2SMEN_Pos)  /*!< 0x00004000 */
#define RCC_APBSMENR1_SPI2SMEN           RCC_APBSMENR1_SPI2SMEN_Msk
#define RCC_APBSMENR1_USART2SMEN_Pos     (17U)
#define RCC_APBSMENR1_USART2SMEN_Msk     (0x1UL << RCC_APBSMENR1_USART2SMEN_Pos) /*!< 0x00020000 */
#define RCC_APBSMENR1_USART2SMEN         RCC_APBSMENR1_USART2SMEN_Msk
#define RCC_APBSMENR1_USART3SMEN_Pos     (18U)
#define RCC_APBSMENR1_USART3SMEN_Msk     (0x1UL << RCC_APBSMENR1_USART3SMEN_Pos) /*!< 0x00040000 */
#define RCC_APBSMENR1_USART3SMEN         RCC_APBSMENR1_USART3SMEN_Msk
#define RCC_APBSMENR1_USART4SMEN_Pos     (19U)
#define RCC_APBSMENR1_USART4SMEN_Msk     (0x1UL << RCC_APBSMENR1_USART4SMEN_Pos) /*!< 0x00080000 */
#define RCC_APBSMENR1_USART4SMEN         RCC_APBSMENR1_USART4SMEN_Msk
#define RCC_APBSMENR1_LPUART1SMEN_Pos    (20U)
#define RCC_APBSMENR1_LPUART1SMEN_Msk    (0x1UL << RCC_APBSMENR1_LPUART1SMEN_Pos) /*!< 0x00100000 */
#define RCC_APBSMENR1_LPUART1SMEN        RCC_APBSMENR1_LPUART1SMEN_Msk
#define RCC_APBSMENR1_I2C1SMEN_Pos       (21U)
#define RCC_APBSMENR1_I2C1SMEN_Msk       (0x1UL << RCC_APBSMENR1_I2C1SMEN_Pos)   /*!< 0x00200000 */
#define RCC_APBSMENR1_I2C1SMEN           RCC_APBSMENR1_I2C1SMEN_Msk
#define RCC_APBSMENR1_I2C2SMEN_Pos       (22U)
#define RCC_APBSMENR1_I2C2SMEN_Msk       (0x1UL << RCC_APBSMENR1_I2C2SMEN_Pos)   /*!< 0x00400000 */
#define RCC_APBSMENR1_I2C2SMEN           RCC_APBSMENR1_I2C2SMEN_Msk
#define RCC_APBSMENR1_CECSMEN_Pos        (24U)
#define RCC_APBSMENR1_CECSMEN_Msk        (0x1UL << RCC_APBSMENR1_CECSMEN_Pos)    /*!< 0x01000000 */
#define RCC_APBSMENR1_CECSMEN            RCC_APBSMENR1_CECSMEN_Msk
#define RCC_APBSMENR1_UCPD1SMEN_Pos      (25U)
#define RCC_APBSMENR1_UCPD1SMEN_Msk      (0x1UL << RCC_APBSMENR1_UCPD1SMEN_Pos)  /*!< 0x02000000 */
#define RCC_APBSMENR1_UCPD1SMEN          RCC_APBSMENR1_UCPD1SMEN_Msk
#define RCC_APBSMENR1_UCPD2SMEN_Pos      (26U)
#define RCC_APBSMENR1_UCPD2SMEN_Msk      (0x1UL << RCC_APBSMENR1_UCPD2SMEN_Pos)  /*!< 0x04000000 */
#define RCC_APBSMENR1_UCPD2SMEN          RCC_APBSMENR1_UCPD2SMEN_Msk
#define RCC_APBSMENR1_DBGSMEN_Pos        (27U)
#define RCC_APBSMENR1_DBGSMEN_Msk        (0x1UL << RCC_APBSMENR1_DBGSMEN_Pos)    /*!< 0x08000000 */
#define RCC_APBSMENR1_DBGSMEN            RCC_APBSMENR1_DBGSMEN_Msk
#define RCC_APBSMENR1_PWRSMEN_Pos        (28U)
#define RCC_APBSMENR1_PWRSMEN_Msk        (0x1UL << RCC_APBSMENR1_PWRSMEN_Pos)    /*!< 0x10000000 */
#define RCC_APBSMENR1_PWRSMEN            RCC_APBSMENR1_PWRSMEN_Msk
#define RCC_APBSMENR1_DAC1SMEN_Pos       (29U)
#define RCC_APBSMENR1_DAC1SMEN_Msk       (0x1UL << RCC_APBSMENR1_DAC1SMEN_Pos)   /*!< 0x20000000 */
#define RCC_APBSMENR1_DAC1SMEN           RCC_APBSMENR1_DAC1SMEN_Msk
#define RCC_APBSMENR1_LPTIM2SMEN_Pos     (30U)
#define RCC_APBSMENR1_LPTIM2SMEN_Msk     (0x1UL << RCC_APBSMENR1_LPTIM2SMEN_Pos) /*!< 0x40000000 */
#define RCC_APBSMENR1_LPTIM2SMEN         RCC_APBSMENR1_LPTIM2SMEN_Msk
#define RCC_APBSMENR1_LPTIM1SMEN_Pos     (31U)
#define RCC_APBSMENR1_LPTIM1SMEN_Msk     (0x1UL << RCC_APBSMENR1_LPTIM1SMEN_Pos) /*!< 0x80000000 */
#define RCC_APBSMENR1_LPTIM1SMEN         RCC_APBSMENR1_LPTIM1SMEN_Msk

/********************  Bit definition for RCC_APBSMENR2 register  *************/
#define RCC_APBSMENR2_SYSCFGSMEN_Pos     (0U)
#define RCC_APBSMENR2_SYSCFGSMEN_Msk     (0x1UL << RCC_APBSMENR2_SYSCFGSMEN_Pos) /*!< 0x00000001 */
#define RCC_APBSMENR2_SYSCFGSMEN         RCC_APBSMENR2_SYSCFGSMEN_Msk
#define RCC_APBSMENR2_TIM1SMEN_Pos       (11U)
#define RCC_APBSMENR2_TIM1SMEN_Msk       (0x1UL << RCC_APBSMENR2_TIM1SMEN_Pos)  /*!< 0x00000800 */
#define RCC_APBSMENR2_TIM1SMEN           RCC_APBSMENR2_TIM1SMEN_Msk
#define RCC_APBSMENR2_SPI1SMEN_Pos       (12U)
#define RCC_APBSMENR2_SPI1SMEN_Msk       (0x1UL << RCC_APBSMENR2_SPI1SMEN_Pos)  /*!< 0x00001000 */
#define RCC_APBSMENR2_SPI1SMEN           RCC_APBSMENR2_SPI1SMEN_Msk
#define RCC_APBSMENR2_USART1SMEN_Pos     (14U)
#define RCC_APBSMENR2_USART1SMEN_Msk     (0x1UL << RCC_APBSMENR2_USART1SMEN_Pos) /*!< 0x00004000 */
#define RCC_APBSMENR2_USART1SMEN         RCC_APBSMENR2_USART1SMEN_Msk
#define RCC_APBSMENR2_TIM14SMEN_Pos      (15U)
#define RCC_APBSMENR2_TIM14SMEN_Msk      (0x1UL << RCC_APBSMENR2_TIM14SMEN_Pos) /*!< 0x00008000 */
#define RCC_APBSMENR2_TIM14SMEN          RCC_APBSMENR2_TIM14SMEN_Msk
#define RCC_APBSMENR2_TIM15SMEN_Pos      (16U)
#define RCC_APBSMENR2_TIM15SMEN_Msk      (0x1UL << RCC_APBSMENR2_TIM15SMEN_Pos) /*!< 0x00010000 */
#define RCC_APBSMENR2_TIM15SMEN          RCC_APBSMENR2_TIM15SMEN_Msk
#define RCC_APBSMENR2_TIM16SMEN_Pos      (17U)
#define RCC_APBSMENR2_TIM16SMEN_Msk      (0x1UL << RCC_APBSMENR2_TIM16SMEN_Pos) /*!< 0x00020000 */
#define RCC_APBSMENR2_TIM16SMEN          RCC_APBSMENR2_TIM16SMEN_Msk
#define RCC_APBSMENR2_TIM17SMEN_Pos      (18U)
#define RCC_APBSMENR2_TIM17SMEN_Msk      (0x1UL << RCC_APBSMENR2_TIM17SMEN_Pos) /*!< 0x00040000 */
#define RCC_APBSMENR2_TIM17SMEN          RCC_APBSMENR2_TIM17SMEN_Msk
#define RCC_APBSMENR2_ADCSMEN_Pos        (20U)
#define RCC_APBSMENR2_ADCSMEN_Msk        (0x1UL << RCC_APBSMENR2_ADCSMEN_Pos)   /*!< 0x00100000 */
#define RCC_APBSMENR2_ADCSMEN            RCC_APBSMENR2_ADCSMEN_Msk

/********************  Bit definition for RCC_CCIPR register  ******************/
#define RCC_CCIPR_USART1SEL_Pos          (0U)
#define RCC_CCIPR_USART1SEL_Msk          (0x3UL << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000003 */
#define RCC_CCIPR_USART1SEL              RCC_CCIPR_USART1SEL_Msk
#define RCC_CCIPR_USART1SEL_0            (0x1UL << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000001 */
#define RCC_CCIPR_USART1SEL_1            (0x2UL << RCC_CCIPR_USART1SEL_Pos)     /*!< 0x00000002 */

#define RCC_CCIPR_USART2SEL_Pos          (2U)
#define RCC_CCIPR_USART2SEL_Msk          (0x3UL << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x0000000C */
#define RCC_CCIPR_USART2SEL              RCC_CCIPR_USART2SEL_Msk
#define RCC_CCIPR_USART2SEL_0            (0x1UL << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x00000004 */
#define RCC_CCIPR_USART2SEL_1            (0x2UL << RCC_CCIPR_USART2SEL_Pos)     /*!< 0x00000008 */

#define RCC_CCIPR_CECSEL_Pos             (6U)
#define RCC_CCIPR_CECSEL_Msk             (0x1UL << RCC_CCIPR_CECSEL_Pos)        /*!< 0x00000040 */
#define RCC_CCIPR_CECSEL                 RCC_CCIPR_CECSEL_Msk

#define RCC_CCIPR_LPUART1SEL_Pos         (10U)
#define RCC_CCIPR_LPUART1SEL_Msk         (0x3UL << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x00000C00 */
#define RCC_CCIPR_LPUART1SEL             RCC_CCIPR_LPUART1SEL_Msk
#define RCC_CCIPR_LPUART1SEL_0           (0x1UL << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x00000400 */
#define RCC_CCIPR_LPUART1SEL_1           (0x2UL << RCC_CCIPR_LPUART1SEL_Pos)    /*!< 0x00000800 */

#define RCC_CCIPR_I2C1SEL_Pos            (12U)
#define RCC_CCIPR_I2C1SEL_Msk            (0x3UL << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00003000 */
#define RCC_CCIPR_I2C1SEL                RCC_CCIPR_I2C1SEL_Msk
#define RCC_CCIPR_I2C1SEL_0              (0x1UL << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00001000 */
#define RCC_CCIPR_I2C1SEL_1              (0x2UL << RCC_CCIPR_I2C1SEL_Pos)       /*!< 0x00002000 */

#define RCC_CCIPR_I2S1SEL_Pos            (14U)
#define RCC_CCIPR_I2S1SEL_Msk            (0x3UL << RCC_CCIPR_I2S1SEL_Pos)       /*!< 0x0000C000 */
#define RCC_CCIPR_I2S1SEL                RCC_CCIPR_I2S1SEL_Msk
#define RCC_CCIPR_I2S1SEL_0              (0x1UL << RCC_CCIPR_I2S1SEL_Pos)       /*!< 0x00004000 */
#define RCC_CCIPR_I2S1SEL_1              (0x2UL << RCC_CCIPR_I2S1SEL_Pos)       /*!< 0x00008000 */

#define RCC_CCIPR_LPTIM1SEL_Pos          (18U)
#define RCC_CCIPR_LPTIM1SEL_Msk          (0x3UL << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x000C0000 */
#define RCC_CCIPR_LPTIM1SEL              RCC_CCIPR_LPTIM1SEL_Msk
#define RCC_CCIPR_LPTIM1SEL_0            (0x1UL << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x00040000 */
#define RCC_CCIPR_LPTIM1SEL_1            (0x2UL << RCC_CCIPR_LPTIM1SEL_Pos)     /*!< 0x00080000 */

#define RCC_CCIPR_LPTIM2SEL_Pos          (20U)
#define RCC_CCIPR_LPTIM2SEL_Msk          (0x3UL << RCC_CCIPR_LPTIM2SEL_Pos)     /*!< 0x00300000 */
#define RCC_CCIPR_LPTIM2SEL              RCC_CCIPR_LPTIM2SEL_Msk
#define RCC_CCIPR_LPTIM2SEL_0            (0x1UL << RCC_CCIPR_LPTIM2SEL_Pos)     /*!< 0x00100000 */
#define RCC_CCIPR_LPTIM2SEL_1            (0x2UL << RCC_CCIPR_LPTIM2SEL_Pos)     /*!< 0x00200000 */

#define RCC_CCIPR_TIM1SEL_Pos            (22U)
#define RCC_CCIPR_TIM1SEL_Msk            (0x1UL << RCC_CCIPR_TIM1SEL_Pos)       /*!< 0x00400000 */
#define RCC_CCIPR_TIM1SEL                RCC_CCIPR_TIM1SEL_Msk

#define RCC_CCIPR_TIM15SEL_Pos           (24U)
#define RCC_CCIPR_TIM15SEL_Msk           (0x1UL << RCC_CCIPR_TIM15SEL_Pos)      /*!< 0x01000000 */
#define RCC_CCIPR_TIM15SEL               RCC_CCIPR_TIM15SEL_Msk

#define RCC_CCIPR_RNGSEL_Pos             (26U)
#define RCC_CCIPR_RNGSEL_Msk             (0x3UL << RCC_CCIPR_RNGSEL_Pos)        /*!< 0x0C000000 */
#define RCC_CCIPR_RNGSEL                 RCC_CCIPR_RNGSEL_Msk
#define RCC_CCIPR_RNGSEL_0               (0x1UL << RCC_CCIPR_RNGSEL_Pos)        /*!< 0x04000000 */
#define RCC_CCIPR_RNGSEL_1               (0x2UL << RCC_CCIPR_RNGSEL_Pos)        /*!< 0x08000000 */

#define RCC_CCIPR_RNGDIV_Pos             (28U)
#define RCC_CCIPR_RNGDIV_Msk             (0x3UL << RCC_CCIPR_RNGDIV_Pos)        /*!< 0x30000000 */
#define RCC_CCIPR_RNGDIV                 RCC_CCIPR_RNGDIV_Msk
#define RCC_CCIPR_RNGDIV_0               (0x1UL << RCC_CCIPR_RNGDIV_Pos)        /*!< 0x10000000 */
#define RCC_CCIPR_RNGDIV_1               (0x2UL << RCC_CCIPR_RNGDIV_Pos)        /*!< 0x20000000 */

#define RCC_CCIPR_ADCSEL_Pos             (30U)
#define RCC_CCIPR_ADCSEL_Msk             (0x3UL << RCC_CCIPR_ADCSEL_Pos)        /*!< 0xC0000000 */
#define RCC_CCIPR_ADCSEL                 RCC_CCIPR_ADCSEL_Msk
#define RCC_CCIPR_ADCSEL_0               (0x1UL << RCC_CCIPR_ADCSEL_Pos)        /*!< 0x40000000 */
#define RCC_CCIPR_ADCSEL_1               (0x2UL << RCC_CCIPR_ADCSEL_Pos)        /*!< 0x80000000 */

/********************  Bit definition for RCC_BDCR register  ******************/
#define RCC_BDCR_LSEON_Pos               (0U)
#define RCC_BDCR_LSEON_Msk               (0x1UL << RCC_BDCR_LSEON_Pos)          /*!< 0x00000001 */
#define RCC_BDCR_LSEON                   RCC_BDCR_LSEON_Msk
#define RCC_BDCR_LSERDY_Pos              (1U)
#define RCC_BDCR_LSERDY_Msk              (0x1UL << RCC_BDCR_LSERDY_Pos)         /*!< 0x00000002 */
#define RCC_BDCR_LSERDY                  RCC_BDCR_LSERDY_Msk
#define RCC_BDCR_LSEBYP_Pos              (2U)
#define RCC_BDCR_LSEBYP_Msk              (0x1UL << RCC_BDCR_LSEBYP_Pos)         /*!< 0x00000004 */
#define RCC_BDCR_LSEBYP                  RCC_BDCR_LSEBYP_Msk

#define RCC_BDCR_LSEDRV_Pos              (3U)
#define RCC_BDCR_LSEDRV_Msk              (0x3UL << RCC_BDCR_LSEDRV_Pos)         /*!< 0x00000018 */
#define RCC_BDCR_LSEDRV                  RCC_BDCR_LSEDRV_Msk
#define RCC_BDCR_LSEDRV_0                (0x1UL << RCC_BDCR_LSEDRV_Pos)         /*!< 0x00000008 */
#define RCC_BDCR_LSEDRV_1                (0x2UL << RCC_BDCR_LSEDRV_Pos)         /*!< 0x00000010 */

#define RCC_BDCR_LSECSSON_Pos            (5U)
#define RCC_BDCR_LSECSSON_Msk            (0x1UL << RCC_BDCR_LSECSSON_Pos)       /*!< 0x00000020 */
#define RCC_BDCR_LSECSSON                RCC_BDCR_LSECSSON_Msk
#define RCC_BDCR_LSECSSD_Pos             (6U)
#define RCC_BDCR_LSECSSD_Msk             (0x1UL << RCC_BDCR_LSECSSD_Pos)        /*!< 0x00000040 */
#define RCC_BDCR_LSECSSD                 RCC_BDCR_LSECSSD_Msk

#define RCC_BDCR_RTCSEL_Pos              (8U)
#define RCC_BDCR_RTCSEL_Msk              (0x3UL << RCC_BDCR_RTCSEL_Pos)         /*!< 0x00000300 */
#define RCC_BDCR_RTCSEL                  RCC_BDCR_RTCSEL_Msk
#define RCC_BDCR_RTCSEL_0                (0x1UL << RCC_BDCR_RTCSEL_Pos)         /*!< 0x00000100 */
#define RCC_BDCR_RTCSEL_1                (0x2UL << RCC_BDCR_RTCSEL_Pos)         /*!< 0x00000200 */

#define RCC_BDCR_RTCEN_Pos               (15U)
#define RCC_BDCR_RTCEN_Msk               (0x1UL << RCC_BDCR_RTCEN_Pos)          /*!< 0x00008000 */
#define RCC_BDCR_RTCEN                   RCC_BDCR_RTCEN_Msk
#define RCC_BDCR_BDRST_Pos               (16U)
#define RCC_BDCR_BDRST_Msk               (0x1UL << RCC_BDCR_BDRST_Pos)          /*!< 0x00010000 */
#define RCC_BDCR_BDRST                   RCC_BDCR_BDRST_Msk

#define RCC_BDCR_LSCOEN_Pos              (24U)
#define RCC_BDCR_LSCOEN_Msk              (0x1UL << RCC_BDCR_LSCOEN_Pos)         /*!< 0x01000000 */
#define RCC_BDCR_LSCOEN                  RCC_BDCR_LSCOEN_Msk
#define RCC_BDCR_LSCOSEL_Pos             (25U)
#define RCC_BDCR_LSCOSEL_Msk             (0x1UL << RCC_BDCR_LSCOSEL_Pos)        /*!< 0x02000000 */
#define RCC_BDCR_LSCOSEL                 RCC_BDCR_LSCOSEL_Msk

/********************  Bit definition for RCC_CSR register  *******************/
#define RCC_CSR_LSION_Pos                (0U)
#define RCC_CSR_LSION_Msk                (0x1UL << RCC_CSR_LSION_Pos)           /*!< 0x00000001 */
#define RCC_CSR_LSION                    RCC_CSR_LSION_Msk
#define RCC_CSR_LSIRDY_Pos               (1U)
#define RCC_CSR_LSIRDY_Msk               (0x1UL << RCC_CSR_LSIRDY_Pos)          /*!< 0x00000002 */
#define RCC_CSR_LSIRDY                   RCC_CSR_LSIRDY_Msk

#define RCC_CSR_RMVF_Pos                 (23U)
#define RCC_CSR_RMVF_Msk                 (0x1UL << RCC_CSR_RMVF_Pos)            /*!< 0x00800000 */
#define RCC_CSR_RMVF                     RCC_CSR_RMVF_Msk
#define RCC_CSR_OBLRSTF_Pos              (25U)
#define RCC_CSR_OBLRSTF_Msk              (0x1UL << RCC_CSR_OBLRSTF_Pos)         /*!< 0x02000000 */
#define RCC_CSR_OBLRSTF                  RCC_CSR_OBLRSTF_Msk
#define RCC_CSR_PINRSTF_Pos              (26U)
#define RCC_CSR_PINRSTF_Msk              (0x1UL << RCC_CSR_PINRSTF_Pos)         /*!< 0x04000000 */
#define RCC_CSR_PINRSTF                  RCC_CSR_PINRSTF_Msk
#define RCC_CSR_PWRRSTF_Pos              (27U)
#define RCC_CSR_PWRRSTF_Msk              (0x1UL << RCC_CSR_PWRRSTF_Pos)         /*!< 0x08000000 */
#define RCC_CSR_PWRRSTF                  RCC_CSR_PWRRSTF_Msk
#define RCC_CSR_SFTRSTF_Pos              (28U)
#define RCC_CSR_SFTRSTF_Msk              (0x1UL << RCC_CSR_SFTRSTF_Pos)         /*!< 0x10000000 */
#define RCC_CSR_SFTRSTF                  RCC_CSR_SFTRSTF_Msk
#define RCC_CSR_IWDGRSTF_Pos             (29U)
#define RCC_CSR_IWDGRSTF_Msk             (0x1UL << RCC_CSR_IWDGRSTF_Pos)        /*!< 0x20000000 */
#define RCC_CSR_IWDGRSTF                 RCC_CSR_IWDGRSTF_Msk
#define RCC_CSR_WWDGRSTF_Pos             (30U)
#define RCC_CSR_WWDGRSTF_Msk             (0x1UL << RCC_CSR_WWDGRSTF_Pos)        /*!< 0x40000000 */
#define RCC_CSR_WWDGRSTF                 RCC_CSR_WWDGRSTF_Msk
#define RCC_CSR_LPWRRSTF_Pos             (31U)
#define RCC_CSR_LPWRRSTF_Msk             (0x1UL << RCC_CSR_LPWRRSTF_Pos)        /*!< 0x80000000 */
#define RCC_CSR_LPWRRSTF                 RCC_CSR_LPWRRSTF_Msk

/******************************************************************************/
/*                                                                            */
/*                                    RNG                                     */
/*                                                                            */
/******************************************************************************/
/********************  Bits definition for RNG_CR register  *******************/
#define RNG_CR_RNGEN_Pos    (2U)
#define RNG_CR_RNGEN_Msk    (0x1UL << RNG_CR_RNGEN_Pos)                         /*!< 0x00000004 */
#define RNG_CR_RNGEN        RNG_CR_RNGEN_Msk
#define RNG_CR_IE_Pos       (3U)
#define RNG_CR_IE_Msk       (0x1UL << RNG_CR_IE_Pos)                            /*!< 0x00000008 */
#define RNG_CR_IE           RNG_CR_IE_Msk
#define RNG_CR_CED_Pos      (5U)
#define RNG_CR_CED_Msk      (0x1UL << RNG_CR_CED_Pos)                           /*!< 0x00000020 */
#define RNG_CR_CED          RNG_CR_CED_Msk

/********************  Bits definition for RNG_SR register  *******************/
#define RNG_SR_DRDY_Pos     (0U)
#define RNG_SR_DRDY_Msk     (0x1UL << RNG_SR_DRDY_Pos)                          /*!< 0x00000001 */
#define RNG_SR_DRDY         RNG_SR_DRDY_Msk
#define RNG_SR_CECS_Pos     (1U)
#define RNG_SR_CECS_Msk     (0x1UL << RNG_SR_CECS_Pos)                          /*!< 0x00000002 */
#define RNG_SR_CECS         RNG_SR_CECS_Msk
#define RNG_SR_SECS_Pos     (2U)
#define RNG_SR_SECS_Msk     (0x1UL << RNG_SR_SECS_Pos)                          /*!< 0x00000004 */
#define RNG_SR_SECS         RNG_SR_SECS_Msk
#define RNG_SR_CEIS_Pos     (5U)
#define RNG_SR_CEIS_Msk     (0x1UL << RNG_SR_CEIS_Pos)                          /*!< 0x00000020 */
#define RNG_SR_CEIS         RNG_SR_CEIS_Msk
#define RNG_SR_SEIS_Pos     (6U)
#define RNG_SR_SEIS_Msk     (0x1UL << RNG_SR_SEIS_Pos)                          /*!< 0x00000040 */
#define RNG_SR_SEIS         RNG_SR_SEIS_Msk

/******************************************************************************/
/*                                                                            */
/*                           Real-Time Clock (RTC)                            */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions
*/
#define RTC_WAKEUP_SUPPORT
#define RTC_BACKUP_SUPPORT

/********************  Bits definition for RTC_TR register  *******************/
#define RTC_TR_PM_Pos                (22U)
#define RTC_TR_PM_Msk                (0x1UL << RTC_TR_PM_Pos)                   /*!< 0x00400000 */
#define RTC_TR_PM                    RTC_TR_PM_Msk
#define RTC_TR_HT_Pos                (20U)
#define RTC_TR_HT_Msk                (0x3UL << RTC_TR_HT_Pos)                   /*!< 0x00300000 */
#define RTC_TR_HT                    RTC_TR_HT_Msk
#define RTC_TR_HT_0                  (0x1UL << RTC_TR_HT_Pos)                   /*!< 0x00100000 */
#define RTC_TR_HT_1                  (0x2UL << RTC_TR_HT_Pos)                   /*!< 0x00200000 */
#define RTC_TR_HU_Pos                (16U)
#define RTC_TR_HU_Msk                (0xFUL << RTC_TR_HU_Pos)                   /*!< 0x000F0000 */
#define RTC_TR_HU                    RTC_TR_HU_Msk
#define RTC_TR_HU_0                  (0x1UL << RTC_TR_HU_Pos)                   /*!< 0x00010000 */
#define RTC_TR_HU_1                  (0x2UL << RTC_TR_HU_Pos)                   /*!< 0x00020000 */
#define RTC_TR_HU_2                  (0x4UL << RTC_TR_HU_Pos)                   /*!< 0x00040000 */
#define RTC_TR_HU_3                  (0x8UL << RTC_TR_HU_Pos)                   /*!< 0x00080000 */
#define RTC_TR_MNT_Pos               (12U)
#define RTC_TR_MNT_Msk               (0x7UL << RTC_TR_MNT_Pos)                  /*!< 0x00007000 */
#define RTC_TR_MNT                   RTC_TR_MNT_Msk
#define RTC_TR_MNT_0                 (0x1UL << RTC_TR_MNT_Pos)                  /*!< 0x00001000 */
#define RTC_TR_MNT_1                 (0x2UL << RTC_TR_MNT_Pos)                  /*!< 0x00002000 */
#define RTC_TR_MNT_2                 (0x4UL << RTC_TR_MNT_Pos)                  /*!< 0x00004000 */
#define RTC_TR_MNU_Pos               (8U)
#define RTC_TR_MNU_Msk               (0xFUL << RTC_TR_MNU_Pos)                  /*!< 0x00000F00 */
#define RTC_TR_MNU                   RTC_TR_MNU_Msk
#define RTC_TR_MNU_0                 (0x1UL << RTC_TR_MNU_Pos)                  /*!< 0x00000100 */
#define RTC_TR_MNU_1                 (0x2UL << RTC_TR_MNU_Pos)                  /*!< 0x00000200 */
#define RTC_TR_MNU_2                 (0x4UL << RTC_TR_MNU_Pos)                  /*!< 0x00000400 */
#define RTC_TR_MNU_3                 (0x8UL << RTC_TR_MNU_Pos)                  /*!< 0x00000800 */
#define RTC_TR_ST_Pos                (4U)
#define RTC_TR_ST_Msk                (0x7UL << RTC_TR_ST_Pos)                   /*!< 0x00000070 */
#define RTC_TR_ST                    RTC_TR_ST_Msk
#define RTC_TR_ST_0                  (0x1UL << RTC_TR_ST_Pos)                   /*!< 0x00000010 */
#define RTC_TR_ST_1                  (0x2UL << RTC_TR_ST_Pos)                   /*!< 0x00000020 */
#define RTC_TR_ST_2                  (0x4UL << RTC_TR_ST_Pos)                   /*!< 0x00000040 */
#define RTC_TR_SU_Pos                (0U)
#define RTC_TR_SU_Msk                (0xFUL << RTC_TR_SU_Pos)                   /*!< 0x0000000F */
#define RTC_TR_SU                    RTC_TR_SU_Msk
#define RTC_TR_SU_0                  (0x1UL << RTC_TR_SU_Pos)                   /*!< 0x00000001 */
#define RTC_TR_SU_1                  (0x2UL << RTC_TR_SU_Pos)                   /*!< 0x00000002 */
#define RTC_TR_SU_2                  (0x4UL << RTC_TR_SU_Pos)                   /*!< 0x00000004 */
#define RTC_TR_SU_3                  (0x8UL << RTC_TR_SU_Pos)                   /*!< 0x00000008 */

/********************  Bits definition for RTC_DR register  *******************/
#define RTC_DR_YT_Pos                (20U)
#define RTC_DR_YT_Msk                (0xFUL << RTC_DR_YT_Pos)                   /*!< 0x00F00000 */
#define RTC_DR_YT                    RTC_DR_YT_Msk
#define RTC_DR_YT_0                  (0x1UL << RTC_DR_YT_Pos)                   /*!< 0x00100000 */
#define RTC_DR_YT_1                  (0x2UL << RTC_DR_YT_Pos)                   /*!< 0x00200000 */
#define RTC_DR_YT_2                  (0x4UL << RTC_DR_YT_Pos)                   /*!< 0x00400000 */
#define RTC_DR_YT_3                  (0x8UL << RTC_DR_YT_Pos)                   /*!< 0x00800000 */
#define RTC_DR_YU_Pos                (16U)
#define RTC_DR_YU_Msk                (0xFUL << RTC_DR_YU_Pos)                   /*!< 0x000F0000 */
#define RTC_DR_YU                    RTC_DR_YU_Msk
#define RTC_DR_YU_0                  (0x1UL << RTC_DR_YU_Pos)                   /*!< 0x00010000 */
#define RTC_DR_YU_1                  (0x2UL << RTC_DR_YU_Pos)                   /*!< 0x00020000 */
#define RTC_DR_YU_2                  (0x4UL << RTC_DR_YU_Pos)                   /*!< 0x00040000 */
#define RTC_DR_YU_3                  (0x8UL << RTC_DR_YU_Pos)                   /*!< 0x00080000 */
#define RTC_DR_WDU_Pos               (13U)
#define RTC_DR_WDU_Msk               (0x7UL << RTC_DR_WDU_Pos)                  /*!< 0x0000E000 */
#define RTC_DR_WDU                   RTC_DR_WDU_Msk
#define RTC_DR_WDU_0                 (0x1UL << RTC_DR_WDU_Pos)                  /*!< 0x00002000 */
#define RTC_DR_WDU_1                 (0x2UL << RTC_DR_WDU_Pos)                  /*!< 0x00004000 */
#define RTC_DR_WDU_2                 (0x4UL << RTC_DR_WDU_Pos)                  /*!< 0x00008000 */
#define RTC_DR_MT_Pos                (12U)
#define RTC_DR_MT_Msk                (0x1UL << RTC_DR_MT_Pos)                   /*!< 0x00001000 */
#define RTC_DR_MT                    RTC_DR_MT_Msk
#define RTC_DR_MU_Pos                (8U)
#define RTC_DR_MU_Msk                (0xFUL << RTC_DR_MU_Pos)                   /*!< 0x00000F00 */
#define RTC_DR_MU                    RTC_DR_MU_Msk
#define RTC_DR_MU_0                  (0x1UL << RTC_DR_MU_Pos)                   /*!< 0x00000100 */
#define RTC_DR_MU_1                  (0x2UL << RTC_DR_MU_Pos)                   /*!< 0x00000200 */
#define RTC_DR_MU_2                  (0x4UL << RTC_DR_MU_Pos)                   /*!< 0x00000400 */
#define RTC_DR_MU_3                  (0x8UL << RTC_DR_MU_Pos)                   /*!< 0x00000800 */
#define RTC_DR_DT_Pos                (4U)
#define RTC_DR_DT_Msk                (0x3UL << RTC_DR_DT_Pos)                   /*!< 0x00000030 */
#define RTC_DR_DT                    RTC_DR_DT_Msk
#define RTC_DR_DT_0                  (0x1UL << RTC_DR_DT_Pos)                   /*!< 0x00000010 */
#define RTC_DR_DT_1                  (0x2UL << RTC_DR_DT_Pos)                   /*!< 0x00000020 */
#define RTC_DR_DU_Pos                (0U)
#define RTC_DR_DU_Msk                (0xFUL << RTC_DR_DU_Pos)                   /*!< 0x0000000F */
#define RTC_DR_DU                    RTC_DR_DU_Msk
#define RTC_DR_DU_0                  (0x1UL << RTC_DR_DU_Pos)                   /*!< 0x00000001 */
#define RTC_DR_DU_1                  (0x2UL << RTC_DR_DU_Pos)                   /*!< 0x00000002 */
#define RTC_DR_DU_2                  (0x4UL << RTC_DR_DU_Pos)                   /*!< 0x00000004 */
#define RTC_DR_DU_3                  (0x8UL << RTC_DR_DU_Pos)                   /*!< 0x00000008 */

/********************  Bits definition for RTC_SSR register  ******************/
#define RTC_SSR_SS_Pos               (0U)
#define RTC_SSR_SS_Msk               (0xFFFFUL << RTC_SSR_SS_Pos)               /*!< 0x0000FFFF */
#define RTC_SSR_SS                   RTC_SSR_SS_Msk

/********************  Bits definition for RTC_ICSR register  ******************/
#define RTC_ICSR_RECALPF_Pos         (16U)
#define RTC_ICSR_RECALPF_Msk         (0x1UL << RTC_ICSR_RECALPF_Pos)            /*!< 0x00010000 */
#define RTC_ICSR_RECALPF             RTC_ICSR_RECALPF_Msk
#define RTC_ICSR_INIT_Pos            (7U)
#define RTC_ICSR_INIT_Msk            (0x1UL << RTC_ICSR_INIT_Pos)               /*!< 0x00000080 */
#define RTC_ICSR_INIT                RTC_ICSR_INIT_Msk
#define RTC_ICSR_INITF_Pos           (6U)
#define RTC_ICSR_INITF_Msk           (0x1UL << RTC_ICSR_INITF_Pos)              /*!< 0x00000040 */
#define RTC_ICSR_INITF               RTC_ICSR_INITF_Msk
#define RTC_ICSR_RSF_Pos             (5U)
#define RTC_ICSR_RSF_Msk             (0x1UL << RTC_ICSR_RSF_Pos)                /*!< 0x00000020 */
#define RTC_ICSR_RSF                 RTC_ICSR_RSF_Msk
#define RTC_ICSR_INITS_Pos           (4U)
#define RTC_ICSR_INITS_Msk           (0x1UL << RTC_ICSR_INITS_Pos)              /*!< 0x00000010 */
#define RTC_ICSR_INITS               RTC_ICSR_INITS_Msk
#define RTC_ICSR_SHPF_Pos            (3U)
#define RTC_ICSR_SHPF_Msk            (0x1UL << RTC_ICSR_SHPF_Pos)               /*!< 0x00000008 */
#define RTC_ICSR_SHPF                RTC_ICSR_SHPF_Msk
#define RTC_ICSR_WUTWF_Pos           (2U)
#define RTC_ICSR_WUTWF_Msk           (0x1UL << RTC_ICSR_WUTWF_Pos)              /*!< 0x00000004 */
#define RTC_ICSR_WUTWF               RTC_ICSR_WUTWF_Msk                         /*!< Wakeup timer write flag > */
#define RTC_ICSR_ALRBWF_Pos          (1U)
#define RTC_ICSR_ALRBWF_Msk          (0x1UL << RTC_ICSR_ALRBWF_Pos)             /*!< 0x00000002 */
#define RTC_ICSR_ALRBWF              RTC_ICSR_ALRBWF_Msk
#define RTC_ICSR_ALRAWF_Pos          (0U)
#define RTC_ICSR_ALRAWF_Msk          (0x1UL << RTC_ICSR_ALRAWF_Pos)             /*!< 0x00000001 */
#define RTC_ICSR_ALRAWF              RTC_ICSR_ALRAWF_Msk

/********************  Bits definition for RTC_PRER register  *****************/
#define RTC_PRER_PREDIV_A_Pos        (16U)
#define RTC_PRER_PREDIV_A_Msk        (0x7FUL << RTC_PRER_PREDIV_A_Pos)          /*!< 0x007F0000 */
#define RTC_PRER_PREDIV_A            RTC_PRER_PREDIV_A_Msk
#define RTC_PRER_PREDIV_S_Pos        (0U)
#define RTC_PRER_PREDIV_S_Msk        (0x7FFFUL << RTC_PRER_PREDIV_S_Pos)        /*!< 0x00007FFF */
#define RTC_PRER_PREDIV_S            RTC_PRER_PREDIV_S_Msk

/********************  Bits definition for RTC_WUTR register  *****************/
#define RTC_WUTR_WUT_Pos             (0U)
#define RTC_WUTR_WUT_Msk             (0xFFFFUL << RTC_WUTR_WUT_Pos)            /*!< 0x0000FFFF */
#define RTC_WUTR_WUT                 RTC_WUTR_WUT_Msk                          /*!< Wakeup auto-reload value bits > */

/********************  Bits definition for RTC_CR register  *******************/
#define RTC_CR_OUT2EN_Pos            (31U)
#define RTC_CR_OUT2EN_Msk            (0x1UL << RTC_CR_OUT2EN_Pos)              /*!< 0x80000000 */
#define RTC_CR_OUT2EN                RTC_CR_OUT2EN_Msk                         /*!< RTC_OUT2 output enable */
#define RTC_CR_TAMPALRM_TYPE_Pos     (30U)
#define RTC_CR_TAMPALRM_TYPE_Msk     (0x1UL << RTC_CR_TAMPALRM_TYPE_Pos)       /*!< 0x40000000 */
#define RTC_CR_TAMPALRM_TYPE         RTC_CR_TAMPALRM_TYPE_Msk                  /*!< TAMPALARM output type  */
#define RTC_CR_TAMPALRM_PU_Pos       (29U)
#define RTC_CR_TAMPALRM_PU_Msk       (0x1UL << RTC_CR_TAMPALRM_PU_Pos)         /*!< 0x20000000 */
#define RTC_CR_TAMPALRM_PU           RTC_CR_TAMPALRM_PU_Msk                    /*!< TAMPALARM output pull-up config */
#define RTC_CR_TAMPOE_Pos            (26U)
#define RTC_CR_TAMPOE_Msk            (0x1UL << RTC_CR_TAMPOE_Pos)              /*!< 0x04000000 */
#define RTC_CR_TAMPOE                RTC_CR_TAMPOE_Msk                         /*!< Tamper detection output enable on TAMPALARM  */
#define RTC_CR_TAMPTS_Pos            (25U)
#define RTC_CR_TAMPTS_Msk            (0x1UL << RTC_CR_TAMPTS_Pos)              /*!< 0x02000000 */
#define RTC_CR_TAMPTS                RTC_CR_TAMPTS_Msk                         /*!< Activate timestamp on tamper detection event  */
#define RTC_CR_ITSE_Pos              (24U)
#define RTC_CR_ITSE_Msk              (0x1UL << RTC_CR_ITSE_Pos)                /*!< 0x01000000 */
#define RTC_CR_ITSE                  RTC_CR_ITSE_Msk                           /*!< Timestamp on internal event enable  */
#define RTC_CR_COE_Pos               (23U)
#define RTC_CR_COE_Msk               (0x1UL << RTC_CR_COE_Pos)                 /*!< 0x00800000 */
#define RTC_CR_COE                   RTC_CR_COE_Msk
#define RTC_CR_OSEL_Pos              (21U)
#define RTC_CR_OSEL_Msk              (0x3UL << RTC_CR_OSEL_Pos)                 /*!< 0x00600000 */
#define RTC_CR_OSEL                  RTC_CR_OSEL_Msk
#define RTC_CR_OSEL_0                (0x1UL << RTC_CR_OSEL_Pos)                 /*!< 0x00200000 */
#define RTC_CR_OSEL_1                (0x2UL << RTC_CR_OSEL_Pos)                 /*!< 0x00400000 */
#define RTC_CR_POL_Pos               (20U)
#define RTC_CR_POL_Msk               (0x1UL << RTC_CR_POL_Pos)                  /*!< 0x00100000 */
#define RTC_CR_POL                   RTC_CR_POL_Msk
#define RTC_CR_COSEL_Pos             (19U)
#define RTC_CR_COSEL_Msk             (0x1UL << RTC_CR_COSEL_Pos)                /*!< 0x00080000 */
#define RTC_CR_COSEL                 RTC_CR_COSEL_Msk
#define RTC_CR_BKP_Pos               (18U)
#define RTC_CR_BKP_Msk               (0x1UL << RTC_CR_BKP_Pos)                  /*!< 0x00040000 */
#define RTC_CR_BKP                   RTC_CR_BKP_Msk
#define RTC_CR_SUB1H_Pos             (17U)
#define RTC_CR_SUB1H_Msk             (0x1UL << RTC_CR_SUB1H_Pos)                /*!< 0x00020000 */
#define RTC_CR_SUB1H                 RTC_CR_SUB1H_Msk
#define RTC_CR_ADD1H_Pos             (16U)
#define RTC_CR_ADD1H_Msk             (0x1UL << RTC_CR_ADD1H_Pos)                /*!< 0x00010000 */
#define RTC_CR_ADD1H                 RTC_CR_ADD1H_Msk
#define RTC_CR_TSIE_Pos              (15U)
#define RTC_CR_TSIE_Msk              (0x1UL << RTC_CR_TSIE_Pos)                /*!< 0x00008000 */
#define RTC_CR_TSIE                  RTC_CR_TSIE_Msk                           /*!< Timestamp interrupt enable > */
#define RTC_CR_WUTIE_Pos             (14U)
#define RTC_CR_WUTIE_Msk             (0x1UL << RTC_CR_WUTIE_Pos)               /*!< 0x00004000 */
#define RTC_CR_WUTIE                 RTC_CR_WUTIE_Msk                          /*!< Wakeup timer interrupt enable > */
#define RTC_CR_ALRBIE_Pos            (13U)
#define RTC_CR_ALRBIE_Msk            (0x1UL << RTC_CR_ALRBIE_Pos)              /*!< 0x00002000 */
#define RTC_CR_ALRBIE                RTC_CR_ALRBIE_Msk
#define RTC_CR_ALRAIE_Pos            (12U)
#define RTC_CR_ALRAIE_Msk            (0x1UL << RTC_CR_ALRAIE_Pos)              /*!< 0x00001000 */
#define RTC_CR_ALRAIE                RTC_CR_ALRAIE_Msk
#define RTC_CR_TSE_Pos               (11U)
#define RTC_CR_TSE_Msk               (0x1UL << RTC_CR_TSE_Pos)                 /*!< 0x00000800 */
#define RTC_CR_TSE                   RTC_CR_TSE_Msk                            /*!< timestamp enable > */
#define RTC_CR_WUTE_Pos              (10U)
#define RTC_CR_WUTE_Msk              (0x1UL << RTC_CR_WUTE_Pos)                /*!< 0x00000400 */
#define RTC_CR_WUTE                  RTC_CR_WUTE_Msk                           /*!< Wakeup timer enable > */
#define RTC_CR_ALRBE_Pos             (9U)
#define RTC_CR_ALRBE_Msk             (0x1UL << RTC_CR_ALRBE_Pos)               /*!< 0x00000200 */
#define RTC_CR_ALRBE                 RTC_CR_ALRBE_Msk
#define RTC_CR_ALRAE_Pos             (8U)
#define RTC_CR_ALRAE_Msk             (0x1UL << RTC_CR_ALRAE_Pos)                /*!< 0x00000100 */
#define RTC_CR_ALRAE                 RTC_CR_ALRAE_Msk
#define RTC_CR_FMT_Pos               (6U)
#define RTC_CR_FMT_Msk               (0x1UL << RTC_CR_FMT_Pos)                  /*!< 0x00000040 */
#define RTC_CR_FMT                   RTC_CR_FMT_Msk
#define RTC_CR_BYPSHAD_Pos           (5U)
#define RTC_CR_BYPSHAD_Msk           (0x1UL << RTC_CR_BYPSHAD_Pos)              /*!< 0x00000020 */
#define RTC_CR_BYPSHAD               RTC_CR_BYPSHAD_Msk
#define RTC_CR_REFCKON_Pos           (4U)
#define RTC_CR_REFCKON_Msk           (0x1UL << RTC_CR_REFCKON_Pos)              /*!< 0x00000010 */
#define RTC_CR_REFCKON               RTC_CR_REFCKON_Msk
#define RTC_CR_TSEDGE_Pos            (3U)
#define RTC_CR_TSEDGE_Msk            (0x1UL << RTC_CR_TSEDGE_Pos)              /*!< 0x00000008 */
#define RTC_CR_TSEDGE                RTC_CR_TSEDGE_Msk                         /*!< Timestamp event active edge > */
#define RTC_CR_WUCKSEL_Pos           (0U)
#define RTC_CR_WUCKSEL_Msk           (0x7UL << RTC_CR_WUCKSEL_Pos)             /*!< 0x00000007 */
#define RTC_CR_WUCKSEL               RTC_CR_WUCKSEL_Msk                        /*!< Wakeup clock selection > */
#define RTC_CR_WUCKSEL_0             (0x1UL << RTC_CR_WUCKSEL_Pos)             /*!< 0x00000001 */
#define RTC_CR_WUCKSEL_1             (0x2UL << RTC_CR_WUCKSEL_Pos)             /*!< 0x00000002 */
#define RTC_CR_WUCKSEL_2             (0x4UL << RTC_CR_WUCKSEL_Pos)             /*!< 0x00000004 */

/********************  Bits definition for RTC_WPR register  ******************/
#define RTC_WPR_KEY_Pos              (0U)
#define RTC_WPR_KEY_Msk              (0xFFUL << RTC_WPR_KEY_Pos)                /*!< 0x000000FF */
#define RTC_WPR_KEY                  RTC_WPR_KEY_Msk

/********************  Bits definition for RTC_CALR register  *****************/
#define RTC_CALR_CALP_Pos            (15U)
#define RTC_CALR_CALP_Msk            (0x1UL << RTC_CALR_CALP_Pos)               /*!< 0x00008000 */
#define RTC_CALR_CALP                RTC_CALR_CALP_Msk
#define RTC_CALR_CALW8_Pos           (14U)
#define RTC_CALR_CALW8_Msk           (0x1UL << RTC_CALR_CALW8_Pos)              /*!< 0x00004000 */
#define RTC_CALR_CALW8               RTC_CALR_CALW8_Msk
#define RTC_CALR_CALW16_Pos          (13U)
#define RTC_CALR_CALW16_Msk          (0x1UL << RTC_CALR_CALW16_Pos)             /*!< 0x00002000 */
#define RTC_CALR_CALW16              RTC_CALR_CALW16_Msk
#define RTC_CALR_CALM_Pos            (0U)
#define RTC_CALR_CALM_Msk            (0x1FFUL << RTC_CALR_CALM_Pos)             /*!< 0x000001FF */
#define RTC_CALR_CALM                RTC_CALR_CALM_Msk
#define RTC_CALR_CALM_0              (0x001UL << RTC_CALR_CALM_Pos)             /*!< 0x00000001 */
#define RTC_CALR_CALM_1              (0x002UL << RTC_CALR_CALM_Pos)             /*!< 0x00000002 */
#define RTC_CALR_CALM_2              (0x004UL << RTC_CALR_CALM_Pos)             /*!< 0x00000004 */
#define RTC_CALR_CALM_3              (0x008UL << RTC_CALR_CALM_Pos)             /*!< 0x00000008 */
#define RTC_CALR_CALM_4              (0x010UL << RTC_CALR_CALM_Pos)             /*!< 0x00000010 */
#define RTC_CALR_CALM_5              (0x020UL << RTC_CALR_CALM_Pos)             /*!< 0x00000020 */
#define RTC_CALR_CALM_6              (0x040UL << RTC_CALR_CALM_Pos)             /*!< 0x00000040 */
#define RTC_CALR_CALM_7              (0x080UL << RTC_CALR_CALM_Pos)             /*!< 0x00000080 */
#define RTC_CALR_CALM_8              (0x100UL << RTC_CALR_CALM_Pos)             /*!< 0x00000100 */

/********************  Bits definition for RTC_SHIFTR register  ***************/
#define RTC_SHIFTR_SUBFS_Pos         (0U)
#define RTC_SHIFTR_SUBFS_Msk         (0x7FFFUL << RTC_SHIFTR_SUBFS_Pos)         /*!< 0x00007FFF */
#define RTC_SHIFTR_SUBFS             RTC_SHIFTR_SUBFS_Msk
#define RTC_SHIFTR_ADD1S_Pos         (31U)
#define RTC_SHIFTR_ADD1S_Msk         (0x1UL << RTC_SHIFTR_ADD1S_Pos)            /*!< 0x80000000 */
#define RTC_SHIFTR_ADD1S             RTC_SHIFTR_ADD1S_Msk

/********************  Bits definition for RTC_TSTR register  *****************/
#define RTC_TSTR_PM_Pos              (22U)
#define RTC_TSTR_PM_Msk              (0x1UL << RTC_TSTR_PM_Pos)                /*!< 0x00400000 */
#define RTC_TSTR_PM                  RTC_TSTR_PM_Msk                           /*!< AM-PM notation > */
#define RTC_TSTR_HT_Pos              (20U)
#define RTC_TSTR_HT_Msk              (0x3UL << RTC_TSTR_HT_Pos)                /*!< 0x00300000 */
#define RTC_TSTR_HT                  RTC_TSTR_HT_Msk
#define RTC_TSTR_HT_0                (0x1UL << RTC_TSTR_HT_Pos)                /*!< 0x00100000 */
#define RTC_TSTR_HT_1                (0x2UL << RTC_TSTR_HT_Pos)                /*!< 0x00200000 */
#define RTC_TSTR_HU_Pos              (16U)
#define RTC_TSTR_HU_Msk              (0xFUL << RTC_TSTR_HU_Pos)                /*!< 0x000F0000 */
#define RTC_TSTR_HU                  RTC_TSTR_HU_Msk
#define RTC_TSTR_HU_0                (0x1UL << RTC_TSTR_HU_Pos)                /*!< 0x00010000 */
#define RTC_TSTR_HU_1                (0x2UL << RTC_TSTR_HU_Pos)                /*!< 0x00020000 */
#define RTC_TSTR_HU_2                (0x4UL << RTC_TSTR_HU_Pos)                /*!< 0x00040000 */
#define RTC_TSTR_HU_3                (0x8UL << RTC_TSTR_HU_Pos)                /*!< 0x00080000 */
#define RTC_TSTR_MNT_Pos             (12U)
#define RTC_TSTR_MNT_Msk             (0x7UL << RTC_TSTR_MNT_Pos)               /*!< 0x00007000 */
#define RTC_TSTR_MNT                 RTC_TSTR_MNT_Msk
#define RTC_TSTR_MNT_0               (0x1UL << RTC_TSTR_MNT_Pos)               /*!< 0x00001000 */
#define RTC_TSTR_MNT_1               (0x2UL << RTC_TSTR_MNT_Pos)               /*!< 0x00002000 */
#define RTC_TSTR_MNT_2               (0x4UL << RTC_TSTR_MNT_Pos)                /*!< 0x00004000 */
#define RTC_TSTR_MNU_Pos             (8U)
#define RTC_TSTR_MNU_Msk             (0xFUL << RTC_TSTR_MNU_Pos)                /*!< 0x00000F00 */
#define RTC_TSTR_MNU                 RTC_TSTR_MNU_Msk
#define RTC_TSTR_MNU_0               (0x1UL << RTC_TSTR_MNU_Pos)                /*!< 0x00000100 */
#define RTC_TSTR_MNU_1               (0x2UL << RTC_TSTR_MNU_Pos)                /*!< 0x00000200 */
#define RTC_TSTR_MNU_2               (0x4UL << RTC_TSTR_MNU_Pos)                /*!< 0x00000400 */
#define RTC_TSTR_MNU_3               (0x8UL << RTC_TSTR_MNU_Pos)                /*!< 0x00000800 */
#define RTC_TSTR_ST_Pos              (4U)
#define RTC_TSTR_ST_Msk              (0x7UL << RTC_TSTR_ST_Pos)                 /*!< 0x00000070 */
#define RTC_TSTR_ST                  RTC_TSTR_ST_Msk
#define RTC_TSTR_ST_0                (0x1UL << RTC_TSTR_ST_Pos)                 /*!< 0x00000010 */
#define RTC_TSTR_ST_1                (0x2UL << RTC_TSTR_ST_Pos)                 /*!< 0x00000020 */
#define RTC_TSTR_ST_2                (0x4UL << RTC_TSTR_ST_Pos)                 /*!< 0x00000040 */
#define RTC_TSTR_SU_Pos              (0U)
#define RTC_TSTR_SU_Msk              (0xFUL << RTC_TSTR_SU_Pos)                 /*!< 0x0000000F */
#define RTC_TSTR_SU                  RTC_TSTR_SU_Msk
#define RTC_TSTR_SU_0                (0x1UL << RTC_TSTR_SU_Pos)                 /*!< 0x00000001 */
#define RTC_TSTR_SU_1                (0x2UL << RTC_TSTR_SU_Pos)                 /*!< 0x00000002 */
#define RTC_TSTR_SU_2                (0x4UL << RTC_TSTR_SU_Pos)                 /*!< 0x00000004 */
#define RTC_TSTR_SU_3                (0x8UL << RTC_TSTR_SU_Pos)                 /*!< 0x00000008 */

/********************  Bits definition for RTC_TSDR register  *****************/
#define RTC_TSDR_WDU_Pos             (13U)
#define RTC_TSDR_WDU_Msk             (0x7UL << RTC_TSDR_WDU_Pos)               /*!< 0x0000E000 */
#define RTC_TSDR_WDU                 RTC_TSDR_WDU_Msk                          /*!< Week day units > */
#define RTC_TSDR_WDU_0               (0x1UL << RTC_TSDR_WDU_Pos)               /*!< 0x00002000 */
#define RTC_TSDR_WDU_1               (0x2UL << RTC_TSDR_WDU_Pos)               /*!< 0x00004000 */
#define RTC_TSDR_WDU_2               (0x4UL << RTC_TSDR_WDU_Pos)               /*!< 0x00008000 */
#define RTC_TSDR_MT_Pos              (12U)
#define RTC_TSDR_MT_Msk              (0x1UL << RTC_TSDR_MT_Pos)                /*!< 0x00001000 */
#define RTC_TSDR_MT                  RTC_TSDR_MT_Msk
#define RTC_TSDR_MU_Pos              (8U)
#define RTC_TSDR_MU_Msk              (0xFUL << RTC_TSDR_MU_Pos)                /*!< 0x00000F00 */
#define RTC_TSDR_MU                  RTC_TSDR_MU_Msk
#define RTC_TSDR_MU_0                (0x1UL << RTC_TSDR_MU_Pos)                /*!< 0x00000100 */
#define RTC_TSDR_MU_1                (0x2UL << RTC_TSDR_MU_Pos)                /*!< 0x00000200 */
#define RTC_TSDR_MU_2                (0x4UL << RTC_TSDR_MU_Pos)                /*!< 0x00000400 */
#define RTC_TSDR_MU_3                (0x8UL << RTC_TSDR_MU_Pos)                /*!< 0x00000800 */
#define RTC_TSDR_DT_Pos              (4U)
#define RTC_TSDR_DT_Msk              (0x3UL << RTC_TSDR_DT_Pos)                /*!< 0x00000030 */
#define RTC_TSDR_DT                  RTC_TSDR_DT_Msk
#define RTC_TSDR_DT_0                (0x1UL << RTC_TSDR_DT_Pos)                /*!< 0x00000010 */
#define RTC_TSDR_DT_1                (0x2UL << RTC_TSDR_DT_Pos)                /*!< 0x00000020 */
#define RTC_TSDR_DU_Pos              (0U)
#define RTC_TSDR_DU_Msk              (0xFUL << RTC_TSDR_DU_Pos)                /*!< 0x0000000F */
#define RTC_TSDR_DU                  RTC_TSDR_DU_Msk
#define RTC_TSDR_DU_0                (0x1UL << RTC_TSDR_DU_Pos)                /*!< 0x00000001 */
#define RTC_TSDR_DU_1                (0x2UL << RTC_TSDR_DU_Pos)                /*!< 0x00000002 */
#define RTC_TSDR_DU_2                (0x4UL << RTC_TSDR_DU_Pos)                /*!< 0x00000004 */
#define RTC_TSDR_DU_3                (0x8UL << RTC_TSDR_DU_Pos)                /*!< 0x00000008 */

/********************  Bits definition for RTC_TSSSR register  ****************/
#define RTC_TSSSR_SS_Pos             (0U)
#define RTC_TSSSR_SS_Msk             (0xFFFFUL << RTC_TSSSR_SS_Pos)            /*!< 0x0000FFFF */
#define RTC_TSSSR_SS                 RTC_TSSSR_SS_Msk                          /*!< Sub second value > */

/********************  Bits definition for RTC_ALRMAR register  ***************/
#define RTC_ALRMAR_MSK4_Pos          (31U)
#define RTC_ALRMAR_MSK4_Msk          (0x1UL << RTC_ALRMAR_MSK4_Pos)            /*!< 0x80000000 */
#define RTC_ALRMAR_MSK4              RTC_ALRMAR_MSK4_Msk
#define RTC_ALRMAR_WDSEL_Pos         (30U)
#define RTC_ALRMAR_WDSEL_Msk         (0x1UL << RTC_ALRMAR_WDSEL_Pos)           /*!< 0x40000000 */
#define RTC_ALRMAR_WDSEL             RTC_ALRMAR_WDSEL_Msk
#define RTC_ALRMAR_DT_Pos            (28U)
#define RTC_ALRMAR_DT_Msk            (0x3UL << RTC_ALRMAR_DT_Pos)              /*!< 0x30000000 */
#define RTC_ALRMAR_DT                RTC_ALRMAR_DT_Msk
#define RTC_ALRMAR_DT_0              (0x1UL << RTC_ALRMAR_DT_Pos)              /*!< 0x10000000 */
#define RTC_ALRMAR_DT_1              (0x2UL << RTC_ALRMAR_DT_Pos)              /*!< 0x20000000 */
#define RTC_ALRMAR_DU_Pos            (24U)
#define RTC_ALRMAR_DU_Msk            (0xFUL << RTC_ALRMAR_DU_Pos)              /*!< 0x0F000000 */
#define RTC_ALRMAR_DU                RTC_ALRMAR_DU_Msk
#define RTC_ALRMAR_DU_0              (0x1UL << RTC_ALRMAR_DU_Pos)              /*!< 0x01000000 */
#define RTC_ALRMAR_DU_1              (0x2UL << RTC_ALRMAR_DU_Pos)              /*!< 0x02000000 */
#define RTC_ALRMAR_DU_2              (0x4UL << RTC_ALRMAR_DU_Pos)              /*!< 0x04000000 */
#define RTC_ALRMAR_DU_3              (0x8UL << RTC_ALRMAR_DU_Pos)              /*!< 0x08000000 */
#define RTC_ALRMAR_MSK3_Pos          (23U)
#define RTC_ALRMAR_MSK3_Msk          (0x1UL << RTC_ALRMAR_MSK3_Pos)            /*!< 0x00800000 */
#define RTC_ALRMAR_MSK3              RTC_ALRMAR_MSK3_Msk
#define RTC_ALRMAR_PM_Pos            (22U)
#define RTC_ALRMAR_PM_Msk            (0x1UL << RTC_ALRMAR_PM_Pos)              /*!< 0x00400000 */
#define RTC_ALRMAR_PM                RTC_ALRMAR_PM_Msk
#define RTC_ALRMAR_HT_Pos            (20U)
#define RTC_ALRMAR_HT_Msk            (0x3UL << RTC_ALRMAR_HT_Pos)              /*!< 0x00300000 */
#define RTC_ALRMAR_HT                RTC_ALRMAR_HT_Msk
#define RTC_ALRMAR_HT_0              (0x1UL << RTC_ALRMAR_HT_Pos)              /*!< 0x00100000 */
#define RTC_ALRMAR_HT_1              (0x2UL << RTC_ALRMAR_HT_Pos)              /*!< 0x00200000 */
#define RTC_ALRMAR_HU_Pos            (16U)
#define RTC_ALRMAR_HU_Msk            (0xFUL << RTC_ALRMAR_HU_Pos)              /*!< 0x000F0000 */
#define RTC_ALRMAR_HU                RTC_ALRMAR_HU_Msk
#define RTC_ALRMAR_HU_0              (0x1UL << RTC_ALRMAR_HU_Pos)              /*!< 0x00010000 */
#define RTC_ALRMAR_HU_1              (0x2UL << RTC_ALRMAR_HU_Pos)              /*!< 0x00020000 */
#define RTC_ALRMAR_HU_2              (0x4UL << RTC_ALRMAR_HU_Pos)              /*!< 0x00040000 */
#define RTC_ALRMAR_HU_3              (0x8UL << RTC_ALRMAR_HU_Pos)              /*!< 0x00080000 */
#define RTC_ALRMAR_MSK2_Pos          (15U)
#define RTC_ALRMAR_MSK2_Msk          (0x1UL << RTC_ALRMAR_MSK2_Pos)            /*!< 0x00008000 */
#define RTC_ALRMAR_MSK2              RTC_ALRMAR_MSK2_Msk
#define RTC_ALRMAR_MNT_Pos           (12U)
#define RTC_ALRMAR_MNT_Msk           (0x7UL << RTC_ALRMAR_MNT_Pos)             /*!< 0x00007000 */
#define RTC_ALRMAR_MNT               RTC_ALRMAR_MNT_Msk
#define RTC_ALRMAR_MNT_0             (0x1UL << RTC_ALRMAR_MNT_Pos)             /*!< 0x00001000 */
#define RTC_ALRMAR_MNT_1             (0x2UL << RTC_ALRMAR_MNT_Pos)             /*!< 0x00002000 */
#define RTC_ALRMAR_MNT_2             (0x4UL << RTC_ALRMAR_MNT_Pos)             /*!< 0x00004000 */
#define RTC_ALRMAR_MNU_Pos           (8U)
#define RTC_ALRMAR_MNU_Msk           (0xFUL << RTC_ALRMAR_MNU_Pos)             /*!< 0x00000F00 */
#define RTC_ALRMAR_MNU               RTC_ALRMAR_MNU_Msk
#define RTC_ALRMAR_MNU_0             (0x1UL << RTC_ALRMAR_MNU_Pos)             /*!< 0x00000100 */
#define RTC_ALRMAR_MNU_1             (0x2UL << RTC_ALRMAR_MNU_Pos)             /*!< 0x00000200 */
#define RTC_ALRMAR_MNU_2             (0x4UL << RTC_ALRMAR_MNU_Pos)             /*!< 0x00000400 */
#define RTC_ALRMAR_MNU_3             (0x8UL << RTC_ALRMAR_MNU_Pos)             /*!< 0x00000800 */
#define RTC_ALRMAR_MSK1_Pos          (7U)
#define RTC_ALRMAR_MSK1_Msk          (0x1UL << RTC_ALRMAR_MSK1_Pos)            /*!< 0x00000080 */
#define RTC_ALRMAR_MSK1              RTC_ALRMAR_MSK1_Msk
#define RTC_ALRMAR_ST_Pos            (4U)
#define RTC_ALRMAR_ST_Msk            (0x7UL << RTC_ALRMAR_ST_Pos)              /*!< 0x00000070 */
#define RTC_ALRMAR_ST                RTC_ALRMAR_ST_Msk
#define RTC_ALRMAR_ST_0              (0x1UL << RTC_ALRMAR_ST_Pos)              /*!< 0x00000010 */
#define RTC_ALRMAR_ST_1              (0x2UL << RTC_ALRMAR_ST_Pos)              /*!< 0x00000020 */
#define RTC_ALRMAR_ST_2              (0x4UL << RTC_ALRMAR_ST_Pos)              /*!< 0x00000040 */
#define RTC_ALRMAR_SU_Pos            (0U)
#define RTC_ALRMAR_SU_Msk            (0xFUL << RTC_ALRMAR_SU_Pos)              /*!< 0x0000000F */
#define RTC_ALRMAR_SU                RTC_ALRMAR_SU_Msk
#define RTC_ALRMAR_SU_0              (0x1UL << RTC_ALRMAR_SU_Pos)              /*!< 0x00000001 */
#define RTC_ALRMAR_SU_1              (0x2UL << RTC_ALRMAR_SU_Pos)              /*!< 0x00000002 */
#define RTC_ALRMAR_SU_2              (0x4UL << RTC_ALRMAR_SU_Pos)              /*!< 0x00000004 */
#define RTC_ALRMAR_SU_3              (0x8UL << RTC_ALRMAR_SU_Pos)              /*!< 0x00000008 */

/********************  Bits definition for RTC_ALRMASSR register  *************/
#define RTC_ALRMASSR_MASKSS_Pos      (24U)
#define RTC_ALRMASSR_MASKSS_Msk      (0xFUL << RTC_ALRMASSR_MASKSS_Pos)        /*!< 0x0F000000 */
#define RTC_ALRMASSR_MASKSS          RTC_ALRMASSR_MASKSS_Msk
#define RTC_ALRMASSR_MASKSS_0        (0x1UL << RTC_ALRMASSR_MASKSS_Pos)        /*!< 0x01000000 */
#define RTC_ALRMASSR_MASKSS_1        (0x2UL << RTC_ALRMASSR_MASKSS_Pos)        /*!< 0x02000000 */
#define RTC_ALRMASSR_MASKSS_2        (0x4UL << RTC_ALRMASSR_MASKSS_Pos)        /*!< 0x04000000 */
#define RTC_ALRMASSR_MASKSS_3        (0x8UL << RTC_ALRMASSR_MASKSS_Pos)        /*!< 0x08000000 */
#define RTC_ALRMASSR_SS_Pos          (0U)
#define RTC_ALRMASSR_SS_Msk          (0x7FFFUL << RTC_ALRMASSR_SS_Pos)         /*!< 0x00007FFF */
#define RTC_ALRMASSR_SS              RTC_ALRMASSR_SS_Msk

/********************  Bits definition for RTC_ALRMBR register  ***************/
#define RTC_ALRMBR_MSK4_Pos          (31U)
#define RTC_ALRMBR_MSK4_Msk          (0x1UL << RTC_ALRMBR_MSK4_Pos)            /*!< 0x80000000 */
#define RTC_ALRMBR_MSK4              RTC_ALRMBR_MSK4_Msk
#define RTC_ALRMBR_WDSEL_Pos         (30U)
#define RTC_ALRMBR_WDSEL_Msk         (0x1UL << RTC_ALRMBR_WDSEL_Pos)           /*!< 0x40000000 */
#define RTC_ALRMBR_WDSEL             RTC_ALRMBR_WDSEL_Msk
#define RTC_ALRMBR_DT_Pos            (28U)
#define RTC_ALRMBR_DT_Msk            (0x3UL << RTC_ALRMBR_DT_Pos)              /*!< 0x30000000 */
#define RTC_ALRMBR_DT                RTC_ALRMBR_DT_Msk
#define RTC_ALRMBR_DT_0              (0x1UL << RTC_ALRMBR_DT_Pos)              /*!< 0x10000000 */
#define RTC_ALRMBR_DT_1              (0x2UL << RTC_ALRMBR_DT_Pos)              /*!< 0x20000000 */
#define RTC_ALRMBR_DU_Pos            (24U)
#define RTC_ALRMBR_DU_Msk            (0xFUL << RTC_ALRMBR_DU_Pos)              /*!< 0x0F000000 */
#define RTC_ALRMBR_DU                RTC_ALRMBR_DU_Msk
#define RTC_ALRMBR_DU_0              (0x1UL << RTC_ALRMBR_DU_Pos)              /*!< 0x01000000 */
#define RTC_ALRMBR_DU_1              (0x2UL << RTC_ALRMBR_DU_Pos)              /*!< 0x02000000 */
#define RTC_ALRMBR_DU_2              (0x4UL << RTC_ALRMBR_DU_Pos)              /*!< 0x04000000 */
#define RTC_ALRMBR_DU_3              (0x8UL << RTC_ALRMBR_DU_Pos)              /*!< 0x08000000 */
#define RTC_ALRMBR_MSK3_Pos          (23U)
#define RTC_ALRMBR_MSK3_Msk          (0x1UL << RTC_ALRMBR_MSK3_Pos)            /*!< 0x00800000 */
#define RTC_ALRMBR_MSK3              RTC_ALRMBR_MSK3_Msk
#define RTC_ALRMBR_PM_Pos            (22U)
#define RTC_ALRMBR_PM_Msk            (0x1UL << RTC_ALRMBR_PM_Pos)              /*!< 0x00400000 */
#define RTC_ALRMBR_PM                RTC_ALRMBR_PM_Msk
#define RTC_ALRMBR_HT_Pos            (20U)
#define RTC_ALRMBR_HT_Msk            (0x3UL << RTC_ALRMBR_HT_Pos)              /*!< 0x00300000 */
#define RTC_ALRMBR_HT                RTC_ALRMBR_HT_Msk
#define RTC_ALRMBR_HT_0              (0x1UL << RTC_ALRMBR_HT_Pos)              /*!< 0x00100000 */
#define RTC_ALRMBR_HT_1              (0x2UL << RTC_ALRMBR_HT_Pos)              /*!< 0x00200000 */
#define RTC_ALRMBR_HU_Pos            (16U)
#define RTC_ALRMBR_HU_Msk            (0xFUL << RTC_ALRMBR_HU_Pos)              /*!< 0x000F0000 */
#define RTC_ALRMBR_HU                RTC_ALRMBR_HU_Msk
#define RTC_ALRMBR_HU_0              (0x1UL << RTC_ALRMBR_HU_Pos)              /*!< 0x00010000 */
#define RTC_ALRMBR_HU_1              (0x2UL << RTC_ALRMBR_HU_Pos)              /*!< 0x00020000 */
#define RTC_ALRMBR_HU_2              (0x4UL << RTC_ALRMBR_HU_Pos)              /*!< 0x00040000 */
#define RTC_ALRMBR_HU_3              (0x8UL << RTC_ALRMBR_HU_Pos)              /*!< 0x00080000 */
#define RTC_ALRMBR_MSK2_Pos          (15U)
#define RTC_ALRMBR_MSK2_Msk          (0x1UL << RTC_ALRMBR_MSK2_Pos)            /*!< 0x00008000 */
#define RTC_ALRMBR_MSK2              RTC_ALRMBR_MSK2_Msk
#define RTC_ALRMBR_MNT_Pos           (12U)
#define RTC_ALRMBR_MNT_Msk           (0x7UL << RTC_ALRMBR_MNT_Pos)             /*!< 0x00007000 */
#define RTC_ALRMBR_MNT               RTC_ALRMBR_MNT_Msk
#define RTC_ALRMBR_MNT_0             (0x1UL << RTC_ALRMBR_MNT_Pos)             /*!< 0x00001000 */
#define RTC_ALRMBR_MNT_1             (0x2UL << RTC_ALRMBR_MNT_Pos)             /*!< 0x00002000 */
#define RTC_ALRMBR_MNT_2             (0x4UL << RTC_ALRMBR_MNT_Pos)             /*!< 0x00004000 */
#define RTC_ALRMBR_MNU_Pos           (8U)
#define RTC_ALRMBR_MNU_Msk           (0xFUL << RTC_ALRMBR_MNU_Pos)             /*!< 0x00000F00 */
#define RTC_ALRMBR_MNU               RTC_ALRMBR_MNU_Msk
#define RTC_ALRMBR_MNU_0             (0x1UL << RTC_ALRMBR_MNU_Pos)             /*!< 0x00000100 */
#define RTC_ALRMBR_MNU_1             (0x2UL << RTC_ALRMBR_MNU_Pos)             /*!< 0x00000200 */
#define RTC_ALRMBR_MNU_2             (0x4UL << RTC_ALRMBR_MNU_Pos)             /*!< 0x00000400 */
#define RTC_ALRMBR_MNU_3             (0x8UL << RTC_ALRMBR_MNU_Pos)             /*!< 0x00000800 */
#define RTC_ALRMBR_MSK1_Pos          (7U)
#define RTC_ALRMBR_MSK1_Msk          (0x1UL << RTC_ALRMBR_MSK1_Pos)            /*!< 0x00000080 */
#define RTC_ALRMBR_MSK1              RTC_ALRMBR_MSK1_Msk
#define RTC_ALRMBR_ST_Pos            (4U)
#define RTC_ALRMBR_ST_Msk            (0x7UL << RTC_ALRMBR_ST_Pos)              /*!< 0x00000070 */
#define RTC_ALRMBR_ST                RTC_ALRMBR_ST_Msk
#define RTC_ALRMBR_ST_0              (0x1UL << RTC_ALRMBR_ST_Pos)              /*!< 0x00000010 */
#define RTC_ALRMBR_ST_1              (0x2UL << RTC_ALRMBR_ST_Pos)              /*!< 0x00000020 */
#define RTC_ALRMBR_ST_2              (0x4UL << RTC_ALRMBR_ST_Pos)              /*!< 0x00000040 */
#define RTC_ALRMBR_SU_Pos            (0U)
#define RTC_ALRMBR_SU_Msk            (0xFUL << RTC_ALRMBR_SU_Pos)              /*!< 0x0000000F */
#define RTC_ALRMBR_SU                RTC_ALRMBR_SU_Msk
#define RTC_ALRMBR_SU_0              (0x1UL << RTC_ALRMBR_SU_Pos)              /*!< 0x00000001 */
#define RTC_ALRMBR_SU_1              (0x2UL << RTC_ALRMBR_SU_Pos)              /*!< 0x00000002 */
#define RTC_ALRMBR_SU_2              (0x4UL << RTC_ALRMBR_SU_Pos)              /*!< 0x00000004 */
#define RTC_ALRMBR_SU_3              (0x8UL << RTC_ALRMBR_SU_Pos)              /*!< 0x00000008 */

/********************  Bits definition for RTC_ALRMASSR register  *************/
#define RTC_ALRMBSSR_MASKSS_Pos      (24U)
#define RTC_ALRMBSSR_MASKSS_Msk      (0xFUL << RTC_ALRMBSSR_MASKSS_Pos)        /*!< 0x0F000000 */
#define RTC_ALRMBSSR_MASKSS          RTC_ALRMBSSR_MASKSS_Msk
#define RTC_ALRMBSSR_MASKSS_0        (0x1UL << RTC_ALRMBSSR_MASKSS_Pos)        /*!< 0x01000000 */
#define RTC_ALRMBSSR_MASKSS_1        (0x2UL << RTC_ALRMBSSR_MASKSS_Pos)        /*!< 0x02000000 */
#define RTC_ALRMBSSR_MASKSS_2        (0x4UL << RTC_ALRMBSSR_MASKSS_Pos)        /*!< 0x04000000 */
#define RTC_ALRMBSSR_MASKSS_3        (0x8UL << RTC_ALRMBSSR_MASKSS_Pos)        /*!< 0x08000000 */
#define RTC_ALRMBSSR_SS_Pos          (0U)
#define RTC_ALRMBSSR_SS_Msk          (0x7FFFUL << RTC_ALRMBSSR_SS_Pos)         /*!< 0x00007FFF */
#define RTC_ALRMBSSR_SS              RTC_ALRMBSSR_SS_Msk

/********************  Bits definition for RTC_SR register  *******************/
#define RTC_SR_ITSF_Pos              (5U)
#define RTC_SR_ITSF_Msk              (0x1UL << RTC_SR_ITSF_Pos)                /*!< 0x00000020 */
#define RTC_SR_ITSF                  RTC_SR_ITSF_Msk
#define RTC_SR_TSOVF_Pos             (4U)
#define RTC_SR_TSOVF_Msk             (0x1UL << RTC_SR_TSOVF_Pos)               /*!< 0x00000010 */
#define RTC_SR_TSOVF                 RTC_SR_TSOVF_Msk                          /*!< Timestamp overflow flag > */
#define RTC_SR_TSF_Pos               (3U)
#define RTC_SR_TSF_Msk               (0x1UL << RTC_SR_TSF_Pos)                 /*!< 0x00000008 */
#define RTC_SR_TSF                   RTC_SR_TSF_Msk                            /*!< Timestamp flag > */
#define RTC_SR_WUTF_Pos              (2U)
#define RTC_SR_WUTF_Msk              (0x1UL << RTC_SR_WUTF_Pos)                /*!< 0x00000004 */
#define RTC_SR_WUTF                  RTC_SR_WUTF_Msk                           /*!< Wakeup timer flag > */
#define RTC_SR_ALRBF_Pos             (1U)
#define RTC_SR_ALRBF_Msk             (0x1UL << RTC_SR_ALRBF_Pos)               /*!< 0x00000002 */
#define RTC_SR_ALRBF                 RTC_SR_ALRBF_Msk
#define RTC_SR_ALRAF_Pos             (0U)
#define RTC_SR_ALRAF_Msk             (0x1UL << RTC_SR_ALRAF_Pos)               /*!< 0x00000001 */
#define RTC_SR_ALRAF                 RTC_SR_ALRAF_Msk

/********************  Bits definition for RTC_MISR register  *****************/
#define RTC_MISR_ITSMF_Pos           (5U)
#define RTC_MISR_ITSMF_Msk           (0x1UL << RTC_MISR_ITSMF_Pos)             /*!< 0x00000020 */
#define RTC_MISR_ITSMF               RTC_MISR_ITSMF_Msk
#define RTC_MISR_TSOVMF_Pos          (4U)
#define RTC_MISR_TSOVMF_Msk          (0x1UL << RTC_MISR_TSOVMF_Pos)            /*!< 0x00000010 */
#define RTC_MISR_TSOVMF              RTC_MISR_TSOVMF_Msk                       /*!< Timestamp overflow masked flag > */
#define RTC_MISR_TSMF_Pos            (3U)
#define RTC_MISR_TSMF_Msk            (0x1UL << RTC_MISR_TSMF_Pos)              /*!< 0x00000008 */
#define RTC_MISR_TSMF                RTC_MISR_TSMF_Msk                         /*!< Timestamp masked flag > */
#define RTC_MISR_WUTMF_Pos           (2U)
#define RTC_MISR_WUTMF_Msk           (0x1UL << RTC_MISR_WUTMF_Pos)             /*!< 0x00000004 */
#define RTC_MISR_WUTMF               RTC_MISR_WUTMF_Msk                        /*!< Wakeup timer masked flag > */
#define RTC_MISR_ALRBMF_Pos          (1U)
#define RTC_MISR_ALRBMF_Msk          (0x1UL << RTC_MISR_ALRBMF_Pos)            /*!< 0x00000002 */
#define RTC_MISR_ALRBMF              RTC_MISR_ALRBMF_Msk
#define RTC_MISR_ALRAMF_Pos          (0U)
#define RTC_MISR_ALRAMF_Msk          (0x1UL << RTC_MISR_ALRAMF_Pos)            /*!< 0x00000001 */
#define RTC_MISR_ALRAMF              RTC_MISR_ALRAMF_Msk

/********************  Bits definition for RTC_SCR register  ******************/
#define RTC_SCR_CITSF_Pos            (5U)
#define RTC_SCR_CITSF_Msk            (0x1UL << RTC_SCR_CITSF_Pos)              /*!< 0x00000020 */
#define RTC_SCR_CITSF                RTC_SCR_CITSF_Msk
#define RTC_SCR_CTSOVF_Pos           (4U)
#define RTC_SCR_CTSOVF_Msk           (0x1UL << RTC_SCR_CTSOVF_Pos)             /*!< 0x00000010 */
#define RTC_SCR_CTSOVF               RTC_SCR_CTSOVF_Msk                        /*!< Clear timestamp overflow flag > */
#define RTC_SCR_CTSF_Pos             (3U)
#define RTC_SCR_CTSF_Msk             (0x1UL << RTC_SCR_CTSF_Pos)               /*!< 0x00000008 */
#define RTC_SCR_CTSF                 RTC_SCR_CTSF_Msk                          /*!< Clear timestamp flag > */
#define RTC_SCR_CWUTF_Pos            (2U)
#define RTC_SCR_CWUTF_Msk            (0x1UL << RTC_SCR_CWUTF_Pos)              /*!< 0x00000004 */
#define RTC_SCR_CWUTF                RTC_SCR_CWUTF_Msk                         /*!< Clear wakeup timer flag > */
#define RTC_SCR_CALRBF_Pos           (1U)
#define RTC_SCR_CALRBF_Msk           (0x1UL << RTC_SCR_CALRBF_Pos)             /*!< 0x00000002 */
#define RTC_SCR_CALRBF               RTC_SCR_CALRBF_Msk
#define RTC_SCR_CALRAF_Pos           (0U)
#define RTC_SCR_CALRAF_Msk           (0x1UL << RTC_SCR_CALRAF_Pos)             /*!< 0x00000001 */
#define RTC_SCR_CALRAF               RTC_SCR_CALRAF_Msk


/******************************************************************************/
/*                                                                            */
/*                     Tamper and backup register (TAMP)                      */
/*                                                                            */
/******************************************************************************/
/********************  Bits definition for TAMP_CR1 register  *****************/
#define TAMP_CR1_TAMP1E_Pos          (0U)
#define TAMP_CR1_TAMP1E_Msk          (0x1UL << TAMP_CR1_TAMP1E_Pos)             /*!< 0x00000001 */
#define TAMP_CR1_TAMP1E              TAMP_CR1_TAMP1E_Msk
#define TAMP_CR1_TAMP2E_Pos          (1U)
#define TAMP_CR1_TAMP2E_Msk          (0x1UL << TAMP_CR1_TAMP2E_Pos)             /*!< 0x00000002 */
#define TAMP_CR1_TAMP2E              TAMP_CR1_TAMP2E_Msk
#define TAMP_CR1_ITAMP3E_Pos         (18U)
#define TAMP_CR1_ITAMP3E_Msk         (0x1UL << TAMP_CR1_ITAMP3E_Pos)            /*!< 0x00040000 */
#define TAMP_CR1_ITAMP3E             TAMP_CR1_ITAMP3E_Msk
#define TAMP_CR1_ITAMP4E_Pos         (19U)
#define TAMP_CR1_ITAMP4E_Msk         (0x1UL << TAMP_CR1_ITAMP4E_Pos)            /*!< 0x00080000 */
#define TAMP_CR1_ITAMP4E             TAMP_CR1_ITAMP4E_Msk
#define TAMP_CR1_ITAMP5E_Pos         (20U)
#define TAMP_CR1_ITAMP5E_Msk         (0x1UL << TAMP_CR1_ITAMP5E_Pos)            /*!< 0x00100000 */
#define TAMP_CR1_ITAMP5E             TAMP_CR1_ITAMP5E_Msk
#define TAMP_CR1_ITAMP6E_Pos         (21U)
#define TAMP_CR1_ITAMP6E_Msk         (0x1UL << TAMP_CR1_ITAMP6E_Pos)            /*!< 0x00200000 */
#define TAMP_CR1_ITAMP6E             TAMP_CR1_ITAMP6E_Msk

/********************  Bits definition for TAMP_CR2 register  *****************/
#define TAMP_CR2_TAMP1NOERASE_Pos    (0U)
#define TAMP_CR2_TAMP1NOERASE_Msk    (0x1UL << TAMP_CR2_TAMP1NOERASE_Pos)       /*!< 0x00000001 */
#define TAMP_CR2_TAMP1NOERASE        TAMP_CR2_TAMP1NOERASE_Msk
#define TAMP_CR2_TAMP2NOERASE_Pos    (1U)
#define TAMP_CR2_TAMP2NOERASE_Msk    (0x1UL << TAMP_CR2_TAMP2NOERASE_Pos)       /*!< 0x00000002 */
#define TAMP_CR2_TAMP2NOERASE        TAMP_CR2_TAMP2NOERASE_Msk
#define TAMP_CR2_TAMP1MSK_Pos        (16U)
#define TAMP_CR2_TAMP1MSK_Msk        (0x1UL << TAMP_CR2_TAMP1MSK_Pos)           /*!< 0x00010000 */
#define TAMP_CR2_TAMP1MSK            TAMP_CR2_TAMP1MSK_Msk
#define TAMP_CR2_TAMP2MSK_Pos        (17U)
#define TAMP_CR2_TAMP2MSK_Msk        (0x1UL << TAMP_CR2_TAMP2MSK_Pos)           /*!< 0x00020000 */
#define TAMP_CR2_TAMP2MSK            TAMP_CR2_TAMP2MSK_Msk
#define TAMP_CR2_TAMP1TRG_Pos        (24U)
#define TAMP_CR2_TAMP1TRG_Msk        (0x1UL << TAMP_CR2_TAMP1TRG_Pos)           /*!< 0x01000000 */
#define TAMP_CR2_TAMP1TRG            TAMP_CR2_TAMP1TRG_Msk
#define TAMP_CR2_TAMP2TRG_Pos        (25U)
#define TAMP_CR2_TAMP2TRG_Msk        (0x1UL << TAMP_CR2_TAMP2TRG_Pos)           /*!< 0x02000000 */
#define TAMP_CR2_TAMP2TRG            TAMP_CR2_TAMP2TRG_Msk

/********************  Bits definition for TAMP_FLTCR register  ***************/
#define TAMP_FLTCR_TAMPFREQ_0        0x00000001U
#define TAMP_FLTCR_TAMPFREQ_1        0x00000002U
#define TAMP_FLTCR_TAMPFREQ_2        0x00000004U
#define TAMP_FLTCR_TAMPFREQ_Pos      (0U)
#define TAMP_FLTCR_TAMPFREQ_Msk      (0x7UL << TAMP_FLTCR_TAMPFREQ_Pos)         /*!< 0x00000007 */
#define TAMP_FLTCR_TAMPFREQ          TAMP_FLTCR_TAMPFREQ_Msk
#define TAMP_FLTCR_TAMPFLT_0         0x00000008U
#define TAMP_FLTCR_TAMPFLT_1         0x00000010U
#define TAMP_FLTCR_TAMPFLT_Pos       (3U)
#define TAMP_FLTCR_TAMPFLT_Msk       (0x3UL << TAMP_FLTCR_TAMPFLT_Pos)          /*!< 0x00000018 */
#define TAMP_FLTCR_TAMPFLT           TAMP_FLTCR_TAMPFLT_Msk
#define TAMP_FLTCR_TAMPPRCH_0        0x00000020U
#define TAMP_FLTCR_TAMPPRCH_1        0x00000040U
#define TAMP_FLTCR_TAMPPRCH_Pos      (5U)
#define TAMP_FLTCR_TAMPPRCH_Msk      (0x3UL << TAMP_FLTCR_TAMPPRCH_Pos)         /*!< 0x00000060 */
#define TAMP_FLTCR_TAMPPRCH          TAMP_FLTCR_TAMPPRCH_Msk
#define TAMP_FLTCR_TAMPPUDIS_Pos     (7U)
#define TAMP_FLTCR_TAMPPUDIS_Msk     (0x1UL << TAMP_FLTCR_TAMPPUDIS_Pos)        /*!< 0x00000080 */
#define TAMP_FLTCR_TAMPPUDIS         TAMP_FLTCR_TAMPPUDIS_Msk

/********************  Bits definition for TAMP_IER register  *****************/
#define TAMP_IER_TAMP1IE_Pos         (0U)
#define TAMP_IER_TAMP1IE_Msk         (0x1UL << TAMP_IER_TAMP1IE_Pos)            /*!< 0x00000001 */
#define TAMP_IER_TAMP1IE             TAMP_IER_TAMP1IE_Msk
#define TAMP_IER_TAMP2IE_Pos         (1U)
#define TAMP_IER_TAMP2IE_Msk         (0x1UL << TAMP_IER_TAMP2IE_Pos)            /*!< 0x00000002 */
#define TAMP_IER_TAMP2IE             TAMP_IER_TAMP2IE_Msk
#define TAMP_IER_ITAMP3IE_Pos        (18U)
#define TAMP_IER_ITAMP3IE_Msk        (0x1UL << TAMP_IER_ITAMP3IE_Pos)           /*!< 0x00040000 */
#define TAMP_IER_ITAMP3IE            TAMP_IER_ITAMP3IE_Msk
#define TAMP_IER_ITAMP4IE_Pos        (19U)
#define TAMP_IER_ITAMP4IE_Msk        (0x1UL << TAMP_IER_ITAMP4IE_Pos)           /*!< 0x00080000 */
#define TAMP_IER_ITAMP4IE            TAMP_IER_ITAMP4IE_Msk
#define TAMP_IER_ITAMP5IE_Pos        (20U)
#define TAMP_IER_ITAMP5IE_Msk        (0x1UL << TAMP_IER_ITAMP5IE_Pos)           /*!< 0x00100000 */
#define TAMP_IER_ITAMP5IE            TAMP_IER_ITAMP5IE_Msk
#define TAMP_IER_ITAMP6IE_Pos        (21U)
#define TAMP_IER_ITAMP6IE_Msk        (0x1UL << TAMP_IER_ITAMP6IE_Pos)           /*!< 0x00200000 */
#define TAMP_IER_ITAMP6IE            TAMP_IER_ITAMP6IE_Msk

/********************  Bits definition for TAMP_SR register  ******************/
#define TAMP_SR_TAMP1F_Pos           (0U)
#define TAMP_SR_TAMP1F_Msk           (0x1UL << TAMP_SR_TAMP1F_Pos)              /*!< 0x00000001 */
#define TAMP_SR_TAMP1F               TAMP_SR_TAMP1F_Msk
#define TAMP_SR_TAMP2F_Pos           (1U)
#define TAMP_SR_TAMP2F_Msk           (0x1UL << TAMP_SR_TAMP2F_Pos)              /*!< 0x00000002 */
#define TAMP_SR_TAMP2F               TAMP_SR_TAMP2F_Msk
#define TAMP_SR_ITAMP3F_Pos          (18U)
#define TAMP_SR_ITAMP3F_Msk          (0x1UL << TAMP_SR_ITAMP3F_Pos)             /*!< 0x00040000 */
#define TAMP_SR_ITAMP3F              TAMP_SR_ITAMP3F_Msk
#define TAMP_SR_ITAMP4F_Pos          (19U)
#define TAMP_SR_ITAMP4F_Msk          (0x1UL << TAMP_SR_ITAMP4F_Pos)             /*!< 0x00080000 */
#define TAMP_SR_ITAMP4F              TAMP_SR_ITAMP4F_Msk
#define TAMP_SR_ITAMP5F_Pos          (20U)
#define TAMP_SR_ITAMP5F_Msk          (0x1UL << TAMP_SR_ITAMP5F_Pos)             /*!< 0x00100000 */
#define TAMP_SR_ITAMP5F              TAMP_SR_ITAMP5F_Msk
#define TAMP_SR_ITAMP6F_Pos          (21U)
#define TAMP_SR_ITAMP6F_Msk          (0x1UL << TAMP_SR_ITAMP6F_Pos)             /*!< 0x00200000 */
#define TAMP_SR_ITAMP6F              TAMP_SR_ITAMP6F_Msk

/********************  Bits definition for TAMP_MISR register  ****************/
#define TAMP_MISR_TAMP1MF_Pos        (0U)
#define TAMP_MISR_TAMP1MF_Msk        (0x1UL << TAMP_MISR_TAMP1MF_Pos)           /*!< 0x00000001 */
#define TAMP_MISR_TAMP1MF            TAMP_MISR_TAMP1MF_Msk
#define TAMP_MISR_TAMP2MF_Pos        (1U)
#define TAMP_MISR_TAMP2MF_Msk        (0x1UL << TAMP_MISR_TAMP2MF_Pos)           /*!< 0x00000002 */
#define TAMP_MISR_TAMP2MF            TAMP_MISR_TAMP2MF_Msk
#define TAMP_MISR_ITAMP3MF_Pos       (18U)
#define TAMP_MISR_ITAMP3MF_Msk       (0x1UL << TAMP_MISR_ITAMP3MF_Pos)          /*!< 0x00040000 */
#define TAMP_MISR_ITAMP3MF           TAMP_MISR_ITAMP3MF_Msk
#define TAMP_MISR_ITAMP4MF_Pos       (19U)
#define TAMP_MISR_ITAMP4MF_Msk       (0x1UL << TAMP_MISR_ITAMP4MF_Pos)          /*!< 0x00080000 */
#define TAMP_MISR_ITAMP4MF           TAMP_MISR_ITAMP4MF_Msk
#define TAMP_MISR_ITAMP5MF_Pos       (20U)
#define TAMP_MISR_ITAMP5MF_Msk       (0x1UL << TAMP_MISR_ITAMP5MF_Pos)          /*!< 0x00100000 */
#define TAMP_MISR_ITAMP5MF           TAMP_MISR_ITAMP5MF_Msk
#define TAMP_MISR_ITAMP6MF_Pos       (21U)
#define TAMP_MISR_ITAMP6MF_Msk       (0x1UL << TAMP_MISR_ITAMP6MF_Pos)          /*!< 0x00200000 */
#define TAMP_MISR_ITAMP6MF           TAMP_MISR_ITAMP6MF_Msk

/********************  Bits definition for TAMP_SCR register  *****************/
#define TAMP_SCR_CTAMP1F_Pos         (0U)
#define TAMP_SCR_CTAMP1F_Msk         (0x1UL << TAMP_SCR_CTAMP1F_Pos)            /*!< 0x00000001 */
#define TAMP_SCR_CTAMP1F             TAMP_SCR_CTAMP1F_Msk
#define TAMP_SCR_CTAMP2F_Pos         (1U)
#define TAMP_SCR_CTAMP2F_Msk         (0x1UL << TAMP_SCR_CTAMP2F_Pos)            /*!< 0x00000002 */
#define TAMP_SCR_CTAMP2F             TAMP_SCR_CTAMP2F_Msk
#define TAMP_SCR_CITAMP3F_Pos        (18U)
#define TAMP_SCR_CITAMP3F_Msk        (0x1UL << TAMP_SCR_CITAMP3F_Pos)           /*!< 0x00040000 */
#define TAMP_SCR_CITAMP3F            TAMP_SCR_CITAMP3F_Msk
#define TAMP_SCR_CITAMP4F_Pos        (19U)
#define TAMP_SCR_CITAMP4F_Msk        (0x1UL << TAMP_SCR_CITAMP4F_Pos)           /*!< 0x00080000 */
#define TAMP_SCR_CITAMP4F            TAMP_SCR_CITAMP4F_Msk
#define TAMP_SCR_CITAMP5F_Pos        (20U)
#define TAMP_SCR_CITAMP5F_Msk        (0x1UL << TAMP_SCR_CITAMP5F_Pos)           /*!< 0x00100000 */
#define TAMP_SCR_CITAMP5F            TAMP_SCR_CITAMP5F_Msk
#define TAMP_SCR_CITAMP6F_Pos        (21U)
#define TAMP_SCR_CITAMP6F_Msk        (0x1UL << TAMP_SCR_CITAMP6F_Pos)           /*!< 0x00200000 */
#define TAMP_SCR_CITAMP6F            TAMP_SCR_CITAMP6F_Msk

/********************  Bits definition for TAMP_BKP0R register  ***************/
#define TAMP_BKP0R_Pos               (0U)
#define TAMP_BKP0R_Msk               (0xFFFFFFFFUL << TAMP_BKP0R_Pos)           /*!< 0xFFFFFFFF */
#define TAMP_BKP0R                   TAMP_BKP0R_Msk

/********************  Bits definition for TAMP_BKP1R register  ***************/
#define TAMP_BKP1R_Pos               (0U)
#define TAMP_BKP1R_Msk               (0xFFFFFFFFUL << TAMP_BKP1R_Pos)           /*!< 0xFFFFFFFF */
#define TAMP_BKP1R                   TAMP_BKP1R_Msk

/********************  Bits definition for TAMP_BKP2R register  ***************/
#define TAMP_BKP2R_Pos               (0U)
#define TAMP_BKP2R_Msk               (0xFFFFFFFFUL << TAMP_BKP2R_Pos)           /*!< 0xFFFFFFFF */
#define TAMP_BKP2R                   TAMP_BKP2R_Msk

/********************  Bits definition for TAMP_BKP3R register  ***************/
#define TAMP_BKP3R_Pos               (0U)
#define TAMP_BKP3R_Msk               (0xFFFFFFFFUL << TAMP_BKP3R_Pos)           /*!< 0xFFFFFFFF */
#define TAMP_BKP3R                   TAMP_BKP3R_Msk

/********************  Bits definition for TAMP_BKP4R register  ***************/
#define TAMP_BKP4R_Pos               (0U)
#define TAMP_BKP4R_Msk               (0xFFFFFFFFUL << TAMP_BKP4R_Pos)           /*!< 0xFFFFFFFF */
#define TAMP_BKP4R                   TAMP_BKP4R_Msk


/******************************************************************************/
/*                                                                            */
/*                        Serial Peripheral Interface (SPI)                   */
/*                                                                            */
/******************************************************************************/
/*
 * @brief Specific device feature definitions (not present on all devices in the STM32G0 serie)
 */
#define SPI_I2S_SUPPORT                       /*!< I2S support */

/*******************  Bit definition for SPI_CR1 register  ********************/
#define SPI_CR1_CPHA_Pos            (0U)
#define SPI_CR1_CPHA_Msk            (0x1UL << SPI_CR1_CPHA_Pos)                /*!< 0x00000001 */
#define SPI_CR1_CPHA                SPI_CR1_CPHA_Msk                           /*!<Clock Phase      */
#define SPI_CR1_CPOL_Pos            (1U)
#define SPI_CR1_CPOL_Msk            (0x1UL << SPI_CR1_CPOL_Pos)                /*!< 0x00000002 */
#define SPI_CR1_CPOL                SPI_CR1_CPOL_Msk                           /*!<Clock Polarity   */
#define SPI_CR1_MSTR_Pos            (2U)
#define SPI_CR1_MSTR_Msk            (0x1UL << SPI_CR1_MSTR_Pos)                /*!< 0x00000004 */
#define SPI_CR1_MSTR                SPI_CR1_MSTR_Msk                           /*!<Master Selection */

#define SPI_CR1_BR_Pos              (3U)
#define SPI_CR1_BR_Msk              (0x7UL << SPI_CR1_BR_Pos)                  /*!< 0x00000038 */
#define SPI_CR1_BR                  SPI_CR1_BR_Msk                             /*!<BR[2:0] bits (Baud Rate Control) */
#define SPI_CR1_BR_0                (0x1UL << SPI_CR1_BR_Pos)                  /*!< 0x00000008 */
#define SPI_CR1_BR_1                (0x2UL << SPI_CR1_BR_Pos)                  /*!< 0x00000010 */
#define SPI_CR1_BR_2                (0x4UL << SPI_CR1_BR_Pos)                  /*!< 0x00000020 */

#define SPI_CR1_SPE_Pos             (6U)
#define SPI_CR1_SPE_Msk             (0x1UL << SPI_CR1_SPE_Pos)                 /*!< 0x00000040 */
#define SPI_CR1_SPE                 SPI_CR1_SPE_Msk                            /*!<SPI Enable                          */
#define SPI_CR1_LSBFIRST_Pos        (7U)
#define SPI_CR1_LSBFIRST_Msk        (0x1UL << SPI_CR1_LSBFIRST_Pos)            /*!< 0x00000080 */
#define SPI_CR1_LSBFIRST            SPI_CR1_LSBFIRST_Msk                       /*!<Frame Format                        */
#define SPI_CR1_SSI_Pos             (8U)
#define SPI_CR1_SSI_Msk             (0x1UL << SPI_CR1_SSI_Pos)                 /*!< 0x00000100 */
#define SPI_CR1_SSI                 SPI_CR1_SSI_Msk                            /*!<Internal slave select               */
#define SPI_CR1_SSM_Pos             (9U)
#define SPI_CR1_SSM_Msk             (0x1UL << SPI_CR1_SSM_Pos)                 /*!< 0x00000200 */
#define SPI_CR1_SSM                 SPI_CR1_SSM_Msk                            /*!<Software slave management           */
#define SPI_CR1_RXONLY_Pos          (10U)
#define SPI_CR1_RXONLY_Msk          (0x1UL << SPI_CR1_RXONLY_Pos)              /*!< 0x00000400 */
#define SPI_CR1_RXONLY              SPI_CR1_RXONLY_Msk                         /*!<Receive only                        */
#define SPI_CR1_CRCL_Pos            (11U)
#define SPI_CR1_CRCL_Msk            (0x1UL << SPI_CR1_CRCL_Pos)                /*!< 0x00000800 */
#define SPI_CR1_CRCL                SPI_CR1_CRCL_Msk                           /*!< CRC Length */
#define SPI_CR1_CRCNEXT_Pos         (12U)
#define SPI_CR1_CRCNEXT_Msk         (0x1UL << SPI_CR1_CRCNEXT_Pos)             /*!< 0x00001000 */
#define SPI_CR1_CRCNEXT             SPI_CR1_CRCNEXT_Msk                        /*!<Transmit CRC next                   */
#define SPI_CR1_CRCEN_Pos           (13U)
#define SPI_CR1_CRCEN_Msk           (0x1UL << SPI_CR1_CRCEN_Pos)               /*!< 0x00002000 */
#define SPI_CR1_CRCEN               SPI_CR1_CRCEN_Msk                          /*!<Hardware CRC calculation enable     */
#define SPI_CR1_BIDIOE_Pos          (14U)
#define SPI_CR1_BIDIOE_Msk          (0x1UL << SPI_CR1_BIDIOE_Pos)              /*!< 0x00004000 */
#define SPI_CR1_BIDIOE              SPI_CR1_BIDIOE_Msk                         /*!<Output enable in bidirectional mode */
#define SPI_CR1_BIDIMODE_Pos        (15U)
#define SPI_CR1_BIDIMODE_Msk        (0x1UL << SPI_CR1_BIDIMODE_Pos)            /*!< 0x00008000 */
#define SPI_CR1_BIDIMODE            SPI_CR1_BIDIMODE_Msk                       /*!<Bidirectional data mode enable      */

/*******************  Bit definition for SPI_CR2 register  ********************/
#define SPI_CR2_RXDMAEN_Pos         (0U)
#define SPI_CR2_RXDMAEN_Msk         (0x1UL << SPI_CR2_RXDMAEN_Pos)             /*!< 0x00000001 */
#define SPI_CR2_RXDMAEN             SPI_CR2_RXDMAEN_Msk                        /*!< Rx Buffer DMA Enable */
#define SPI_CR2_TXDMAEN_Pos         (1U)
#define SPI_CR2_TXDMAEN_Msk         (0x1UL << SPI_CR2_TXDMAEN_Pos)             /*!< 0x00000002 */
#define SPI_CR2_TXDMAEN             SPI_CR2_TXDMAEN_Msk                        /*!< Tx Buffer DMA Enable */
#define SPI_CR2_SSOE_Pos            (2U)
#define SPI_CR2_SSOE_Msk            (0x1UL << SPI_CR2_SSOE_Pos)                /*!< 0x00000004 */
#define SPI_CR2_SSOE                SPI_CR2_SSOE_Msk                           /*!< SS Output Enable */
#define SPI_CR2_NSSP_Pos            (3U)
#define SPI_CR2_NSSP_Msk            (0x1UL << SPI_CR2_NSSP_Pos)                /*!< 0x00000008 */
#define SPI_CR2_NSSP                SPI_CR2_NSSP_Msk                           /*!< NSS pulse management Enable */
#define SPI_CR2_FRF_Pos             (4U)
#define SPI_CR2_FRF_Msk             (0x1UL << SPI_CR2_FRF_Pos)                 /*!< 0x00000010 */
#define SPI_CR2_FRF                 SPI_CR2_FRF_Msk                            /*!< Frame Format Enable */
#define SPI_CR2_ERRIE_Pos           (5U)
#define SPI_CR2_ERRIE_Msk           (0x1UL << SPI_CR2_ERRIE_Pos)               /*!< 0x00000020 */
#define SPI_CR2_ERRIE               SPI_CR2_ERRIE_Msk                          /*!< Error Interrupt Enable */
#define SPI_CR2_RXNEIE_Pos          (6U)
#define SPI_CR2_RXNEIE_Msk          (0x1UL << SPI_CR2_RXNEIE_Pos)              /*!< 0x00000040 */
#define SPI_CR2_RXNEIE              SPI_CR2_RXNEIE_Msk                         /*!< RX buffer Not Empty Interrupt Enable */
#define SPI_CR2_TXEIE_Pos           (7U)
#define SPI_CR2_TXEIE_Msk           (0x1UL << SPI_CR2_TXEIE_Pos)               /*!< 0x00000080 */
#define SPI_CR2_TXEIE               SPI_CR2_TXEIE_Msk                          /*!< Tx buffer Empty Interrupt Enable */
#define SPI_CR2_DS_Pos              (8U)
#define SPI_CR2_DS_Msk              (0xFUL << SPI_CR2_DS_Pos)                  /*!< 0x00000F00 */
#define SPI_CR2_DS                  SPI_CR2_DS_Msk                             /*!< DS[3:0] Data Size */
#define SPI_CR2_DS_0                (0x1UL << SPI_CR2_DS_Pos)                  /*!< 0x00000100 */
#define SPI_CR2_DS_1                (0x2UL << SPI_CR2_DS_Pos)                  /*!< 0x00000200 */
#define SPI_CR2_DS_2                (0x4UL << SPI_CR2_DS_Pos)                  /*!< 0x00000400 */
#define SPI_CR2_DS_3                (0x8UL << SPI_CR2_DS_Pos)                  /*!< 0x00000800 */
#define SPI_CR2_FRXTH_Pos           (12U)
#define SPI_CR2_FRXTH_Msk           (0x1UL << SPI_CR2_FRXTH_Pos)               /*!< 0x00001000 */
#define SPI_CR2_FRXTH               SPI_CR2_FRXTH_Msk                          /*!< FIFO reception Threshold */
#define SPI_CR2_LDMARX_Pos          (13U)
#define SPI_CR2_LDMARX_Msk          (0x1UL << SPI_CR2_LDMARX_Pos)              /*!< 0x00002000 */
#define SPI_CR2_LDMARX              SPI_CR2_LDMARX_Msk                         /*!< Last DMA transfer for reception */
#define SPI_CR2_LDMATX_Pos          (14U)
#define SPI_CR2_LDMATX_Msk          (0x1UL << SPI_CR2_LDMATX_Pos)              /*!< 0x00004000 */
#define SPI_CR2_LDMATX              SPI_CR2_LDMATX_Msk                         /*!< Last DMA transfer for transmission */

/********************  Bit definition for SPI_SR register  ********************/
#define SPI_SR_RXNE_Pos             (0U)
#define SPI_SR_RXNE_Msk             (0x1UL << SPI_SR_RXNE_Pos)                 /*!< 0x00000001 */
#define SPI_SR_RXNE                 SPI_SR_RXNE_Msk                            /*!< Receive buffer Not Empty */
#define SPI_SR_TXE_Pos              (1U)
#define SPI_SR_TXE_Msk              (0x1UL << SPI_SR_TXE_Pos)                  /*!< 0x00000002 */
#define SPI_SR_TXE                  SPI_SR_TXE_Msk                             /*!< Transmit buffer Empty */
#define SPI_SR_CHSIDE_Pos           (2U)
#define SPI_SR_CHSIDE_Msk           (0x1UL << SPI_SR_CHSIDE_Pos)               /*!< 0x00000004 */
#define SPI_SR_CHSIDE               SPI_SR_CHSIDE_Msk                          /*!< Channel side */
#define SPI_SR_UDR_Pos              (3U)
#define SPI_SR_UDR_Msk              (0x1UL << SPI_SR_UDR_Pos)                  /*!< 0x00000008 */
#define SPI_SR_UDR                  SPI_SR_UDR_Msk                             /*!< Underrun flag */
#define SPI_SR_CRCERR_Pos           (4U)
#define SPI_SR_CRCERR_Msk           (0x1UL << SPI_SR_CRCERR_Pos)               /*!< 0x00000010 */
#define SPI_SR_CRCERR               SPI_SR_CRCERR_Msk                          /*!< CRC Error flag */
#define SPI_SR_MODF_Pos             (5U)
#define SPI_SR_MODF_Msk             (0x1UL << SPI_SR_MODF_Pos)                 /*!< 0x00000020 */
#define SPI_SR_MODF                 SPI_SR_MODF_Msk                            /*!< Mode fault */
#define SPI_SR_OVR_Pos              (6U)
#define SPI_SR_OVR_Msk              (0x1UL << SPI_SR_OVR_Pos)                  /*!< 0x00000040 */
#define SPI_SR_OVR                  SPI_SR_OVR_Msk                             /*!< Overrun flag */
#define SPI_SR_BSY_Pos              (7U)
#define SPI_SR_BSY_Msk              (0x1UL << SPI_SR_BSY_Pos)                  /*!< 0x00000080 */
#define SPI_SR_BSY                  SPI_SR_BSY_Msk                             /*!< Busy flag */
#define SPI_SR_FRE_Pos              (8U)
#define SPI_SR_FRE_Msk              (0x1UL << SPI_SR_FRE_Pos)                  /*!< 0x00000100 */
#define SPI_SR_FRE                  SPI_SR_FRE_Msk                             /*!< TI frame format error */
#define SPI_SR_FRLVL_Pos            (9U)
#define SPI_SR_FRLVL_Msk            (0x3UL << SPI_SR_FRLVL_Pos)                /*!< 0x00000600 */
#define SPI_SR_FRLVL                SPI_SR_FRLVL_Msk                           /*!< FIFO Reception Level */
#define SPI_SR_FRLVL_0              (0x1UL << SPI_SR_FRLVL_Pos)                /*!< 0x00000200 */
#define SPI_SR_FRLVL_1              (0x2UL << SPI_SR_FRLVL_Pos)                /*!< 0x00000400 */
#define SPI_SR_FTLVL_Pos            (11U)
#define SPI_SR_FTLVL_Msk            (0x3UL << SPI_SR_FTLVL_Pos)                /*!< 0x00001800 */
#define SPI_SR_FTLVL                SPI_SR_FTLVL_Msk                           /*!< FIFO Transmission Level */
#define SPI_SR_FTLVL_0              (0x1UL << SPI_SR_FTLVL_Pos)                /*!< 0x00000800 */
#define SPI_SR_FTLVL_1              (0x2UL << SPI_SR_FTLVL_Pos)                /*!< 0x00001000 */

/********************  Bit definition for SPI_DR register  ********************/
#define SPI_DR_DR_Pos               (0U)
#define SPI_DR_DR_Msk               (0xFFFFUL << SPI_DR_DR_Pos)                /*!< 0x0000FFFF */
#define SPI_DR_DR                   SPI_DR_DR_Msk                              /*!<Data Register           */

/*******************  Bit definition for SPI_CRCPR register  ******************/
#define SPI_CRCPR_CRCPOLY_Pos       (0U)
#define SPI_CRCPR_CRCPOLY_Msk       (0xFFFFUL << SPI_CRCPR_CRCPOLY_Pos)        /*!< 0x0000FFFF */
#define SPI_CRCPR_CRCPOLY           SPI_CRCPR_CRCPOLY_Msk                      /*!<CRC polynomial register */

/******************  Bit definition for SPI_RXCRCR register  ******************/
#define SPI_RXCRCR_RXCRC_Pos        (0U)
#define SPI_RXCRCR_RXCRC_Msk        (0xFFFFUL << SPI_RXCRCR_RXCRC_Pos)         /*!< 0x0000FFFF */
#define SPI_RXCRCR_RXCRC            SPI_RXCRCR_RXCRC_Msk                       /*!<Rx CRC Register         */

/******************  Bit definition for SPI_TXCRCR register  ******************/
#define SPI_TXCRCR_TXCRC_Pos        (0U)
#define SPI_TXCRCR_TXCRC_Msk        (0xFFFFUL << SPI_TXCRCR_TXCRC_Pos)         /*!< 0x0000FFFF */
#define SPI_TXCRCR_TXCRC            SPI_TXCRCR_TXCRC_Msk                       /*!<Tx CRC Register         */

/******************  Bit definition for SPI_I2SCFGR register  *****************/
#define SPI_I2SCFGR_CHLEN_Pos       (0U)
#define SPI_I2SCFGR_CHLEN_Msk       (0x1UL << SPI_I2SCFGR_CHLEN_Pos)           /*!< 0x00000001 */
#define SPI_I2SCFGR_CHLEN           SPI_I2SCFGR_CHLEN_Msk                      /*!<Channel length (number of bits per audio channel) */
#define SPI_I2SCFGR_DATLEN_Pos      (1U)
#define SPI_I2SCFGR_DATLEN_Msk      (0x3UL << SPI_I2SCFGR_DATLEN_Pos)          /*!< 0x00000006 */
#define SPI_I2SCFGR_DATLEN          SPI_I2SCFGR_DATLEN_Msk                     /*!<DATLEN[1:0] bits (Data length to be transferred) */
#define SPI_I2SCFGR_DATLEN_0        (0x1UL << SPI_I2SCFGR_DATLEN_Pos)          /*!< 0x00000002 */
#define SPI_I2SCFGR_DATLEN_1        (0x2UL << SPI_I2SCFGR_DATLEN_Pos)          /*!< 0x00000004 */
#define SPI_I2SCFGR_CKPOL_Pos       (3U)
#define SPI_I2SCFGR_CKPOL_Msk       (0x1UL << SPI_I2SCFGR_CKPOL_Pos)           /*!< 0x00000008 */
#define SPI_I2SCFGR_CKPOL           SPI_I2SCFGR_CKPOL_Msk                      /*!<steady state clock polarity */
#define SPI_I2SCFGR_I2SSTD_Pos      (4U)
#define SPI_I2SCFGR_I2SSTD_Msk      (0x3UL << SPI_I2SCFGR_I2SSTD_Pos)          /*!< 0x00000030 */
#define SPI_I2SCFGR_I2SSTD          SPI_I2SCFGR_I2SSTD_Msk                     /*!<I2SSTD[1:0] bits (I2S standard selection) */
#define SPI_I2SCFGR_I2SSTD_0        (0x1UL << SPI_I2SCFGR_I2SSTD_Pos)          /*!< 0x00000010 */
#define SPI_I2SCFGR_I2SSTD_1        (0x2UL << SPI_I2SCFGR_I2SSTD_Pos)          /*!< 0x00000020 */
#define SPI_I2SCFGR_PCMSYNC_Pos     (7U)
#define SPI_I2SCFGR_PCMSYNC_Msk     (0x1UL << SPI_I2SCFGR_PCMSYNC_Pos)         /*!< 0x00000080 */
#define SPI_I2SCFGR_PCMSYNC         SPI_I2SCFGR_PCMSYNC_Msk                    /*!<PCM frame synchronization */
#define SPI_I2SCFGR_I2SCFG_Pos      (8U)
#define SPI_I2SCFGR_I2SCFG_Msk      (0x3UL << SPI_I2SCFGR_I2SCFG_Pos)          /*!< 0x00000300 */
#define SPI_I2SCFGR_I2SCFG          SPI_I2SCFGR_I2SCFG_Msk                     /*!<I2SCFG[1:0] bits (I2S configuration mode) */
#define SPI_I2SCFGR_I2SCFG_0        (0x1UL << SPI_I2SCFGR_I2SCFG_Pos)          /*!< 0x00000100 */
#define SPI_I2SCFGR_I2SCFG_1        (0x2UL << SPI_I2SCFGR_I2SCFG_Pos)          /*!< 0x00000200 */
#define SPI_I2SCFGR_I2SE_Pos        (10U)
#define SPI_I2SCFGR_I2SE_Msk        (0x1UL << SPI_I2SCFGR_I2SE_Pos)            /*!< 0x00000400 */
#define SPI_I2SCFGR_I2SE            SPI_I2SCFGR_I2SE_Msk                       /*!<I2S Enable */
#define SPI_I2SCFGR_I2SMOD_Pos      (11U)
#define SPI_I2SCFGR_I2SMOD_Msk      (0x1UL << SPI_I2SCFGR_I2SMOD_Pos)          /*!< 0x00000800 */
#define SPI_I2SCFGR_I2SMOD          SPI_I2SCFGR_I2SMOD_Msk                     /*!<I2S mode selection */
#define SPI_I2SCFGR_ASTRTEN_Pos     (12U)
#define SPI_I2SCFGR_ASTRTEN_Msk     (0x1UL << SPI_I2SCFGR_ASTRTEN_Pos)         /*!< 0x00001000 */
#define SPI_I2SCFGR_ASTRTEN         SPI_I2SCFGR_ASTRTEN_Msk                    /*!<Asynchronous start enable */

/******************  Bit definition for SPI_I2SPR register  *******************/
#define SPI_I2SPR_I2SDIV_Pos        (0U)
#define SPI_I2SPR_I2SDIV_Msk        (0xFFUL << SPI_I2SPR_I2SDIV_Pos)           /*!< 0x000000FF */
#define SPI_I2SPR_I2SDIV            SPI_I2SPR_I2SDIV_Msk                       /*!<I2S Linear prescaler */
#define SPI_I2SPR_ODD_Pos           (8U)
#define SPI_I2SPR_ODD_Msk           (0x1UL << SPI_I2SPR_ODD_Pos)               /*!< 0x00000100 */
#define SPI_I2SPR_ODD               SPI_I2SPR_ODD_Msk                          /*!<Odd factor for the prescaler */
#define SPI_I2SPR_MCKOE_Pos         (9U)
#define SPI_I2SPR_MCKOE_Msk         (0x1UL << SPI_I2SPR_MCKOE_Pos)             /*!< 0x00000200 */
#define SPI_I2SPR_MCKOE             SPI_I2SPR_MCKOE_Msk                        /*!<Master Clock Output Enable */

/******************************************************************************/
/*                                                                            */
/*                                 SYSCFG                                     */
/*                                                                            */
/******************************************************************************/
/*****************  Bit definition for SYSCFG_CFGR1 register  ****************/
#define SYSCFG_CFGR1_MEM_MODE_Pos             (0U)
#define SYSCFG_CFGR1_MEM_MODE_Msk             (0x3UL << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000003 */
#define SYSCFG_CFGR1_MEM_MODE                 SYSCFG_CFGR1_MEM_MODE_Msk            /*!< SYSCFG_Memory Remap Config */
#define SYSCFG_CFGR1_MEM_MODE_0               (0x1UL << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000001 */
#define SYSCFG_CFGR1_MEM_MODE_1               (0x2UL << SYSCFG_CFGR1_MEM_MODE_Pos) /*!< 0x00000002 */
#define SYSCFG_CFGR1_PA11_RMP_Pos             (3U)
#define SYSCFG_CFGR1_PA11_RMP_Msk             (0x1UL << SYSCFG_CFGR1_PA11_RMP_Pos) /*!< 0x00000008 */
#define SYSCFG_CFGR1_PA11_RMP                 SYSCFG_CFGR1_PA11_RMP_Msk            /*!< PA11 Remap */
#define SYSCFG_CFGR1_PA12_RMP_Pos             (4U)
#define SYSCFG_CFGR1_PA12_RMP_Msk             (0x1UL << SYSCFG_CFGR1_PA12_RMP_Pos) /*!< 0x00000010 */
#define SYSCFG_CFGR1_PA12_RMP                 SYSCFG_CFGR1_PA12_RMP_Msk            /*!< PA12 Remap */
#define SYSCFG_CFGR1_IR_POL_Pos               (5U)
#define SYSCFG_CFGR1_IR_POL_Msk               (0x1UL << SYSCFG_CFGR1_IR_POL_Pos) /*!< 0x00000020 */
#define SYSCFG_CFGR1_IR_POL                   SYSCFG_CFGR1_IR_POL_Msk            /*!< IROut Polarity Selection */
#define SYSCFG_CFGR1_IR_MOD_Pos               (6U)
#define SYSCFG_CFGR1_IR_MOD_Msk               (0x3UL << SYSCFG_CFGR1_IR_MOD_Pos) /*!< 0x000000C0 */
#define SYSCFG_CFGR1_IR_MOD                   SYSCFG_CFGR1_IR_MOD_Msk            /*!< IRDA Modulation Envelope signal source selection */
#define SYSCFG_CFGR1_IR_MOD_0                 (0x1UL << SYSCFG_CFGR1_IR_MOD_Pos) /*!< 0x00000040 */
#define SYSCFG_CFGR1_IR_MOD_1                 (0x2UL << SYSCFG_CFGR1_IR_MOD_Pos) /*!< 0x00000080 */
#define SYSCFG_CFGR1_BOOSTEN_Pos              (8U)
#define SYSCFG_CFGR1_BOOSTEN_Msk              (0x1UL << SYSCFG_CFGR1_BOOSTEN_Pos) /*!< 0x00000100 */
#define SYSCFG_CFGR1_BOOSTEN                  SYSCFG_CFGR1_BOOSTEN_Msk            /*!< I/O analog switch voltage booster enable */
#define SYSCFG_CFGR1_UCPD1_STROBE_Pos         (9U)
#define SYSCFG_CFGR1_UCPD1_STROBE_Msk         (0x1UL << SYSCFG_CFGR1_UCPD1_STROBE_Pos) /*!< 0x00000200 */
#define SYSCFG_CFGR1_UCPD1_STROBE             SYSCFG_CFGR1_UCPD1_STROBE_Msk            /*!< Strobe signal bit for UCPD1 */
#define SYSCFG_CFGR1_UCPD2_STROBE_Pos         (10U)
#define SYSCFG_CFGR1_UCPD2_STROBE_Msk         (0x1UL << SYSCFG_CFGR1_UCPD2_STROBE_Pos) /*!< 0x00000400 */
#define SYSCFG_CFGR1_UCPD2_STROBE             SYSCFG_CFGR1_UCPD2_STROBE_Msk            /*!< Strobe signal bit for UCPD2 */
#define SYSCFG_CFGR1_I2C_PB6_FMP_Pos          (16U)
#define SYSCFG_CFGR1_I2C_PB6_FMP_Msk          (0x1UL << SYSCFG_CFGR1_I2C_PB6_FMP_Pos)  /*!< 0x00010000 */
#define SYSCFG_CFGR1_I2C_PB6_FMP              SYSCFG_CFGR1_I2C_PB6_FMP_Msk             /*!< I2C PB6 Fast mode plus */
#define SYSCFG_CFGR1_I2C_PB7_FMP_Pos          (17U)
#define SYSCFG_CFGR1_I2C_PB7_FMP_Msk          (0x1UL << SYSCFG_CFGR1_I2C_PB7_FMP_Pos)  /*!< 0x00020000 */
#define SYSCFG_CFGR1_I2C_PB7_FMP              SYSCFG_CFGR1_I2C_PB7_FMP_Msk             /*!< I2C PB7 Fast mode plus */
#define SYSCFG_CFGR1_I2C_PB8_FMP_Pos          (18U)
#define SYSCFG_CFGR1_I2C_PB8_FMP_Msk          (0x1UL << SYSCFG_CFGR1_I2C_PB8_FMP_Pos)  /*!< 0x00040000 */
#define SYSCFG_CFGR1_I2C_PB8_FMP              SYSCFG_CFGR1_I2C_PB8_FMP_Msk             /*!< I2C PB8 Fast mode plus */
#define SYSCFG_CFGR1_I2C_PB9_FMP_Pos          (19U)
#define SYSCFG_CFGR1_I2C_PB9_FMP_Msk          (0x1UL << SYSCFG_CFGR1_I2C_PB9_FMP_Pos)  /*!< 0x00080000 */
#define SYSCFG_CFGR1_I2C_PB9_FMP              SYSCFG_CFGR1_I2C_PB9_FMP_Msk             /*!< I2C PB9 Fast mode plus */
#define SYSCFG_CFGR1_I2C1_FMP_Pos             (20U)
#define SYSCFG_CFGR1_I2C1_FMP_Msk             (0x1UL << SYSCFG_CFGR1_I2C1_FMP_Pos)     /*!< 0x00100000 */
#define SYSCFG_CFGR1_I2C1_FMP                 SYSCFG_CFGR1_I2C1_FMP_Msk                /*!< Enable Fast Mode Plus on PB10, PB11, PF6 and PF7  */
#define SYSCFG_CFGR1_I2C2_FMP_Pos             (21U)
#define SYSCFG_CFGR1_I2C2_FMP_Msk             (0x1UL << SYSCFG_CFGR1_I2C2_FMP_Pos)     /*!< 0x00200000 */
#define SYSCFG_CFGR1_I2C2_FMP                 SYSCFG_CFGR1_I2C2_FMP_Msk                /*!< Enable I2C2 Fast mode plus  */
#define SYSCFG_CFGR1_I2C_PA9_FMP_Pos          (22U)
#define SYSCFG_CFGR1_I2C_PA9_FMP_Msk          (0x1UL << SYSCFG_CFGR1_I2C_PA9_FMP_Pos)  /*!< 0x00400000 */
#define SYSCFG_CFGR1_I2C_PA9_FMP              SYSCFG_CFGR1_I2C_PA9_FMP_Msk             /*!< Enable Fast Mode Plus on PA9  */
#define SYSCFG_CFGR1_I2C_PA10_FMP_Pos         (23U)
#define SYSCFG_CFGR1_I2C_PA10_FMP_Msk         (0x1UL << SYSCFG_CFGR1_I2C_PA10_FMP_Pos) /*!< 0x00800000 */
#define SYSCFG_CFGR1_I2C_PA10_FMP             SYSCFG_CFGR1_I2C_PA10_FMP_Msk            /*!< Enable Fast Mode Plus on PA10 */

/******************  Bit definition for SYSCFG_CFGR2 register  ****************/
#define SYSCFG_CFGR2_CLL_Pos                  (0U)
#define SYSCFG_CFGR2_CLL_Msk                  (0x1UL << SYSCFG_CFGR2_CLL_Pos)   /*!< 0x00000001 */
#define SYSCFG_CFGR2_CLL                      SYSCFG_CFGR2_CLL_Msk              /*!< Enables and locks the LOCKUP (Hardfault) output of CortexM0 with Break Input of TIMER1 */
#define SYSCFG_CFGR2_SPL_Pos                  (1U)
#define SYSCFG_CFGR2_SPL_Msk                  (0x1UL << SYSCFG_CFGR2_SPL_Pos)   /*!< 0x00000002 */
#define SYSCFG_CFGR2_SPL                      SYSCFG_CFGR2_SPL_Msk              /*!< Enables and locks the SRAM_PARITY error signal with Break Input of TIMER1 */
#define SYSCFG_CFGR2_PVDL_Pos                 (2U)
#define SYSCFG_CFGR2_PVDL_Msk                 (0x1UL << SYSCFG_CFGR2_PVDL_Pos)  /*!< 0x00000004 */
#define SYSCFG_CFGR2_PVDL                     SYSCFG_CFGR2_PVDL_Msk             /*!< Enables and locks the PVD connection with Timer1 Break Input and also the PVD_EN and PVDSEL[2:0] bits of the Power Control Interface */
#define SYSCFG_CFGR2_ECCL_Pos                 (3U)
#define SYSCFG_CFGR2_ECCL_Msk                 (0x1UL << SYSCFG_CFGR2_ECCL_Pos)  /*!< 0x00000008 */
#define SYSCFG_CFGR2_ECCL                     SYSCFG_CFGR2_ECCL_Msk             /*!< ECCL */
#define SYSCFG_CFGR2_SPF_Pos                  (8U)
#define SYSCFG_CFGR2_SPF_Msk                  (0x1UL << SYSCFG_CFGR2_SPF_Pos)   /*!< 0x00000100 */
#define SYSCFG_CFGR2_SPF                      SYSCFG_CFGR2_SPF_Msk              /*!< SRAM Parity error flag */
#define SYSCFG_CFGR2_SRAM_PE                  SYSCFG_CFGR2_SPF                  /*!< SRAM Parity error flag (define maintained for legacy purpose) */

/*****************  Bit definition for SYSCFG_ITLINEx ISR Wrapper register  ****************/
#define SYSCFG_ITLINE0_SR_EWDG_Pos            (0U)
#define SYSCFG_ITLINE0_SR_EWDG_Msk            (0x1UL << SYSCFG_ITLINE0_SR_EWDG_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE0_SR_EWDG                SYSCFG_ITLINE0_SR_EWDG_Msk       /*!< EWDG interrupt */
#define SYSCFG_ITLINE1_SR_PVDOUT_Pos          (0U)
#define SYSCFG_ITLINE1_SR_PVDOUT_Msk          (0x1UL << SYSCFG_ITLINE1_SR_PVDOUT_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE1_SR_PVDOUT              SYSCFG_ITLINE1_SR_PVDOUT_Msk     /*!< Power voltage detection -> exti[16] Interrupt */
#define SYSCFG_ITLINE2_SR_TAMPER_Pos          (0U)
#define SYSCFG_ITLINE2_SR_TAMPER_Msk          (0x1UL << SYSCFG_ITLINE2_SR_TAMPER_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE2_SR_TAMPER              SYSCFG_ITLINE2_SR_TAMPER_Msk     /*!< TAMPER -> exti[21] interrupt */
#define SYSCFG_ITLINE2_SR_RTC_WAKEUP_Pos      (1U)
#define SYSCFG_ITLINE2_SR_RTC_WAKEUP_Msk      (0x1UL << SYSCFG_ITLINE2_SR_RTC_WAKEUP_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE2_SR_RTC_WAKEUP          SYSCFG_ITLINE2_SR_RTC_WAKEUP_Msk /*!< RTC_WAKEUP -> exti[19] interrupt .... */
#define SYSCFG_ITLINE3_SR_FLASH_ECC_Pos       (0U)
#define SYSCFG_ITLINE3_SR_FLASH_ECC_Msk       (0x1UL << SYSCFG_ITLINE3_SR_FLASH_ECC_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE3_SR_FLASH_ECC           SYSCFG_ITLINE3_SR_FLASH_ECC_Msk  /*!< Flash ITF ECC interrupt */
#define SYSCFG_ITLINE3_SR_FLASH_ITF_Pos       (1U)
#define SYSCFG_ITLINE3_SR_FLASH_ITF_Msk       (0x1UL << SYSCFG_ITLINE3_SR_FLASH_ITF_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE3_SR_FLASH_ITF           SYSCFG_ITLINE3_SR_FLASH_ITF_Msk  /*!< FLASH ITF interrupt */
#define SYSCFG_ITLINE4_SR_CLK_CTRL_Pos        (0U)
#define SYSCFG_ITLINE4_SR_CLK_CTRL_Msk        (0x1UL << SYSCFG_ITLINE4_SR_CLK_CTRL_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE4_SR_CLK_CTRL            SYSCFG_ITLINE4_SR_CLK_CTRL_Msk   /*!< RCC interrupt */
#define SYSCFG_ITLINE5_SR_EXTI0_Pos           (0U)
#define SYSCFG_ITLINE5_SR_EXTI0_Msk           (0x1UL << SYSCFG_ITLINE5_SR_EXTI0_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE5_SR_EXTI0               SYSCFG_ITLINE5_SR_EXTI0_Msk      /*!< External Interrupt 0 */
#define SYSCFG_ITLINE5_SR_EXTI1_Pos           (1U)
#define SYSCFG_ITLINE5_SR_EXTI1_Msk           (0x1UL << SYSCFG_ITLINE5_SR_EXTI1_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE5_SR_EXTI1               SYSCFG_ITLINE5_SR_EXTI1_Msk      /*!< External Interrupt 1 */
#define SYSCFG_ITLINE6_SR_EXTI2_Pos           (0U)
#define SYSCFG_ITLINE6_SR_EXTI2_Msk           (0x1UL << SYSCFG_ITLINE6_SR_EXTI2_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE6_SR_EXTI2               SYSCFG_ITLINE6_SR_EXTI2_Msk      /*!< External Interrupt 2 */
#define SYSCFG_ITLINE6_SR_EXTI3_Pos           (1U)
#define SYSCFG_ITLINE6_SR_EXTI3_Msk           (0x1UL << SYSCFG_ITLINE6_SR_EXTI3_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE6_SR_EXTI3               SYSCFG_ITLINE6_SR_EXTI3_Msk      /*!< External Interrupt 3 */
#define SYSCFG_ITLINE7_SR_EXTI4_Pos           (0U)
#define SYSCFG_ITLINE7_SR_EXTI4_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI4_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE7_SR_EXTI4               SYSCFG_ITLINE7_SR_EXTI4_Msk      /*!< External Interrupt 4 */
#define SYSCFG_ITLINE7_SR_EXTI5_Pos           (1U)
#define SYSCFG_ITLINE7_SR_EXTI5_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI5_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE7_SR_EXTI5               SYSCFG_ITLINE7_SR_EXTI5_Msk      /*!< External Interrupt 5 */
#define SYSCFG_ITLINE7_SR_EXTI6_Pos           (2U)
#define SYSCFG_ITLINE7_SR_EXTI6_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI6_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE7_SR_EXTI6               SYSCFG_ITLINE7_SR_EXTI6_Msk      /*!< External Interrupt 6 */
#define SYSCFG_ITLINE7_SR_EXTI7_Pos           (3U)
#define SYSCFG_ITLINE7_SR_EXTI7_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI7_Pos) /*!< 0x00000008 */
#define SYSCFG_ITLINE7_SR_EXTI7               SYSCFG_ITLINE7_SR_EXTI7_Msk      /*!< External Interrupt 7 */
#define SYSCFG_ITLINE7_SR_EXTI8_Pos           (4U)
#define SYSCFG_ITLINE7_SR_EXTI8_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI8_Pos) /*!< 0x00000010 */
#define SYSCFG_ITLINE7_SR_EXTI8               SYSCFG_ITLINE7_SR_EXTI8_Msk      /*!< External Interrupt 8 */
#define SYSCFG_ITLINE7_SR_EXTI9_Pos           (5U)
#define SYSCFG_ITLINE7_SR_EXTI9_Msk           (0x1UL << SYSCFG_ITLINE7_SR_EXTI9_Pos) /*!< 0x00000020 */
#define SYSCFG_ITLINE7_SR_EXTI9               SYSCFG_ITLINE7_SR_EXTI9_Msk      /*!< External Interrupt 9 */
#define SYSCFG_ITLINE7_SR_EXTI10_Pos          (6U)
#define SYSCFG_ITLINE7_SR_EXTI10_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI10_Pos) /*!< 0x00000040 */
#define SYSCFG_ITLINE7_SR_EXTI10              SYSCFG_ITLINE7_SR_EXTI10_Msk     /*!< External Interrupt 10 */
#define SYSCFG_ITLINE7_SR_EXTI11_Pos          (7U)
#define SYSCFG_ITLINE7_SR_EXTI11_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI11_Pos) /*!< 0x00000080 */
#define SYSCFG_ITLINE7_SR_EXTI11              SYSCFG_ITLINE7_SR_EXTI11_Msk     /*!< External Interrupt 11 */
#define SYSCFG_ITLINE7_SR_EXTI12_Pos          (8U)
#define SYSCFG_ITLINE7_SR_EXTI12_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI12_Pos) /*!< 0x00000100 */
#define SYSCFG_ITLINE7_SR_EXTI12              SYSCFG_ITLINE7_SR_EXTI12_Msk     /*!< External Interrupt 12 */
#define SYSCFG_ITLINE7_SR_EXTI13_Pos          (9U)
#define SYSCFG_ITLINE7_SR_EXTI13_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI13_Pos) /*!< 0x00000200 */
#define SYSCFG_ITLINE7_SR_EXTI13              SYSCFG_ITLINE7_SR_EXTI13_Msk     /*!< External Interrupt 13 */
#define SYSCFG_ITLINE7_SR_EXTI14_Pos          (10U)
#define SYSCFG_ITLINE7_SR_EXTI14_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI14_Pos) /*!< 0x00000400 */
#define SYSCFG_ITLINE7_SR_EXTI14              SYSCFG_ITLINE7_SR_EXTI14_Msk     /*!< External Interrupt 14 */
#define SYSCFG_ITLINE7_SR_EXTI15_Pos          (11U)
#define SYSCFG_ITLINE7_SR_EXTI15_Msk          (0x1UL << SYSCFG_ITLINE7_SR_EXTI15_Pos) /*!< 0x00000800 */
#define SYSCFG_ITLINE7_SR_EXTI15              SYSCFG_ITLINE7_SR_EXTI15_Msk     /*!< External Interrupt 15 */
#define SYSCFG_ITLINE8_SR_UCPD1_Pos          (0U)
#define SYSCFG_ITLINE8_SR_UCPD1_Msk          (0x1UL << SYSCFG_ITLINE8_SR_UCPD1_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE8_SR_UCPD1              SYSCFG_ITLINE8_SR_UCPD1_Msk       /*!< UCPD1 -> exti[32] Interrupt */
#define SYSCFG_ITLINE8_SR_UCPD2_Pos          (1U)
#define SYSCFG_ITLINE8_SR_UCPD2_Msk          (0x1UL << SYSCFG_ITLINE8_SR_UCPD2_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE8_SR_UCPD2              SYSCFG_ITLINE8_SR_UCPD2_Msk       /*!< UCPD2 -> exti[33] Interrupt */
#define SYSCFG_ITLINE9_SR_DMA1_CH1_Pos        (0U)
#define SYSCFG_ITLINE9_SR_DMA1_CH1_Msk        (0x1UL << SYSCFG_ITLINE9_SR_DMA1_CH1_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE9_SR_DMA1_CH1            SYSCFG_ITLINE9_SR_DMA1_CH1_Msk   /*!< DMA1 Channel 1 Interrupt */
#define SYSCFG_ITLINE10_SR_DMA1_CH2_Pos       (0U)
#define SYSCFG_ITLINE10_SR_DMA1_CH2_Msk       (0x1UL << SYSCFG_ITLINE10_SR_DMA1_CH2_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE10_SR_DMA1_CH2           SYSCFG_ITLINE10_SR_DMA1_CH2_Msk  /*!< DMA1 Channel 2 Interrupt */
#define SYSCFG_ITLINE10_SR_DMA1_CH3_Pos       (1U)
#define SYSCFG_ITLINE10_SR_DMA1_CH3_Msk       (0x1UL << SYSCFG_ITLINE10_SR_DMA1_CH3_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE10_SR_DMA1_CH3           SYSCFG_ITLINE10_SR_DMA1_CH3_Msk  /*!< DMA2 Channel 3 Interrupt */
#define SYSCFG_ITLINE11_SR_DMAMUX1_Pos         (0U)
#define SYSCFG_ITLINE11_SR_DMAMUX1_Msk         (0x1UL << SYSCFG_ITLINE11_SR_DMAMUX1_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE11_SR_DMAMUX1             SYSCFG_ITLINE11_SR_DMAMUX1_Msk    /*!< DMAMUX Interrupt */
#define SYSCFG_ITLINE11_SR_DMA1_CH4_Pos       (1U)
#define SYSCFG_ITLINE11_SR_DMA1_CH4_Msk       (0x1UL << SYSCFG_ITLINE11_SR_DMA1_CH4_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE11_SR_DMA1_CH4           SYSCFG_ITLINE11_SR_DMA1_CH4_Msk  /*!< DMA1 Channel 4 Interrupt */
#define SYSCFG_ITLINE11_SR_DMA1_CH5_Pos       (2U)
#define SYSCFG_ITLINE11_SR_DMA1_CH5_Msk       (0x1UL << SYSCFG_ITLINE11_SR_DMA1_CH5_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE11_SR_DMA1_CH5           SYSCFG_ITLINE11_SR_DMA1_CH5_Msk  /*!< DMA1 Channel 5 Interrupt */
#define SYSCFG_ITLINE11_SR_DMA1_CH6_Pos       (3U)
#define SYSCFG_ITLINE11_SR_DMA1_CH6_Msk       (0x1UL << SYSCFG_ITLINE11_SR_DMA1_CH6_Pos) /*!< 0x00000008 */
#define SYSCFG_ITLINE11_SR_DMA1_CH6           SYSCFG_ITLINE11_SR_DMA1_CH6_Msk  /*!< DMA1 Channel 6 Interrupt */
#define SYSCFG_ITLINE11_SR_DMA1_CH7_Pos       (4U)
#define SYSCFG_ITLINE11_SR_DMA1_CH7_Msk       (0x1UL << SYSCFG_ITLINE11_SR_DMA1_CH7_Pos) /*!< 0x00000010 */
#define SYSCFG_ITLINE11_SR_DMA1_CH7           SYSCFG_ITLINE11_SR_DMA1_CH7_Msk  /*!< DMA1 Channel 7 Interrupt */
#define SYSCFG_ITLINE12_SR_ADC_Pos            (0U)
#define SYSCFG_ITLINE12_SR_ADC_Msk            (0x1UL << SYSCFG_ITLINE12_SR_ADC_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE12_SR_ADC                SYSCFG_ITLINE12_SR_ADC_Msk       /*!< ADC Interrupt */
#define SYSCFG_ITLINE12_SR_COMP1_Pos          (1U)
#define SYSCFG_ITLINE12_SR_COMP1_Msk          (0x1UL << SYSCFG_ITLINE12_SR_COMP1_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE12_SR_COMP1              SYSCFG_ITLINE12_SR_COMP1_Msk     /*!< COMP1 Interrupt -> exti[17] */
#define SYSCFG_ITLINE12_SR_COMP2_Pos          (2U)
#define SYSCFG_ITLINE12_SR_COMP2_Msk          (0x1UL << SYSCFG_ITLINE12_SR_COMP2_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE12_SR_COMP2              SYSCFG_ITLINE12_SR_COMP2_Msk     /*!< COMP2 Interrupt -> exti[18] */
#define SYSCFG_ITLINE13_SR_TIM1_CCU_Pos       (0U)
#define SYSCFG_ITLINE13_SR_TIM1_CCU_Msk       (0x1UL << SYSCFG_ITLINE13_SR_TIM1_CCU_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE13_SR_TIM1_CCU           SYSCFG_ITLINE13_SR_TIM1_CCU_Msk  /*!< TIM1 CCU Interrupt */
#define SYSCFG_ITLINE13_SR_TIM1_TRG_Pos       (1U)
#define SYSCFG_ITLINE13_SR_TIM1_TRG_Msk       (0x1UL << SYSCFG_ITLINE13_SR_TIM1_TRG_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE13_SR_TIM1_TRG           SYSCFG_ITLINE13_SR_TIM1_TRG_Msk  /*!< TIM1 TRG Interrupt */
#define SYSCFG_ITLINE13_SR_TIM1_UPD_Pos       (2U)
#define SYSCFG_ITLINE13_SR_TIM1_UPD_Msk       (0x1UL << SYSCFG_ITLINE13_SR_TIM1_UPD_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE13_SR_TIM1_UPD           SYSCFG_ITLINE13_SR_TIM1_UPD_Msk  /*!< TIM1 UPD Interrupt */
#define SYSCFG_ITLINE13_SR_TIM1_BRK_Pos       (3U)
#define SYSCFG_ITLINE13_SR_TIM1_BRK_Msk       (0x1UL << SYSCFG_ITLINE13_SR_TIM1_BRK_Pos) /*!< 0x00000008 */
#define SYSCFG_ITLINE13_SR_TIM1_BRK           SYSCFG_ITLINE13_SR_TIM1_BRK_Msk  /*!< TIM1 BRK Interrupt */
#define SYSCFG_ITLINE14_SR_TIM1_CC_Pos        (0U)
#define SYSCFG_ITLINE14_SR_TIM1_CC_Msk        (0x1UL << SYSCFG_ITLINE14_SR_TIM1_CC_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE14_SR_TIM1_CC            SYSCFG_ITLINE14_SR_TIM1_CC_Msk   /*!< TIM1 CC Interrupt */
#define SYSCFG_ITLINE15_SR_TIM2_GLB_Pos       (0U)
#define SYSCFG_ITLINE15_SR_TIM2_GLB_Msk       (0x1UL << SYSCFG_ITLINE15_SR_TIM2_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE15_SR_TIM2_GLB           SYSCFG_ITLINE15_SR_TIM2_GLB_Msk  /*!< TIM2 GLB Interrupt */
#define SYSCFG_ITLINE16_SR_TIM3_GLB_Pos       (0U)
#define SYSCFG_ITLINE16_SR_TIM3_GLB_Msk       (0x1UL << SYSCFG_ITLINE16_SR_TIM3_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE16_SR_TIM3_GLB           SYSCFG_ITLINE16_SR_TIM3_GLB_Msk  /*!< TIM3 GLB Interrupt */
#define SYSCFG_ITLINE17_SR_TIM6_GLB_Pos       (0U)
#define SYSCFG_ITLINE17_SR_TIM6_GLB_Msk       (0x1UL << SYSCFG_ITLINE17_SR_TIM6_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE17_SR_TIM6_GLB           SYSCFG_ITLINE17_SR_TIM6_GLB_Msk  /*!< TIM6 GLB Interrupt */
#define SYSCFG_ITLINE17_SR_DAC_Pos            (1U)
#define SYSCFG_ITLINE17_SR_DAC_Msk            (0x1UL << SYSCFG_ITLINE17_SR_DAC_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE17_SR_DAC                SYSCFG_ITLINE17_SR_DAC_Msk       /*!< DAC Interrupt */
#define SYSCFG_ITLINE17_SR_LPTIM1_GLB_Pos     (2U)
#define SYSCFG_ITLINE17_SR_LPTIM1_GLB_Msk     (0x1UL << SYSCFG_ITLINE17_SR_LPTIM1_GLB_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE17_SR_LPTIM1_GLB         SYSCFG_ITLINE17_SR_LPTIM1_GLB_Msk /*!< LPTIM1 -> exti[29] Interrupt */
#define SYSCFG_ITLINE18_SR_TIM7_GLB_Pos       (0U)
#define SYSCFG_ITLINE18_SR_TIM7_GLB_Msk       (0x1UL << SYSCFG_ITLINE18_SR_TIM7_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE18_SR_TIM7_GLB           SYSCFG_ITLINE18_SR_TIM7_GLB_Msk  /*!< TIM7 GLB Interrupt */
#define SYSCFG_ITLINE18_SR_LPTIM2_GLB_Pos     (1U)
#define SYSCFG_ITLINE18_SR_LPTIM2_GLB_Msk     (0x1UL << SYSCFG_ITLINE18_SR_LPTIM2_GLB_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE18_SR_LPTIM2_GLB         SYSCFG_ITLINE18_SR_LPTIM2_GLB_Msk /*!< LPTIM2 -> exti[30] Interrupt */
#define SYSCFG_ITLINE19_SR_TIM14_GLB_Pos      (0U)
#define SYSCFG_ITLINE19_SR_TIM14_GLB_Msk      (0x1UL << SYSCFG_ITLINE19_SR_TIM14_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE19_SR_TIM14_GLB          SYSCFG_ITLINE19_SR_TIM14_GLB_Msk /*!< TIM14 GLB Interrupt */
#define SYSCFG_ITLINE20_SR_TIM15_GLB_Pos      (0U)
#define SYSCFG_ITLINE20_SR_TIM15_GLB_Msk      (0x1UL << SYSCFG_ITLINE20_SR_TIM15_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE20_SR_TIM15_GLB          SYSCFG_ITLINE20_SR_TIM15_GLB_Msk /*!< TIM15 GLB Interrupt */
#define SYSCFG_ITLINE21_SR_TIM16_GLB_Pos      (0U)
#define SYSCFG_ITLINE21_SR_TIM16_GLB_Msk      (0x1UL << SYSCFG_ITLINE21_SR_TIM16_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE21_SR_TIM16_GLB          SYSCFG_ITLINE21_SR_TIM16_GLB_Msk /*!< TIM16 GLB Interrupt */
#define SYSCFG_ITLINE22_SR_TIM17_GLB_Pos      (0U)
#define SYSCFG_ITLINE22_SR_TIM17_GLB_Msk      (0x1UL << SYSCFG_ITLINE22_SR_TIM17_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE22_SR_TIM17_GLB          SYSCFG_ITLINE22_SR_TIM17_GLB_Msk /*!< TIM17 GLB Interrupt */
#define SYSCFG_ITLINE23_SR_I2C1_GLB_Pos       (0U)
#define SYSCFG_ITLINE23_SR_I2C1_GLB_Msk       (0x1UL << SYSCFG_ITLINE23_SR_I2C1_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE23_SR_I2C1_GLB           SYSCFG_ITLINE23_SR_I2C1_GLB_Msk  /*!< I2C1 GLB Interrupt -> exti[23] */
#define SYSCFG_ITLINE24_SR_I2C2_GLB_Pos       (0U)
#define SYSCFG_ITLINE24_SR_I2C2_GLB_Msk       (0x1UL << SYSCFG_ITLINE24_SR_I2C2_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE24_SR_I2C2_GLB           SYSCFG_ITLINE24_SR_I2C2_GLB_Msk  /*!< I2C2 GLB Interrupt */
#define SYSCFG_ITLINE25_SR_SPI1_Pos           (0U)
#define SYSCFG_ITLINE25_SR_SPI1_Msk           (0x1UL << SYSCFG_ITLINE25_SR_SPI1_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE25_SR_SPI1               SYSCFG_ITLINE25_SR_SPI1_Msk      /*!< SPI1 Interrupt */
#define SYSCFG_ITLINE26_SR_SPI2_Pos           (0U)
#define SYSCFG_ITLINE26_SR_SPI2_Msk           (0x1UL << SYSCFG_ITLINE26_SR_SPI2_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE26_SR_SPI2               SYSCFG_ITLINE26_SR_SPI2_Msk      /*!< SPI2  Interrupt */
#define SYSCFG_ITLINE27_SR_USART1_GLB_Pos     (0U)
#define SYSCFG_ITLINE27_SR_USART1_GLB_Msk     (0x1UL << SYSCFG_ITLINE27_SR_USART1_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE27_SR_USART1_GLB         SYSCFG_ITLINE27_SR_USART1_GLB_Msk /*!< USART1 GLB Interrupt -> exti[25] */
#define SYSCFG_ITLINE28_SR_USART2_GLB_Pos     (0U)
#define SYSCFG_ITLINE28_SR_USART2_GLB_Msk     (0x1UL << SYSCFG_ITLINE28_SR_USART2_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE28_SR_USART2_GLB         SYSCFG_ITLINE28_SR_USART2_GLB_Msk /*!< USART2 GLB Interrupt -> exti[26] */
#define SYSCFG_ITLINE29_SR_USART3_GLB_Pos     (0U)
#define SYSCFG_ITLINE29_SR_USART3_GLB_Msk     (0x1UL << SYSCFG_ITLINE29_SR_USART3_GLB_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE29_SR_USART3_GLB         SYSCFG_ITLINE29_SR_USART3_GLB_Msk /*!< USART3 GLB Interrupt */
#define SYSCFG_ITLINE29_SR_USART4_GLB_Pos     (1U)
#define SYSCFG_ITLINE29_SR_USART4_GLB_Msk     (0x1UL << SYSCFG_ITLINE29_SR_USART4_GLB_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE29_SR_USART4_GLB         SYSCFG_ITLINE29_SR_USART4_GLB_Msk /*!< USART4 GLB Interrupt */
#define SYSCFG_ITLINE29_SR_LPUART1_GLB_Pos    (2U)
#define SYSCFG_ITLINE29_SR_LPUART1_GLB_Msk    (0x1UL << SYSCFG_ITLINE29_SR_LPUART1_GLB_Pos) /*!< 0x00000004 */
#define SYSCFG_ITLINE29_SR_LPUART1_GLB        SYSCFG_ITLINE29_SR_LPUART1_GLB_Msk /*!< LPUART1 GLB Interrupt -> exti[28] */
#define SYSCFG_ITLINE30_SR_CEC_Pos            (0U)
#define SYSCFG_ITLINE30_SR_CEC_Msk            (0x1UL << SYSCFG_ITLINE30_SR_CEC_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE30_SR_CEC                SYSCFG_ITLINE30_SR_CEC_Msk       /*!< CEC Interrupt-> exti[27] */
#define SYSCFG_ITLINE31_SR_RNG_Pos            (0U)
#define SYSCFG_ITLINE31_SR_RNG_Msk            (0x1UL << SYSCFG_ITLINE31_SR_RNG_Pos) /*!< 0x00000001 */
#define SYSCFG_ITLINE31_SR_RNG                SYSCFG_ITLINE31_SR_RNG_Msk       /*!< RNG Interrupt */
#define SYSCFG_ITLINE31_SR_AES_Pos            (1U)
#define SYSCFG_ITLINE31_SR_AES_Msk            (0x1UL << SYSCFG_ITLINE31_SR_AES_Pos) /*!< 0x00000002 */
#define SYSCFG_ITLINE31_SR_AES                SYSCFG_ITLINE31_SR_AES_Msk       /*!< AES Interrupt */

/******************************************************************************/
/*                                                                            */
/*                                    TIM                                     */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for TIM_CR1 register  ********************/
#define TIM_CR1_CEN_Pos           (0U)
#define TIM_CR1_CEN_Msk           (0x1UL << TIM_CR1_CEN_Pos)                   /*!< 0x00000001 */
#define TIM_CR1_CEN               TIM_CR1_CEN_Msk                              /*!<Counter enable */
#define TIM_CR1_UDIS_Pos          (1U)
#define TIM_CR1_UDIS_Msk          (0x1UL << TIM_CR1_UDIS_Pos)                  /*!< 0x00000002 */
#define TIM_CR1_UDIS              TIM_CR1_UDIS_Msk                             /*!<Update disable */
#define TIM_CR1_URS_Pos           (2U)
#define TIM_CR1_URS_Msk           (0x1UL << TIM_CR1_URS_Pos)                   /*!< 0x00000004 */
#define TIM_CR1_URS               TIM_CR1_URS_Msk                              /*!<Update request source */
#define TIM_CR1_OPM_Pos           (3U)
#define TIM_CR1_OPM_Msk           (0x1UL << TIM_CR1_OPM_Pos)                   /*!< 0x00000008 */
#define TIM_CR1_OPM               TIM_CR1_OPM_Msk                              /*!<One pulse mode */
#define TIM_CR1_DIR_Pos           (4U)
#define TIM_CR1_DIR_Msk           (0x1UL << TIM_CR1_DIR_Pos)                   /*!< 0x00000010 */
#define TIM_CR1_DIR               TIM_CR1_DIR_Msk                              /*!<Direction */

#define TIM_CR1_CMS_Pos           (5U)
#define TIM_CR1_CMS_Msk           (0x3UL << TIM_CR1_CMS_Pos)                   /*!< 0x00000060 */
#define TIM_CR1_CMS               TIM_CR1_CMS_Msk                              /*!<CMS[1:0] bits (Center-aligned mode selection) */
#define TIM_CR1_CMS_0             (0x1UL << TIM_CR1_CMS_Pos)                   /*!< 0x00000020 */
#define TIM_CR1_CMS_1             (0x2UL << TIM_CR1_CMS_Pos)                   /*!< 0x00000040 */

#define TIM_CR1_ARPE_Pos          (7U)
#define TIM_CR1_ARPE_Msk          (0x1UL << TIM_CR1_ARPE_Pos)                  /*!< 0x00000080 */
#define TIM_CR1_ARPE              TIM_CR1_ARPE_Msk                             /*!<Auto-reload preload enable */

#define TIM_CR1_CKD_Pos           (8U)
#define TIM_CR1_CKD_Msk           (0x3UL << TIM_CR1_CKD_Pos)                   /*!< 0x00000300 */
#define TIM_CR1_CKD               TIM_CR1_CKD_Msk                              /*!<CKD[1:0] bits (clock division) */
#define TIM_CR1_CKD_0             (0x1UL << TIM_CR1_CKD_Pos)                   /*!< 0x00000100 */
#define TIM_CR1_CKD_1             (0x2UL << TIM_CR1_CKD_Pos)                   /*!< 0x00000200 */

#define TIM_CR1_UIFREMAP_Pos      (11U)
#define TIM_CR1_UIFREMAP_Msk      (0x1UL << TIM_CR1_UIFREMAP_Pos)              /*!< 0x00000800 */
#define TIM_CR1_UIFREMAP          TIM_CR1_UIFREMAP_Msk                         /*!<Update interrupt flag remap */

/*******************  Bit definition for TIM_CR2 register  ********************/
#define TIM_CR2_CCPC_Pos          (0U)
#define TIM_CR2_CCPC_Msk          (0x1UL << TIM_CR2_CCPC_Pos)                  /*!< 0x00000001 */
#define TIM_CR2_CCPC              TIM_CR2_CCPC_Msk                             /*!<Capture/Compare Preloaded Control */
#define TIM_CR2_CCUS_Pos          (2U)
#define TIM_CR2_CCUS_Msk          (0x1UL << TIM_CR2_CCUS_Pos)                  /*!< 0x00000004 */
#define TIM_CR2_CCUS              TIM_CR2_CCUS_Msk                             /*!<Capture/Compare Control Update Selection */
#define TIM_CR2_CCDS_Pos          (3U)
#define TIM_CR2_CCDS_Msk          (0x1UL << TIM_CR2_CCDS_Pos)                  /*!< 0x00000008 */
#define TIM_CR2_CCDS              TIM_CR2_CCDS_Msk                             /*!<Capture/Compare DMA Selection */

#define TIM_CR2_MMS_Pos           (4U)
#define TIM_CR2_MMS_Msk           (0x7UL << TIM_CR2_MMS_Pos)                   /*!< 0x00000070 */
#define TIM_CR2_MMS               TIM_CR2_MMS_Msk                              /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS_0             (0x1UL << TIM_CR2_MMS_Pos)                   /*!< 0x00000010 */
#define TIM_CR2_MMS_1             (0x2UL << TIM_CR2_MMS_Pos)                   /*!< 0x00000020 */
#define TIM_CR2_MMS_2             (0x4UL << TIM_CR2_MMS_Pos)                   /*!< 0x00000040 */

#define TIM_CR2_TI1S_Pos          (7U)
#define TIM_CR2_TI1S_Msk          (0x1UL << TIM_CR2_TI1S_Pos)                  /*!< 0x00000080 */
#define TIM_CR2_TI1S              TIM_CR2_TI1S_Msk                             /*!<TI1 Selection */
#define TIM_CR2_OIS1_Pos          (8U)
#define TIM_CR2_OIS1_Msk          (0x1UL << TIM_CR2_OIS1_Pos)                  /*!< 0x00000100 */
#define TIM_CR2_OIS1              TIM_CR2_OIS1_Msk                             /*!<Output Idle state 1 (OC1 output) */
#define TIM_CR2_OIS1N_Pos         (9U)
#define TIM_CR2_OIS1N_Msk         (0x1UL << TIM_CR2_OIS1N_Pos)                 /*!< 0x00000200 */
#define TIM_CR2_OIS1N             TIM_CR2_OIS1N_Msk                            /*!<Output Idle state 1 (OC1N output) */
#define TIM_CR2_OIS2_Pos          (10U)
#define TIM_CR2_OIS2_Msk          (0x1UL << TIM_CR2_OIS2_Pos)                  /*!< 0x00000400 */
#define TIM_CR2_OIS2              TIM_CR2_OIS2_Msk                             /*!<Output Idle state 2 (OC2 output) */
#define TIM_CR2_OIS2N_Pos         (11U)
#define TIM_CR2_OIS2N_Msk         (0x1UL << TIM_CR2_OIS2N_Pos)                 /*!< 0x00000800 */
#define TIM_CR2_OIS2N             TIM_CR2_OIS2N_Msk                            /*!<Output Idle state 2 (OC2N output) */
#define TIM_CR2_OIS3_Pos          (12U)
#define TIM_CR2_OIS3_Msk          (0x1UL << TIM_CR2_OIS3_Pos)                  /*!< 0x00001000 */
#define TIM_CR2_OIS3              TIM_CR2_OIS3_Msk                             /*!<Output Idle state 3 (OC3 output) */
#define TIM_CR2_OIS3N_Pos         (13U)
#define TIM_CR2_OIS3N_Msk         (0x1UL << TIM_CR2_OIS3N_Pos)                 /*!< 0x00002000 */
#define TIM_CR2_OIS3N             TIM_CR2_OIS3N_Msk                            /*!<Output Idle state 3 (OC3N output) */
#define TIM_CR2_OIS4_Pos          (14U)
#define TIM_CR2_OIS4_Msk          (0x1UL << TIM_CR2_OIS4_Pos)                  /*!< 0x00004000 */
#define TIM_CR2_OIS4              TIM_CR2_OIS4_Msk                             /*!<Output Idle state 4 (OC4 output) */
#define TIM_CR2_OIS5_Pos          (16U)
#define TIM_CR2_OIS5_Msk          (0x1UL << TIM_CR2_OIS5_Pos)                  /*!< 0x00010000 */
#define TIM_CR2_OIS5              TIM_CR2_OIS5_Msk                             /*!<Output Idle state 5 (OC5 output) */
#define TIM_CR2_OIS6_Pos          (18U)
#define TIM_CR2_OIS6_Msk          (0x1UL << TIM_CR2_OIS6_Pos)                  /*!< 0x00040000 */
#define TIM_CR2_OIS6              TIM_CR2_OIS6_Msk                             /*!<Output Idle state 6 (OC6 output) */

#define TIM_CR2_MMS2_Pos          (20U)
#define TIM_CR2_MMS2_Msk          (0xFUL << TIM_CR2_MMS2_Pos)                  /*!< 0x00F00000 */
#define TIM_CR2_MMS2              TIM_CR2_MMS2_Msk                             /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS2_0            (0x1UL << TIM_CR2_MMS2_Pos)                  /*!< 0x00100000 */
#define TIM_CR2_MMS2_1            (0x2UL << TIM_CR2_MMS2_Pos)                  /*!< 0x00200000 */
#define TIM_CR2_MMS2_2            (0x4UL << TIM_CR2_MMS2_Pos)                  /*!< 0x00400000 */
#define TIM_CR2_MMS2_3            (0x8UL << TIM_CR2_MMS2_Pos)                  /*!< 0x00800000 */

/*******************  Bit definition for TIM_SMCR register  *******************/
#define TIM_SMCR_SMS_Pos          (0U)
#define TIM_SMCR_SMS_Msk          (0x10007UL << TIM_SMCR_SMS_Pos)              /*!< 0x00010007 */
#define TIM_SMCR_SMS              TIM_SMCR_SMS_Msk                             /*!<SMS[2:0] bits (Slave mode selection) */
#define TIM_SMCR_SMS_0            (0x00001UL << TIM_SMCR_SMS_Pos)              /*!< 0x00000001 */
#define TIM_SMCR_SMS_1            (0x00002UL << TIM_SMCR_SMS_Pos)              /*!< 0x00000002 */
#define TIM_SMCR_SMS_2            (0x00004UL << TIM_SMCR_SMS_Pos)              /*!< 0x00000004 */
#define TIM_SMCR_SMS_3            (0x10000UL << TIM_SMCR_SMS_Pos)              /*!< 0x00010000 */

#define TIM_SMCR_OCCS_Pos         (3U)
#define TIM_SMCR_OCCS_Msk         (0x1UL << TIM_SMCR_OCCS_Pos)                 /*!< 0x00000008 */
#define TIM_SMCR_OCCS             TIM_SMCR_OCCS_Msk                            /*!< OCREF clear selection */

#define TIM_SMCR_TS_Pos           (4U)
#define TIM_SMCR_TS_Msk           (0x30007UL << TIM_SMCR_TS_Pos)               /*!< 0x00300070 */
#define TIM_SMCR_TS               TIM_SMCR_TS_Msk                              /*!<TS[2:0] bits (Trigger selection) */
#define TIM_SMCR_TS_0             (0x00001UL << TIM_SMCR_TS_Pos)               /*!< 0x00000010 */
#define TIM_SMCR_TS_1             (0x00002UL << TIM_SMCR_TS_Pos)               /*!< 0x00000020 */
#define TIM_SMCR_TS_2             (0x00004UL << TIM_SMCR_TS_Pos)               /*!< 0x00000040 */
#define TIM_SMCR_TS_3             (0x10000UL << TIM_SMCR_TS_Pos)               /*!< 0x00100000 */
#define TIM_SMCR_TS_4             (0x20000UL << TIM_SMCR_TS_Pos)               /*!< 0x00200000 */

#define TIM_SMCR_MSM_Pos          (7U)
#define TIM_SMCR_MSM_Msk          (0x1UL << TIM_SMCR_MSM_Pos)                  /*!< 0x00000080 */
#define TIM_SMCR_MSM              TIM_SMCR_MSM_Msk                             /*!<Master/slave mode */

#define TIM_SMCR_ETF_Pos          (8U)
#define TIM_SMCR_ETF_Msk          (0xFUL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000F00 */
#define TIM_SMCR_ETF              TIM_SMCR_ETF_Msk                             /*!<ETF[3:0] bits (External trigger filter) */
#define TIM_SMCR_ETF_0            (0x1UL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000100 */
#define TIM_SMCR_ETF_1            (0x2UL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000200 */
#define TIM_SMCR_ETF_2            (0x4UL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000400 */
#define TIM_SMCR_ETF_3            (0x8UL << TIM_SMCR_ETF_Pos)                  /*!< 0x00000800 */

#define TIM_SMCR_ETPS_Pos         (12U)
#define TIM_SMCR_ETPS_Msk         (0x3UL << TIM_SMCR_ETPS_Pos)                 /*!< 0x00003000 */
#define TIM_SMCR_ETPS             TIM_SMCR_ETPS_Msk                            /*!<ETPS[1:0] bits (External trigger prescaler) */
#define TIM_SMCR_ETPS_0           (0x1UL << TIM_SMCR_ETPS_Pos)                 /*!< 0x00001000 */
#define TIM_SMCR_ETPS_1           (0x2UL << TIM_SMCR_ETPS_Pos)                 /*!< 0x00002000 */

#define TIM_SMCR_ECE_Pos          (14U)
#define TIM_SMCR_ECE_Msk          (0x1UL << TIM_SMCR_ECE_Pos)                  /*!< 0x00004000 */
#define TIM_SMCR_ECE              TIM_SMCR_ECE_Msk                             /*!<External clock enable */
#define TIM_SMCR_ETP_Pos          (15U)
#define TIM_SMCR_ETP_Msk          (0x1UL << TIM_SMCR_ETP_Pos)                  /*!< 0x00008000 */
#define TIM_SMCR_ETP              TIM_SMCR_ETP_Msk                             /*!<External trigger polarity */

/*******************  Bit definition for TIM_DIER register  *******************/
#define TIM_DIER_UIE_Pos          (0U)
#define TIM_DIER_UIE_Msk          (0x1UL << TIM_DIER_UIE_Pos)                  /*!< 0x00000001 */
#define TIM_DIER_UIE              TIM_DIER_UIE_Msk                             /*!<Update interrupt enable */
#define TIM_DIER_CC1IE_Pos        (1U)
#define TIM_DIER_CC1IE_Msk        (0x1UL << TIM_DIER_CC1IE_Pos)                /*!< 0x00000002 */
#define TIM_DIER_CC1IE            TIM_DIER_CC1IE_Msk                           /*!<Capture/Compare 1 interrupt enable */
#define TIM_DIER_CC2IE_Pos        (2U)
#define TIM_DIER_CC2IE_Msk        (0x1UL << TIM_DIER_CC2IE_Pos)                /*!< 0x00000004 */
#define TIM_DIER_CC2IE            TIM_DIER_CC2IE_Msk                           /*!<Capture/Compare 2 interrupt enable */
#define TIM_DIER_CC3IE_Pos        (3U)
#define TIM_DIER_CC3IE_Msk        (0x1UL << TIM_DIER_CC3IE_Pos)                /*!< 0x00000008 */
#define TIM_DIER_CC3IE            TIM_DIER_CC3IE_Msk                           /*!<Capture/Compare 3 interrupt enable */
#define TIM_DIER_CC4IE_Pos        (4U)
#define TIM_DIER_CC4IE_Msk        (0x1UL << TIM_DIER_CC4IE_Pos)                /*!< 0x00000010 */
#define TIM_DIER_CC4IE            TIM_DIER_CC4IE_Msk                           /*!<Capture/Compare 4 interrupt enable */
#define TIM_DIER_COMIE_Pos        (5U)
#define TIM_DIER_COMIE_Msk        (0x1UL << TIM_DIER_COMIE_Pos)                /*!< 0x00000020 */
#define TIM_DIER_COMIE            TIM_DIER_COMIE_Msk                           /*!<COM interrupt enable */
#define TIM_DIER_TIE_Pos          (6U)
#define TIM_DIER_TIE_Msk          (0x1UL << TIM_DIER_TIE_Pos)                  /*!< 0x00000040 */
#define TIM_DIER_TIE              TIM_DIER_TIE_Msk                             /*!<Trigger interrupt enable */
#define TIM_DIER_BIE_Pos          (7U)
#define TIM_DIER_BIE_Msk          (0x1UL << TIM_DIER_BIE_Pos)                  /*!< 0x00000080 */
#define TIM_DIER_BIE              TIM_DIER_BIE_Msk                             /*!<Break interrupt enable */
#define TIM_DIER_UDE_Pos          (8U)
#define TIM_DIER_UDE_Msk          (0x1UL << TIM_DIER_UDE_Pos)                  /*!< 0x00000100 */
#define TIM_DIER_UDE              TIM_DIER_UDE_Msk                             /*!<Update DMA request enable */
#define TIM_DIER_CC1DE_Pos        (9U)
#define TIM_DIER_CC1DE_Msk        (0x1UL << TIM_DIER_CC1DE_Pos)                /*!< 0x00000200 */
#define TIM_DIER_CC1DE            TIM_DIER_CC1DE_Msk                           /*!<Capture/Compare 1 DMA request enable */
#define TIM_DIER_CC2DE_Pos        (10U)
#define TIM_DIER_CC2DE_Msk        (0x1UL << TIM_DIER_CC2DE_Pos)                /*!< 0x00000400 */
#define TIM_DIER_CC2DE            TIM_DIER_CC2DE_Msk                           /*!<Capture/Compare 2 DMA request enable */
#define TIM_DIER_CC3DE_Pos        (11U)
#define TIM_DIER_CC3DE_Msk        (0x1UL << TIM_DIER_CC3DE_Pos)                /*!< 0x00000800 */
#define TIM_DIER_CC3DE            TIM_DIER_CC3DE_Msk                           /*!<Capture/Compare 3 DMA request enable */
#define TIM_DIER_CC4DE_Pos        (12U)
#define TIM_DIER_CC4DE_Msk        (0x1UL << TIM_DIER_CC4DE_Pos)                /*!< 0x00001000 */
#define TIM_DIER_CC4DE            TIM_DIER_CC4DE_Msk                           /*!<Capture/Compare 4 DMA request enable */
#define TIM_DIER_COMDE_Pos        (13U)
#define TIM_DIER_COMDE_Msk        (0x1UL << TIM_DIER_COMDE_Pos)                /*!< 0x00002000 */
#define TIM_DIER_COMDE            TIM_DIER_COMDE_Msk                           /*!<COM DMA request enable */
#define TIM_DIER_TDE_Pos          (14U)
#define TIM_DIER_TDE_Msk          (0x1UL << TIM_DIER_TDE_Pos)                  /*!< 0x00004000 */
#define TIM_DIER_TDE              TIM_DIER_TDE_Msk                             /*!<Trigger DMA request enable */

/********************  Bit definition for TIM_SR register  ********************/
#define TIM_SR_UIF_Pos            (0U)
#define TIM_SR_UIF_Msk            (0x1UL << TIM_SR_UIF_Pos)                    /*!< 0x00000001 */
#define TIM_SR_UIF                TIM_SR_UIF_Msk                               /*!<Update interrupt Flag */
#define TIM_SR_CC1IF_Pos          (1U)
#define TIM_SR_CC1IF_Msk          (0x1UL << TIM_SR_CC1IF_Pos)                  /*!< 0x00000002 */
#define TIM_SR_CC1IF              TIM_SR_CC1IF_Msk                             /*!<Capture/Compare 1 interrupt Flag */
#define TIM_SR_CC2IF_Pos          (2U)
#define TIM_SR_CC2IF_Msk          (0x1UL << TIM_SR_CC2IF_Pos)                  /*!< 0x00000004 */
#define TIM_SR_CC2IF              TIM_SR_CC2IF_Msk                             /*!<Capture/Compare 2 interrupt Flag */
#define TIM_SR_CC3IF_Pos          (3U)
#define TIM_SR_CC3IF_Msk          (0x1UL << TIM_SR_CC3IF_Pos)                  /*!< 0x00000008 */
#define TIM_SR_CC3IF              TIM_SR_CC3IF_Msk                             /*!<Capture/Compare 3 interrupt Flag */
#define TIM_SR_CC4IF_Pos          (4U)
#define TIM_SR_CC4IF_Msk          (0x1UL << TIM_SR_CC4IF_Pos)                  /*!< 0x00000010 */
#define TIM_SR_CC4IF              TIM_SR_CC4IF_Msk                             /*!<Capture/Compare 4 interrupt Flag */
#define TIM_SR_COMIF_Pos          (5U)
#define TIM_SR_COMIF_Msk          (0x1UL << TIM_SR_COMIF_Pos)                  /*!< 0x00000020 */
#define TIM_SR_COMIF              TIM_SR_COMIF_Msk                             /*!<COM interrupt Flag */
#define TIM_SR_TIF_Pos            (6U)
#define TIM_SR_TIF_Msk            (0x1UL << TIM_SR_TIF_Pos)                    /*!< 0x00000040 */
#define TIM_SR_TIF                TIM_SR_TIF_Msk                               /*!<Trigger interrupt Flag */
#define TIM_SR_BIF_Pos            (7U)
#define TIM_SR_BIF_Msk            (0x1UL << TIM_SR_BIF_Pos)                    /*!< 0x00000080 */
#define TIM_SR_BIF                TIM_SR_BIF_Msk                               /*!<Break interrupt Flag */
#define TIM_SR_B2IF_Pos           (8U)
#define TIM_SR_B2IF_Msk           (0x1UL << TIM_SR_B2IF_Pos)                   /*!< 0x00000100 */
#define TIM_SR_B2IF               TIM_SR_B2IF_Msk                              /*!<Break 2 interrupt Flag */
#define TIM_SR_CC1OF_Pos          (9U)
#define TIM_SR_CC1OF_Msk          (0x1UL << TIM_SR_CC1OF_Pos)                  /*!< 0x00000200 */
#define TIM_SR_CC1OF              TIM_SR_CC1OF_Msk                             /*!<Capture/Compare 1 Overcapture Flag */
#define TIM_SR_CC2OF_Pos          (10U)
#define TIM_SR_CC2OF_Msk          (0x1UL << TIM_SR_CC2OF_Pos)                  /*!< 0x00000400 */
#define TIM_SR_CC2OF              TIM_SR_CC2OF_Msk                             /*!<Capture/Compare 2 Overcapture Flag */
#define TIM_SR_CC3OF_Pos          (11U)
#define TIM_SR_CC3OF_Msk          (0x1UL << TIM_SR_CC3OF_Pos)                  /*!< 0x00000800 */
#define TIM_SR_CC3OF              TIM_SR_CC3OF_Msk                             /*!<Capture/Compare 3 Overcapture Flag */
#define TIM_SR_CC4OF_Pos          (12U)
#define TIM_SR_CC4OF_Msk          (0x1UL << TIM_SR_CC4OF_Pos)                  /*!< 0x00001000 */
#define TIM_SR_CC4OF              TIM_SR_CC4OF_Msk                             /*!<Capture/Compare 4 Overcapture Flag */
#define TIM_SR_SBIF_Pos           (13U)
#define TIM_SR_SBIF_Msk           (0x1UL << TIM_SR_SBIF_Pos)                   /*!< 0x00002000 */
#define TIM_SR_SBIF               TIM_SR_SBIF_Msk                              /*!<System Break interrupt Flag */
#define TIM_SR_CC5IF_Pos          (16U)
#define TIM_SR_CC5IF_Msk          (0x1UL << TIM_SR_CC5IF_Pos)                  /*!< 0x00010000 */
#define TIM_SR_CC5IF              TIM_SR_CC5IF_Msk                             /*!<Capture/Compare 5 interrupt Flag */
#define TIM_SR_CC6IF_Pos          (17U)
#define TIM_SR_CC6IF_Msk          (0x1UL << TIM_SR_CC6IF_Pos)                  /*!< 0x00020000 */
#define TIM_SR_CC6IF              TIM_SR_CC6IF_Msk                             /*!<Capture/Compare 6 interrupt Flag */


/*******************  Bit definition for TIM_EGR register  ********************/
#define TIM_EGR_UG_Pos            (0U)
#define TIM_EGR_UG_Msk            (0x1UL << TIM_EGR_UG_Pos)                    /*!< 0x00000001 */
#define TIM_EGR_UG                TIM_EGR_UG_Msk                               /*!<Update Generation */
#define TIM_EGR_CC1G_Pos          (1U)
#define TIM_EGR_CC1G_Msk          (0x1UL << TIM_EGR_CC1G_Pos)                  /*!< 0x00000002 */
#define TIM_EGR_CC1G              TIM_EGR_CC1G_Msk                             /*!<Capture/Compare 1 Generation */
#define TIM_EGR_CC2G_Pos          (2U)
#define TIM_EGR_CC2G_Msk          (0x1UL << TIM_EGR_CC2G_Pos)                  /*!< 0x00000004 */
#define TIM_EGR_CC2G              TIM_EGR_CC2G_Msk                             /*!<Capture/Compare 2 Generation */
#define TIM_EGR_CC3G_Pos          (3U)
#define TIM_EGR_CC3G_Msk          (0x1UL << TIM_EGR_CC3G_Pos)                  /*!< 0x00000008 */
#define TIM_EGR_CC3G              TIM_EGR_CC3G_Msk                             /*!<Capture/Compare 3 Generation */
#define TIM_EGR_CC4G_Pos          (4U)
#define TIM_EGR_CC4G_Msk          (0x1UL << TIM_EGR_CC4G_Pos)                  /*!< 0x00000010 */
#define TIM_EGR_CC4G              TIM_EGR_CC4G_Msk                             /*!<Capture/Compare 4 Generation */
#define TIM_EGR_COMG_Pos          (5U)
#define TIM_EGR_COMG_Msk          (0x1UL << TIM_EGR_COMG_Pos)                  /*!< 0x00000020 */
#define TIM_EGR_COMG              TIM_EGR_COMG_Msk                             /*!<Capture/Compare Control Update Generation */
#define TIM_EGR_TG_Pos            (6U)
#define TIM_EGR_TG_Msk            (0x1UL << TIM_EGR_TG_Pos)                    /*!< 0x00000040 */
#define TIM_EGR_TG                TIM_EGR_TG_Msk                               /*!<Trigger Generation */
#define TIM_EGR_BG_Pos            (7U)
#define TIM_EGR_BG_Msk            (0x1UL << TIM_EGR_BG_Pos)                    /*!< 0x00000080 */
#define TIM_EGR_BG                TIM_EGR_BG_Msk                               /*!<Break Generation */
#define TIM_EGR_B2G_Pos           (8U)
#define TIM_EGR_B2G_Msk           (0x1UL << TIM_EGR_B2G_Pos)                   /*!< 0x00000100 */
#define TIM_EGR_B2G               TIM_EGR_B2G_Msk                              /*!<Break 2 Generation */


/******************  Bit definition for TIM_CCMR1 register  *******************/
#define TIM_CCMR1_CC1S_Pos        (0U)
#define TIM_CCMR1_CC1S_Msk        (0x3UL << TIM_CCMR1_CC1S_Pos)                /*!< 0x00000003 */
#define TIM_CCMR1_CC1S            TIM_CCMR1_CC1S_Msk                           /*!<CC1S[1:0] bits (Capture/Compare 1 Selection) */
#define TIM_CCMR1_CC1S_0          (0x1UL << TIM_CCMR1_CC1S_Pos)                /*!< 0x00000001 */
#define TIM_CCMR1_CC1S_1          (0x2UL << TIM_CCMR1_CC1S_Pos)                /*!< 0x00000002 */

#define TIM_CCMR1_OC1FE_Pos       (2U)
#define TIM_CCMR1_OC1FE_Msk       (0x1UL << TIM_CCMR1_OC1FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR1_OC1FE           TIM_CCMR1_OC1FE_Msk                          /*!<Output Compare 1 Fast enable */
#define TIM_CCMR1_OC1PE_Pos       (3U)
#define TIM_CCMR1_OC1PE_Msk       (0x1UL << TIM_CCMR1_OC1PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR1_OC1PE           TIM_CCMR1_OC1PE_Msk                          /*!<Output Compare 1 Preload enable */

#define TIM_CCMR1_OC1M_Pos        (4U)
#define TIM_CCMR1_OC1M_Msk        (0x1007UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00010070 */
#define TIM_CCMR1_OC1M            TIM_CCMR1_OC1M_Msk                           /*!<OC1M[2:0] bits (Output Compare 1 Mode) */
#define TIM_CCMR1_OC1M_0          (0x0001UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00000010 */
#define TIM_CCMR1_OC1M_1          (0x0002UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00000020 */
#define TIM_CCMR1_OC1M_2          (0x0004UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00000040 */
#define TIM_CCMR1_OC1M_3          (0x1000UL << TIM_CCMR1_OC1M_Pos)             /*!< 0x00010000 */

#define TIM_CCMR1_OC1CE_Pos       (7U)
#define TIM_CCMR1_OC1CE_Msk       (0x1UL << TIM_CCMR1_OC1CE_Pos)               /*!< 0x00000080 */
#define TIM_CCMR1_OC1CE           TIM_CCMR1_OC1CE_Msk                          /*!<Output Compare 1 Clear Enable */

#define TIM_CCMR1_CC2S_Pos        (8U)
#define TIM_CCMR1_CC2S_Msk        (0x3UL << TIM_CCMR1_CC2S_Pos)                /*!< 0x00000300 */
#define TIM_CCMR1_CC2S            TIM_CCMR1_CC2S_Msk                           /*!<CC2S[1:0] bits (Capture/Compare 2 Selection) */
#define TIM_CCMR1_CC2S_0          (0x1UL << TIM_CCMR1_CC2S_Pos)                /*!< 0x00000100 */
#define TIM_CCMR1_CC2S_1          (0x2UL << TIM_CCMR1_CC2S_Pos)                /*!< 0x00000200 */

#define TIM_CCMR1_OC2FE_Pos       (10U)
#define TIM_CCMR1_OC2FE_Msk       (0x1UL << TIM_CCMR1_OC2FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR1_OC2FE           TIM_CCMR1_OC2FE_Msk                          /*!<Output Compare 2 Fast enable */
#define TIM_CCMR1_OC2PE_Pos       (11U)
#define TIM_CCMR1_OC2PE_Msk       (0x1UL << TIM_CCMR1_OC2PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR1_OC2PE           TIM_CCMR1_OC2PE_Msk                          /*!<Output Compare 2 Preload enable */

#define TIM_CCMR1_OC2M_Pos        (12U)
#define TIM_CCMR1_OC2M_Msk        (0x1007UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x01007000 */
#define TIM_CCMR1_OC2M            TIM_CCMR1_OC2M_Msk                           /*!<OC2M[2:0] bits (Output Compare 2 Mode) */
#define TIM_CCMR1_OC2M_0          (0x0001UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x00001000 */
#define TIM_CCMR1_OC2M_1          (0x0002UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x00002000 */
#define TIM_CCMR1_OC2M_2          (0x0004UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x00004000 */
#define TIM_CCMR1_OC2M_3          (0x1000UL << TIM_CCMR1_OC2M_Pos)             /*!< 0x01000000 */

#define TIM_CCMR1_OC2CE_Pos       (15U)
#define TIM_CCMR1_OC2CE_Msk       (0x1UL << TIM_CCMR1_OC2CE_Pos)               /*!< 0x00008000 */
#define TIM_CCMR1_OC2CE           TIM_CCMR1_OC2CE_Msk                          /*!<Output Compare 2 Clear Enable */

/*----------------------------------------------------------------------------*/
#define TIM_CCMR1_IC1PSC_Pos      (2U)
#define TIM_CCMR1_IC1PSC_Msk      (0x3UL << TIM_CCMR1_IC1PSC_Pos)              /*!< 0x0000000C */
#define TIM_CCMR1_IC1PSC          TIM_CCMR1_IC1PSC_Msk                         /*!<IC1PSC[1:0] bits (Input Capture 1 Prescaler) */
#define TIM_CCMR1_IC1PSC_0        (0x1UL << TIM_CCMR1_IC1PSC_Pos)              /*!< 0x00000004 */
#define TIM_CCMR1_IC1PSC_1        (0x2UL << TIM_CCMR1_IC1PSC_Pos)              /*!< 0x00000008 */

#define TIM_CCMR1_IC1F_Pos        (4U)
#define TIM_CCMR1_IC1F_Msk        (0xFUL << TIM_CCMR1_IC1F_Pos)                /*!< 0x000000F0 */
#define TIM_CCMR1_IC1F            TIM_CCMR1_IC1F_Msk                           /*!<IC1F[3:0] bits (Input Capture 1 Filter) */
#define TIM_CCMR1_IC1F_0          (0x1UL << TIM_CCMR1_IC1F_Pos)                /*!< 0x00000010 */
#define TIM_CCMR1_IC1F_1          (0x2UL << TIM_CCMR1_IC1F_Pos)                /*!< 0x00000020 */
#define TIM_CCMR1_IC1F_2          (0x4UL << TIM_CCMR1_IC1F_Pos)                /*!< 0x00000040 */
#define TIM_CCMR1_IC1F_3          (0x8UL << TIM_CCMR1_IC1F_Pos)                /*!< 0x00000080 */

#define TIM_CCMR1_IC2PSC_Pos      (10U)
#define TIM_CCMR1_IC2PSC_Msk      (0x3UL << TIM_CCMR1_IC2PSC_Pos)              /*!< 0x00000C00 */
#define TIM_CCMR1_IC2PSC          TIM_CCMR1_IC2PSC_Msk                         /*!<IC2PSC[1:0] bits (Input Capture 2 Prescaler) */
#define TIM_CCMR1_IC2PSC_0        (0x1UL << TIM_CCMR1_IC2PSC_Pos)              /*!< 0x00000400 */
#define TIM_CCMR1_IC2PSC_1        (0x2UL << TIM_CCMR1_IC2PSC_Pos)              /*!< 0x00000800 */

#define TIM_CCMR1_IC2F_Pos        (12U)
#define TIM_CCMR1_IC2F_Msk        (0xFUL << TIM_CCMR1_IC2F_Pos)                /*!< 0x0000F000 */
#define TIM_CCMR1_IC2F            TIM_CCMR1_IC2F_Msk                           /*!<IC2F[3:0] bits (Input Capture 2 Filter) */
#define TIM_CCMR1_IC2F_0          (0x1UL << TIM_CCMR1_IC2F_Pos)                /*!< 0x00001000 */
#define TIM_CCMR1_IC2F_1          (0x2UL << TIM_CCMR1_IC2F_Pos)                /*!< 0x00002000 */
#define TIM_CCMR1_IC2F_2          (0x4UL << TIM_CCMR1_IC2F_Pos)                /*!< 0x00004000 */
#define TIM_CCMR1_IC2F_3          (0x8UL << TIM_CCMR1_IC2F_Pos)                /*!< 0x00008000 */

/******************  Bit definition for TIM_CCMR2 register  *******************/
#define TIM_CCMR2_CC3S_Pos        (0U)
#define TIM_CCMR2_CC3S_Msk        (0x3UL << TIM_CCMR2_CC3S_Pos)                /*!< 0x00000003 */
#define TIM_CCMR2_CC3S            TIM_CCMR2_CC3S_Msk                           /*!<CC3S[1:0] bits (Capture/Compare 3 Selection) */
#define TIM_CCMR2_CC3S_0          (0x1UL << TIM_CCMR2_CC3S_Pos)                /*!< 0x00000001 */
#define TIM_CCMR2_CC3S_1          (0x2UL << TIM_CCMR2_CC3S_Pos)                /*!< 0x00000002 */

#define TIM_CCMR2_OC3FE_Pos       (2U)
#define TIM_CCMR2_OC3FE_Msk       (0x1UL << TIM_CCMR2_OC3FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR2_OC3FE           TIM_CCMR2_OC3FE_Msk                          /*!<Output Compare 3 Fast enable */
#define TIM_CCMR2_OC3PE_Pos       (3U)
#define TIM_CCMR2_OC3PE_Msk       (0x1UL << TIM_CCMR2_OC3PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR2_OC3PE           TIM_CCMR2_OC3PE_Msk                          /*!<Output Compare 3 Preload enable */

#define TIM_CCMR2_OC3M_Pos        (4U)
#define TIM_CCMR2_OC3M_Msk        (0x1007UL << TIM_CCMR2_OC3M_Pos)             /*!< 0x00010070 */
#define TIM_CCMR2_OC3M            TIM_CCMR2_OC3M_Msk                           /*!<OC3M[2:0] bits (Output Compare 3 Mode) */
#define TIM_CCMR2_OC3M_0          (0x0001UL << TIM_CCMR2_OC3M_Pos)             /*!< 0x00000010 */
#define TIM_CCMR2_OC3M_1          (0x0002UL << TIM_CCMR2_OC3M_Pos)             /*!< 0x00000020 */
#define TIM_CCMR2_OC3M_2          (0x0004UL << TIM_CCMR2_OC3M_Pos)             /*!< 0x00000040 */
#define TIM_CCMR2_OC3M_3          (0x1000UL << TIM_CCMR2_OC3M_Pos)             /*!< 0x00010000 */

#define TIM_CCMR2_OC3CE_Pos       (7U)
#define TIM_CCMR2_OC3CE_Msk       (0x1UL << TIM_CCMR2_OC3CE_Pos)               /*!< 0x00000080 */
#define TIM_CCMR2_OC3CE           TIM_CCMR2_OC3CE_Msk                          /*!<Output Compare 3 Clear Enable */

#define TIM_CCMR2_CC4S_Pos        (8U)
#define TIM_CCMR2_CC4S_Msk        (0x3UL << TIM_CCMR2_CC4S_Pos)                /*!< 0x00000300 */
#define TIM_CCMR2_CC4S            TIM_CCMR2_CC4S_Msk                           /*!<CC4S[1:0] bits (Capture/Compare 4 Selection) */
#define TIM_CCMR2_CC4S_0          (0x1UL << TIM_CCMR2_CC4S_Pos)                /*!< 0x00000100 */
#define TIM_CCMR2_CC4S_1          (0x2UL << TIM_CCMR2_CC4S_Pos)                /*!< 0x00000200 */

#define TIM_CCMR2_OC4FE_Pos       (10U)
#define TIM_CCMR2_OC4FE_Msk       (0x1UL << TIM_CCMR2_OC4FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR2_OC4FE           TIM_CCMR2_OC4FE_Msk                          /*!<Output Compare 4 Fast enable */
#define TIM_CCMR2_OC4PE_Pos       (11U)
#define TIM_CCMR2_OC4PE_Msk       (0x1UL << TIM_CCMR2_OC4PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR2_OC4PE           TIM_CCMR2_OC4PE_Msk                          /*!<Output Compare 4 Preload enable */

#define TIM_CCMR2_OC4M_Pos        (12U)
#define TIM_CCMR2_OC4M_Msk        (0x1007UL << TIM_CCMR2_OC4M_Pos)             /*!< 0x01007000 */
#define TIM_CCMR2_OC4M            TIM_CCMR2_OC4M_Msk                           /*!<OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_CCMR2_OC4M_0          (0x0001UL << TIM_CCMR2_OC4M_Pos)             /*!< 0x00001000 */
#define TIM_CCMR2_OC4M_1          (0x0002UL << TIM_CCMR2_OC4M_Pos)             /*!< 0x00002000 */
#define TIM_CCMR2_OC4M_2          (0x0004UL << TIM_CCMR2_OC4M_Pos)             /*!< 0x00004000 */
#define TIM_CCMR2_OC4M_3          (0x1000UL << TIM_CCMR2_OC4M_Pos)             /*!< 0x01000000 */

#define TIM_CCMR2_OC4CE_Pos       (15U)
#define TIM_CCMR2_OC4CE_Msk       (0x1UL << TIM_CCMR2_OC4CE_Pos)               /*!< 0x00008000 */
#define TIM_CCMR2_OC4CE           TIM_CCMR2_OC4CE_Msk                          /*!<Output Compare 4 Clear Enable */

/*----------------------------------------------------------------------------*/
#define TIM_CCMR2_IC3PSC_Pos      (2U)
#define TIM_CCMR2_IC3PSC_Msk      (0x3UL << TIM_CCMR2_IC3PSC_Pos)              /*!< 0x0000000C */
#define TIM_CCMR2_IC3PSC          TIM_CCMR2_IC3PSC_Msk                         /*!<IC3PSC[1:0] bits (Input Capture 3 Prescaler) */
#define TIM_CCMR2_IC3PSC_0        (0x1UL << TIM_CCMR2_IC3PSC_Pos)              /*!< 0x00000004 */
#define TIM_CCMR2_IC3PSC_1        (0x2UL << TIM_CCMR2_IC3PSC_Pos)              /*!< 0x00000008 */

#define TIM_CCMR2_IC3F_Pos        (4U)
#define TIM_CCMR2_IC3F_Msk        (0xFUL << TIM_CCMR2_IC3F_Pos)                /*!< 0x000000F0 */
#define TIM_CCMR2_IC3F            TIM_CCMR2_IC3F_Msk                           /*!<IC3F[3:0] bits (Input Capture 3 Filter) */
#define TIM_CCMR2_IC3F_0          (0x1UL << TIM_CCMR2_IC3F_Pos)                /*!< 0x00000010 */
#define TIM_CCMR2_IC3F_1          (0x2UL << TIM_CCMR2_IC3F_Pos)                /*!< 0x00000020 */
#define TIM_CCMR2_IC3F_2          (0x4UL << TIM_CCMR2_IC3F_Pos)                /*!< 0x00000040 */
#define TIM_CCMR2_IC3F_3          (0x8UL << TIM_CCMR2_IC3F_Pos)                /*!< 0x00000080 */

#define TIM_CCMR2_IC4PSC_Pos      (10U)
#define TIM_CCMR2_IC4PSC_Msk      (0x3UL << TIM_CCMR2_IC4PSC_Pos)              /*!< 0x00000C00 */
#define TIM_CCMR2_IC4PSC          TIM_CCMR2_IC4PSC_Msk                         /*!<IC4PSC[1:0] bits (Input Capture 4 Prescaler) */
#define TIM_CCMR2_IC4PSC_0        (0x1UL << TIM_CCMR2_IC4PSC_Pos)              /*!< 0x00000400 */
#define TIM_CCMR2_IC4PSC_1        (0x2UL << TIM_CCMR2_IC4PSC_Pos)              /*!< 0x00000800 */

#define TIM_CCMR2_IC4F_Pos        (12U)
#define TIM_CCMR2_IC4F_Msk        (0xFUL << TIM_CCMR2_IC4F_Pos)                /*!< 0x0000F000 */
#define TIM_CCMR2_IC4F            TIM_CCMR2_IC4F_Msk                           /*!<IC4F[3:0] bits (Input Capture 4 Filter) */
#define TIM_CCMR2_IC4F_0          (0x1UL << TIM_CCMR2_IC4F_Pos)                /*!< 0x00001000 */
#define TIM_CCMR2_IC4F_1          (0x2UL << TIM_CCMR2_IC4F_Pos)                /*!< 0x00002000 */
#define TIM_CCMR2_IC4F_2          (0x4UL << TIM_CCMR2_IC4F_Pos)                /*!< 0x00004000 */
#define TIM_CCMR2_IC4F_3          (0x8UL << TIM_CCMR2_IC4F_Pos)                /*!< 0x00008000 */

/******************  Bit definition for TIM_CCMR3 register  *******************/
#define TIM_CCMR3_OC5FE_Pos       (2U)
#define TIM_CCMR3_OC5FE_Msk       (0x1UL << TIM_CCMR3_OC5FE_Pos)               /*!< 0x00000004 */
#define TIM_CCMR3_OC5FE           TIM_CCMR3_OC5FE_Msk                          /*!<Output Compare 5 Fast enable */
#define TIM_CCMR3_OC5PE_Pos       (3U)
#define TIM_CCMR3_OC5PE_Msk       (0x1UL << TIM_CCMR3_OC5PE_Pos)               /*!< 0x00000008 */
#define TIM_CCMR3_OC5PE           TIM_CCMR3_OC5PE_Msk                          /*!<Output Compare 5 Preload enable */

#define TIM_CCMR3_OC5M_Pos        (4U)
#define TIM_CCMR3_OC5M_Msk        (0x1007UL << TIM_CCMR3_OC5M_Pos)             /*!< 0x00010070 */
#define TIM_CCMR3_OC5M            TIM_CCMR3_OC5M_Msk                           /*!<OC5M[3:0] bits (Output Compare 5 Mode) */
#define TIM_CCMR3_OC5M_0          (0x0001UL << TIM_CCMR3_OC5M_Pos)             /*!< 0x00000010 */
#define TIM_CCMR3_OC5M_1          (0x0002UL << TIM_CCMR3_OC5M_Pos)             /*!< 0x00000020 */
#define TIM_CCMR3_OC5M_2          (0x0004UL << TIM_CCMR3_OC5M_Pos)             /*!< 0x00000040 */
#define TIM_CCMR3_OC5M_3          (0x1000UL << TIM_CCMR3_OC5M_Pos)             /*!< 0x00010000 */

#define TIM_CCMR3_OC5CE_Pos       (7U)
#define TIM_CCMR3_OC5CE_Msk       (0x1UL << TIM_CCMR3_OC5CE_Pos)               /*!< 0x00000080 */
#define TIM_CCMR3_OC5CE           TIM_CCMR3_OC5CE_Msk                          /*!<Output Compare 5 Clear Enable */

#define TIM_CCMR3_OC6FE_Pos       (10U)
#define TIM_CCMR3_OC6FE_Msk       (0x1UL << TIM_CCMR3_OC6FE_Pos)               /*!< 0x00000400 */
#define TIM_CCMR3_OC6FE           TIM_CCMR3_OC6FE_Msk                          /*!<Output Compare 6 Fast enable */
#define TIM_CCMR3_OC6PE_Pos       (11U)
#define TIM_CCMR3_OC6PE_Msk       (0x1UL << TIM_CCMR3_OC6PE_Pos)               /*!< 0x00000800 */
#define TIM_CCMR3_OC6PE           TIM_CCMR3_OC6PE_Msk                          /*!<Output Compare 6 Preload enable */

#define TIM_CCMR3_OC6M_Pos        (12U)
#define TIM_CCMR3_OC6M_Msk        (0x1007UL << TIM_CCMR3_OC6M_Pos)             /*!< 0x01007000 */
#define TIM_CCMR3_OC6M            TIM_CCMR3_OC6M_Msk                           /*!<OC6M[3:0] bits (Output Compare 6 Mode) */
#define TIM_CCMR3_OC6M_0          (0x0001UL << TIM_CCMR3_OC6M_Pos)             /*!< 0x00001000 */
#define TIM_CCMR3_OC6M_1          (0x0002UL << TIM_CCMR3_OC6M_Pos)             /*!< 0x00002000 */
#define TIM_CCMR3_OC6M_2          (0x0004UL << TIM_CCMR3_OC6M_Pos)             /*!< 0x00004000 */
#define TIM_CCMR3_OC6M_3          (0x1000UL << TIM_CCMR3_OC6M_Pos)             /*!< 0x01000000 */

#define TIM_CCMR3_OC6CE_Pos       (15U)
#define TIM_CCMR3_OC6CE_Msk       (0x1UL << TIM_CCMR3_OC6CE_Pos)               /*!< 0x00008000 */
#define TIM_CCMR3_OC6CE           TIM_CCMR3_OC6CE_Msk                          /*!<Output Compare 6 Clear Enable */

/*******************  Bit definition for TIM_CCER register  *******************/
#define TIM_CCER_CC1E_Pos         (0U)
#define TIM_CCER_CC1E_Msk         (0x1UL << TIM_CCER_CC1E_Pos)                 /*!< 0x00000001 */
#define TIM_CCER_CC1E             TIM_CCER_CC1E_Msk                            /*!<Capture/Compare 1 output enable */
#define TIM_CCER_CC1P_Pos         (1U)
#define TIM_CCER_CC1P_Msk         (0x1UL << TIM_CCER_CC1P_Pos)                 /*!< 0x00000002 */
#define TIM_CCER_CC1P             TIM_CCER_CC1P_Msk                            /*!<Capture/Compare 1 output Polarity */
#define TIM_CCER_CC1NE_Pos        (2U)
#define TIM_CCER_CC1NE_Msk        (0x1UL << TIM_CCER_CC1NE_Pos)                /*!< 0x00000004 */
#define TIM_CCER_CC1NE            TIM_CCER_CC1NE_Msk                           /*!<Capture/Compare 1 Complementary output enable */
#define TIM_CCER_CC1NP_Pos        (3U)
#define TIM_CCER_CC1NP_Msk        (0x1UL << TIM_CCER_CC1NP_Pos)                /*!< 0x00000008 */
#define TIM_CCER_CC1NP            TIM_CCER_CC1NP_Msk                           /*!<Capture/Compare 1 Complementary output Polarity */
#define TIM_CCER_CC2E_Pos         (4U)
#define TIM_CCER_CC2E_Msk         (0x1UL << TIM_CCER_CC2E_Pos)                 /*!< 0x00000010 */
#define TIM_CCER_CC2E             TIM_CCER_CC2E_Msk                            /*!<Capture/Compare 2 output enable */
#define TIM_CCER_CC2P_Pos         (5U)
#define TIM_CCER_CC2P_Msk         (0x1UL << TIM_CCER_CC2P_Pos)                 /*!< 0x00000020 */
#define TIM_CCER_CC2P             TIM_CCER_CC2P_Msk                            /*!<Capture/Compare 2 output Polarity */
#define TIM_CCER_CC2NE_Pos        (6U)
#define TIM_CCER_CC2NE_Msk        (0x1UL << TIM_CCER_CC2NE_Pos)                /*!< 0x00000040 */
#define TIM_CCER_CC2NE            TIM_CCER_CC2NE_Msk                           /*!<Capture/Compare 2 Complementary output enable */
#define TIM_CCER_CC2NP_Pos        (7U)
#define TIM_CCER_CC2NP_Msk        (0x1UL << TIM_CCER_CC2NP_Pos)                /*!< 0x00000080 */
#define TIM_CCER_CC2NP            TIM_CCER_CC2NP_Msk                           /*!<Capture/Compare 2 Complementary output Polarity */
#define TIM_CCER_CC3E_Pos         (8U)
#define TIM_CCER_CC3E_Msk         (0x1UL << TIM_CCER_CC3E_Pos)                 /*!< 0x00000100 */
#define TIM_CCER_CC3E             TIM_CCER_CC3E_Msk                            /*!<Capture/Compare 3 output enable */
#define TIM_CCER_CC3P_Pos         (9U)
#define TIM_CCER_CC3P_Msk         (0x1UL << TIM_CCER_CC3P_Pos)                 /*!< 0x00000200 */
#define TIM_CCER_CC3P             TIM_CCER_CC3P_Msk                            /*!<Capture/Compare 3 output Polarity */
#define TIM_CCER_CC3NE_Pos        (10U)
#define TIM_CCER_CC3NE_Msk        (0x1UL << TIM_CCER_CC3NE_Pos)                /*!< 0x00000400 */
#define TIM_CCER_CC3NE            TIM_CCER_CC3NE_Msk                           /*!<Capture/Compare 3 Complementary output enable */
#define TIM_CCER_CC3NP_Pos        (11U)
#define TIM_CCER_CC3NP_Msk        (0x1UL << TIM_CCER_CC3NP_Pos)                /*!< 0x00000800 */
#define TIM_CCER_CC3NP            TIM_CCER_CC3NP_Msk                           /*!<Capture/Compare 3 Complementary output Polarity */
#define TIM_CCER_CC4E_Pos         (12U)
#define TIM_CCER_CC4E_Msk         (0x1UL << TIM_CCER_CC4E_Pos)                 /*!< 0x00001000 */
#define TIM_CCER_CC4E             TIM_CCER_CC4E_Msk                            /*!<Capture/Compare 4 output enable */
#define TIM_CCER_CC4P_Pos         (13U)
#define TIM_CCER_CC4P_Msk         (0x1UL << TIM_CCER_CC4P_Pos)                 /*!< 0x00002000 */
#define TIM_CCER_CC4P             TIM_CCER_CC4P_Msk                            /*!<Capture/Compare 4 output Polarity */
#define TIM_CCER_CC4NP_Pos        (15U)
#define TIM_CCER_CC4NP_Msk        (0x1UL << TIM_CCER_CC4NP_Pos)                /*!< 0x00008000 */
#define TIM_CCER_CC4NP            TIM_CCER_CC4NP_Msk                           /*!<Capture/Compare 4 Complementary output Polarity */
#define TIM_CCER_CC5E_Pos         (16U)
#define TIM_CCER_CC5E_Msk         (0x1UL << TIM_CCER_CC5E_Pos)                 /*!< 0x00010000 */
#define TIM_CCER_CC5E             TIM_CCER_CC5E_Msk                            /*!<Capture/Compare 5 output enable */
#define TIM_CCER_CC5P_Pos         (17U)
#define TIM_CCER_CC5P_Msk         (0x1UL << TIM_CCER_CC5P_Pos)                 /*!< 0x00020000 */
#define TIM_CCER_CC5P             TIM_CCER_CC5P_Msk                            /*!<Capture/Compare 5 output Polarity */
#define TIM_CCER_CC6E_Pos         (20U)
#define TIM_CCER_CC6E_Msk         (0x1UL << TIM_CCER_CC6E_Pos)                 /*!< 0x00100000 */
#define TIM_CCER_CC6E             TIM_CCER_CC6E_Msk                            /*!<Capture/Compare 6 output enable */
#define TIM_CCER_CC6P_Pos         (21U)
#define TIM_CCER_CC6P_Msk         (0x1UL << TIM_CCER_CC6P_Pos)                 /*!< 0x00200000 */
#define TIM_CCER_CC6P             TIM_CCER_CC6P_Msk                            /*!<Capture/Compare 6 output Polarity */

/*******************  Bit definition for TIM_CNT register  ********************/
#define TIM_CNT_CNT_Pos           (0U)
#define TIM_CNT_CNT_Msk           (0xFFFFFFFFUL << TIM_CNT_CNT_Pos)            /*!< 0xFFFFFFFF */
#define TIM_CNT_CNT               TIM_CNT_CNT_Msk                              /*!<Counter Value */
#define TIM_CNT_UIFCPY_Pos        (31U)
#define TIM_CNT_UIFCPY_Msk        (0x1UL << TIM_CNT_UIFCPY_Pos)                /*!< 0x80000000 */
#define TIM_CNT_UIFCPY            TIM_CNT_UIFCPY_Msk                           /*!<Update interrupt flag copy (if UIFREMAP=1) */

/*******************  Bit definition for TIM_PSC register  ********************/
#define TIM_PSC_PSC_Pos           (0U)
#define TIM_PSC_PSC_Msk           (0xFFFFUL << TIM_PSC_PSC_Pos)                /*!< 0x0000FFFF */
#define TIM_PSC_PSC               TIM_PSC_PSC_Msk                              /*!<Prescaler Value */

/*******************  Bit definition for TIM_ARR register  ********************/
#define TIM_ARR_ARR_Pos           (0U)
#define TIM_ARR_ARR_Msk           (0xFFFFFFFFUL << TIM_ARR_ARR_Pos)            /*!< 0xFFFFFFFF */
#define TIM_ARR_ARR               TIM_ARR_ARR_Msk                              /*!<Actual auto-reload Value */

/*******************  Bit definition for TIM_RCR register  ********************/
#define TIM_RCR_REP_Pos           (0U)
#define TIM_RCR_REP_Msk           (0xFFFFUL << TIM_RCR_REP_Pos)                /*!< 0x0000FFFF */
#define TIM_RCR_REP               TIM_RCR_REP_Msk                              /*!<Repetition Counter Value */

/*******************  Bit definition for TIM_CCR1 register  *******************/
#define TIM_CCR1_CCR1_Pos         (0U)
#define TIM_CCR1_CCR1_Msk         (0xFFFFUL << TIM_CCR1_CCR1_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR1_CCR1             TIM_CCR1_CCR1_Msk                            /*!<Capture/Compare 1 Value */

/*******************  Bit definition for TIM_CCR2 register  *******************/
#define TIM_CCR2_CCR2_Pos         (0U)
#define TIM_CCR2_CCR2_Msk         (0xFFFFUL << TIM_CCR2_CCR2_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR2_CCR2             TIM_CCR2_CCR2_Msk                            /*!<Capture/Compare 2 Value */

/*******************  Bit definition for TIM_CCR3 register  *******************/
#define TIM_CCR3_CCR3_Pos         (0U)
#define TIM_CCR3_CCR3_Msk         (0xFFFFUL << TIM_CCR3_CCR3_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR3_CCR3             TIM_CCR3_CCR3_Msk                            /*!<Capture/Compare 3 Value */

/*******************  Bit definition for TIM_CCR4 register  *******************/
#define TIM_CCR4_CCR4_Pos         (0U)
#define TIM_CCR4_CCR4_Msk         (0xFFFFUL << TIM_CCR4_CCR4_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR4_CCR4             TIM_CCR4_CCR4_Msk                            /*!<Capture/Compare 4 Value */

/*******************  Bit definition for TIM_CCR5 register  *******************/
#define TIM_CCR5_CCR5_Pos         (0U)
#define TIM_CCR5_CCR5_Msk         (0xFFFFFFFFUL << TIM_CCR5_CCR5_Pos)          /*!< 0xFFFFFFFF */
#define TIM_CCR5_CCR5             TIM_CCR5_CCR5_Msk                            /*!<Capture/Compare 5 Value */
#define TIM_CCR5_GC5C1_Pos        (29U)
#define TIM_CCR5_GC5C1_Msk        (0x1UL << TIM_CCR5_GC5C1_Pos)                /*!< 0x20000000 */
#define TIM_CCR5_GC5C1            TIM_CCR5_GC5C1_Msk                           /*!<Group Channel 5 and Channel 1 */
#define TIM_CCR5_GC5C2_Pos        (30U)
#define TIM_CCR5_GC5C2_Msk        (0x1UL << TIM_CCR5_GC5C2_Pos)                /*!< 0x40000000 */
#define TIM_CCR5_GC5C2            TIM_CCR5_GC5C2_Msk                           /*!<Group Channel 5 and Channel 2 */
#define TIM_CCR5_GC5C3_Pos        (31U)
#define TIM_CCR5_GC5C3_Msk        (0x1UL << TIM_CCR5_GC5C3_Pos)                /*!< 0x80000000 */
#define TIM_CCR5_GC5C3            TIM_CCR5_GC5C3_Msk                           /*!<Group Channel 5 and Channel 3 */

/*******************  Bit definition for TIM_CCR6 register  *******************/
#define TIM_CCR6_CCR6_Pos         (0U)
#define TIM_CCR6_CCR6_Msk         (0xFFFFUL << TIM_CCR6_CCR6_Pos)              /*!< 0x0000FFFF */
#define TIM_CCR6_CCR6             TIM_CCR6_CCR6_Msk                            /*!<Capture/Compare 6 Value */

/*******************  Bit definition for TIM_BDTR register  *******************/
#define TIM_BDTR_DTG_Pos          (0U)
#define TIM_BDTR_DTG_Msk          (0xFFUL << TIM_BDTR_DTG_Pos)                 /*!< 0x000000FF */
#define TIM_BDTR_DTG              TIM_BDTR_DTG_Msk                             /*!<DTG[0:7] bits (Dead-Time Generator set-up) */
#define TIM_BDTR_DTG_0            (0x01UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000001 */
#define TIM_BDTR_DTG_1            (0x02UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000002 */
#define TIM_BDTR_DTG_2            (0x04UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000004 */
#define TIM_BDTR_DTG_3            (0x08UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000008 */
#define TIM_BDTR_DTG_4            (0x10UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000010 */
#define TIM_BDTR_DTG_5            (0x20UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000020 */
#define TIM_BDTR_DTG_6            (0x40UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000040 */
#define TIM_BDTR_DTG_7            (0x80UL << TIM_BDTR_DTG_Pos)                 /*!< 0x00000080 */

#define TIM_BDTR_LOCK_Pos         (8U)
#define TIM_BDTR_LOCK_Msk         (0x3UL << TIM_BDTR_LOCK_Pos)                 /*!< 0x00000300 */
#define TIM_BDTR_LOCK             TIM_BDTR_LOCK_Msk                            /*!<LOCK[1:0] bits (Lock Configuration) */
#define TIM_BDTR_LOCK_0           (0x1UL << TIM_BDTR_LOCK_Pos)                 /*!< 0x00000100 */
#define TIM_BDTR_LOCK_1           (0x2UL << TIM_BDTR_LOCK_Pos)                 /*!< 0x00000200 */

#define TIM_BDTR_OSSI_Pos         (10U)
#define TIM_BDTR_OSSI_Msk         (0x1UL << TIM_BDTR_OSSI_Pos)                 /*!< 0x00000400 */
#define TIM_BDTR_OSSI             TIM_BDTR_OSSI_Msk                            /*!<Off-State Selection for Idle mode */
#define TIM_BDTR_OSSR_Pos         (11U)
#define TIM_BDTR_OSSR_Msk         (0x1UL << TIM_BDTR_OSSR_Pos)                 /*!< 0x00000800 */
#define TIM_BDTR_OSSR             TIM_BDTR_OSSR_Msk                            /*!<Off-State Selection for Run mode */
#define TIM_BDTR_BKE_Pos          (12U)
#define TIM_BDTR_BKE_Msk          (0x1UL << TIM_BDTR_BKE_Pos)                  /*!< 0x00001000 */
#define TIM_BDTR_BKE              TIM_BDTR_BKE_Msk                             /*!<Break enable for Break 1 */
#define TIM_BDTR_BKP_Pos          (13U)
#define TIM_BDTR_BKP_Msk          (0x1UL << TIM_BDTR_BKP_Pos)                  /*!< 0x00002000 */
#define TIM_BDTR_BKP              TIM_BDTR_BKP_Msk                             /*!<Break Polarity for Break 1 */
#define TIM_BDTR_AOE_Pos          (14U)
#define TIM_BDTR_AOE_Msk          (0x1UL << TIM_BDTR_AOE_Pos)                  /*!< 0x00004000 */
#define TIM_BDTR_AOE              TIM_BDTR_AOE_Msk                             /*!<Automatic Output enable */
#define TIM_BDTR_MOE_Pos          (15U)
#define TIM_BDTR_MOE_Msk          (0x1UL << TIM_BDTR_MOE_Pos)                  /*!< 0x00008000 */
#define TIM_BDTR_MOE              TIM_BDTR_MOE_Msk                             /*!<Main Output enable */

#define TIM_BDTR_BKF_Pos          (16U)
#define TIM_BDTR_BKF_Msk          (0xFUL << TIM_BDTR_BKF_Pos)                  /*!< 0x000F0000 */
#define TIM_BDTR_BKF              TIM_BDTR_BKF_Msk                             /*!<Break Filter for Break 1 */
#define TIM_BDTR_BK2F_Pos         (20U)
#define TIM_BDTR_BK2F_Msk         (0xFUL << TIM_BDTR_BK2F_Pos)                 /*!< 0x00F00000 */
#define TIM_BDTR_BK2F             TIM_BDTR_BK2F_Msk                            /*!<Break Filter for Break 2 */

#define TIM_BDTR_BK2E_Pos         (24U)
#define TIM_BDTR_BK2E_Msk         (0x1UL << TIM_BDTR_BK2E_Pos)                 /*!< 0x01000000 */
#define TIM_BDTR_BK2E             TIM_BDTR_BK2E_Msk                            /*!<Break enable for Break 2 */
#define TIM_BDTR_BK2P_Pos         (25U)
#define TIM_BDTR_BK2P_Msk         (0x1UL << TIM_BDTR_BK2P_Pos)                 /*!< 0x02000000 */
#define TIM_BDTR_BK2P             TIM_BDTR_BK2P_Msk                            /*!<Break Polarity for Break 2 */

#define TIM_BDTR_BKDSRM_Pos       (26U)
#define TIM_BDTR_BKDSRM_Msk       (0x1UL << TIM_BDTR_BKDSRM_Pos)               /*!< 0x04000000 */
#define TIM_BDTR_BKDSRM           TIM_BDTR_BKDSRM_Msk                          /*!<Break disarming/re-arming */
#define TIM_BDTR_BK2DSRM_Pos      (27U)
#define TIM_BDTR_BK2DSRM_Msk      (0x1UL << TIM_BDTR_BK2DSRM_Pos)              /*!< 0x08000000 */
#define TIM_BDTR_BK2DSRM          TIM_BDTR_BK2DSRM_Msk                         /*!<Break2 disarming/re-arming */

#define TIM_BDTR_BKBID_Pos        (28U)
#define TIM_BDTR_BKBID_Msk        (0x1UL << TIM_BDTR_BKBID_Pos)                /*!< 0x10000000 */
#define TIM_BDTR_BKBID            TIM_BDTR_BKBID_Msk                           /*!<Break BIDirectional */
#define TIM_BDTR_BK2BID_Pos       (29U)
#define TIM_BDTR_BK2BID_Msk       (0x1UL << TIM_BDTR_BK2BID_Pos)               /*!< 0x20000000 */
#define TIM_BDTR_BK2BID           TIM_BDTR_BK2BID_Msk                          /*!<Break2 BIDirectional */

/*******************  Bit definition for TIM_DCR register  ********************/
#define TIM_DCR_DBA_Pos           (0U)
#define TIM_DCR_DBA_Msk           (0x1FUL << TIM_DCR_DBA_Pos)                  /*!< 0x0000001F */
#define TIM_DCR_DBA               TIM_DCR_DBA_Msk                              /*!<DBA[4:0] bits (DMA Base Address) */
#define TIM_DCR_DBA_0             (0x01UL << TIM_DCR_DBA_Pos)                  /*!< 0x00000001 */
#define TIM_DCR_DBA_1             (0x02UL << TIM_DCR_DBA_Pos)                  /*!< 0x00000002 */
#define TIM_DCR_DBA_2             (0x04UL << TIM_DCR_DBA_Pos)                  /*!< 0x00000004 */
#define TIM_DCR_DBA_3             (0x08UL << TIM_DCR_DBA_Pos)                  /*!< 0x00000008 */
#define TIM_DCR_DBA_4             (0x10UL << TIM_DCR_DBA_Pos)                  /*!< 0x00000010 */

#define TIM_DCR_DBL_Pos           (8U)
#define TIM_DCR_DBL_Msk           (0x1FUL << TIM_DCR_DBL_Pos)                  /*!< 0x00001F00 */
#define TIM_DCR_DBL               TIM_DCR_DBL_Msk                              /*!<DBL[4:0] bits (DMA Burst Length) */
#define TIM_DCR_DBL_0             (0x01UL << TIM_DCR_DBL_Pos)                  /*!< 0x00000100 */
#define TIM_DCR_DBL_1             (0x02UL << TIM_DCR_DBL_Pos)                  /*!< 0x00000200 */
#define TIM_DCR_DBL_2             (0x04UL << TIM_DCR_DBL_Pos)                  /*!< 0x00000400 */
#define TIM_DCR_DBL_3             (0x08UL << TIM_DCR_DBL_Pos)                  /*!< 0x00000800 */
#define TIM_DCR_DBL_4             (0x10UL << TIM_DCR_DBL_Pos)                  /*!< 0x00001000 */

/*******************  Bit definition for TIM_DMAR register  *******************/
#define TIM_DMAR_DMAB_Pos         (0U)
#define TIM_DMAR_DMAB_Msk         (0xFFFFUL << TIM_DMAR_DMAB_Pos)              /*!< 0x0000FFFF */
#define TIM_DMAR_DMAB             TIM_DMAR_DMAB_Msk                            /*!<DMA register for burst accesses */

/*******************  Bit definition for TIM1_OR1 register  *******************/
#define TIM1_OR1_OCREF_CLR_Pos     (0U)
#define TIM1_OR1_OCREF_CLR_Msk     (0x1UL << TIM1_OR1_OCREF_CLR_Pos)           /*!< 0x00000001 */
#define TIM1_OR1_OCREF_CLR         TIM1_OR1_OCREF_CLR_Msk                      /*!<OCREF clear input selection */

/*******************  Bit definition for TIM1_AF1 register  *******************/
#define TIM1_AF1_BKINE_Pos        (0U)
#define TIM1_AF1_BKINE_Msk        (0x1UL << TIM1_AF1_BKINE_Pos)                /*!< 0x00000001 */
#define TIM1_AF1_BKINE            TIM1_AF1_BKINE_Msk                           /*!<BRK BKIN input enable */
#define TIM1_AF1_BKCMP1E_Pos      (1U)
#define TIM1_AF1_BKCMP1E_Msk      (0x1UL << TIM1_AF1_BKCMP1E_Pos)              /*!< 0x00000002 */
#define TIM1_AF1_BKCMP1E          TIM1_AF1_BKCMP1E_Msk                         /*!<BRK COMP1 enable */
#define TIM1_AF1_BKCMP2E_Pos      (2U)
#define TIM1_AF1_BKCMP2E_Msk      (0x1UL << TIM1_AF1_BKCMP2E_Pos)              /*!< 0x00000004 */
#define TIM1_AF1_BKCMP2E          TIM1_AF1_BKCMP2E_Msk                         /*!<BRK COMP2 enable */
#define TIM1_AF1_BKINP_Pos        (9U)
#define TIM1_AF1_BKINP_Msk        (0x1UL << TIM1_AF1_BKINP_Pos)                /*!< 0x00000200 */
#define TIM1_AF1_BKINP            TIM1_AF1_BKINP_Msk                           /*!<BRK BKIN input polarity */
#define TIM1_AF1_BKCMP1P_Pos      (10U)
#define TIM1_AF1_BKCMP1P_Msk      (0x1UL << TIM1_AF1_BKCMP1P_Pos)              /*!< 0x00000400 */
#define TIM1_AF1_BKCMP1P          TIM1_AF1_BKCMP1P_Msk                         /*!<BRK COMP1 input polarity */
#define TIM1_AF1_BKCMP2P_Pos      (11U)
#define TIM1_AF1_BKCMP2P_Msk      (0x1UL << TIM1_AF1_BKCMP2P_Pos)              /*!< 0x00000800 */
#define TIM1_AF1_BKCMP2P          TIM1_AF1_BKCMP2P_Msk                         /*!<BRK COMP2 input polarity */

#define TIM1_AF1_ETRSEL_Pos       (14U)
#define TIM1_AF1_ETRSEL_Msk       (0xFUL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x0003C000 */
#define TIM1_AF1_ETRSEL           TIM1_AF1_ETRSEL_Msk                          /*!<ETRSEL[3:0] bits (TIM1 ETR source selection) */
#define TIM1_AF1_ETRSEL_0         (0x1UL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x00004000 */
#define TIM1_AF1_ETRSEL_1         (0x2UL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x00008000 */
#define TIM1_AF1_ETRSEL_2         (0x4UL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x00010000 */
#define TIM1_AF1_ETRSEL_3         (0x8UL << TIM1_AF1_ETRSEL_Pos)               /*!< 0x00020000 */

/*******************  Bit definition for TIM1_AF2 register  *******************/
#define TIM1_AF2_BK2INE_Pos       (0U)
#define TIM1_AF2_BK2INE_Msk       (0x1UL << TIM1_AF2_BK2INE_Pos)               /*!< 0x00000001 */
#define TIM1_AF2_BK2INE           TIM1_AF2_BK2INE_Msk                          /*!<BRK2 BKIN2 input enable */
#define TIM1_AF2_BK2CMP1E_Pos     (1U)
#define TIM1_AF2_BK2CMP1E_Msk     (0x1UL << TIM1_AF2_BK2CMP1E_Pos)             /*!< 0x00000002 */
#define TIM1_AF2_BK2CMP1E         TIM1_AF2_BK2CMP1E_Msk                        /*!<BRK2 COMP1 enable */
#define TIM1_AF2_BK2CMP2E_Pos     (2U)
#define TIM1_AF2_BK2CMP2E_Msk     (0x1UL << TIM1_AF2_BK2CMP2E_Pos)             /*!< 0x00000004 */
#define TIM1_AF2_BK2CMP2E         TIM1_AF2_BK2CMP2E_Msk                        /*!<BRK2 COMP2 enable */
#define TIM1_AF2_BK2INP_Pos       (9U)
#define TIM1_AF2_BK2INP_Msk       (0x1UL << TIM1_AF2_BK2INP_Pos)               /*!< 0x00000200 */
#define TIM1_AF2_BK2INP           TIM1_AF2_BK2INP_Msk                          /*!<BRK2 BKIN2 input polarity */
#define TIM1_AF2_BK2CMP1P_Pos     (10U)
#define TIM1_AF2_BK2CMP1P_Msk     (0x1UL << TIM1_AF2_BK2CMP1P_Pos)             /*!< 0x00000400 */
#define TIM1_AF2_BK2CMP1P         TIM1_AF2_BK2CMP1P_Msk                        /*!<BRK2 COMP1 input polarity */
#define TIM1_AF2_BK2CMP2P_Pos     (11U)
#define TIM1_AF2_BK2CMP2P_Msk     (0x1UL << TIM1_AF2_BK2CMP2P_Pos)             /*!< 0x00000800 */
#define TIM1_AF2_BK2CMP2P         TIM1_AF2_BK2CMP2P_Msk                        /*!<BRK2 COMP2 input polarity */

/*******************  Bit definition for TIM2_OR1 register  *******************/
#define TIM2_OR1_OCREF_CLR_Pos     (0U)
#define TIM2_OR1_OCREF_CLR_Msk     (0x1UL << TIM2_OR1_OCREF_CLR_Pos)           /*!< 0x00000001 */
#define TIM2_OR1_OCREF_CLR         TIM2_OR1_OCREF_CLR_Msk                      /*!<OCREF clear input selection */

/*******************  Bit definition for TIM2_AF1 register  *******************/
#define TIM2_AF1_ETRSEL_Pos       (14U)
#define TIM2_AF1_ETRSEL_Msk       (0xFUL << TIM2_AF1_ETRSEL_Pos)               /*!< 0x0003C000 */
#define TIM2_AF1_ETRSEL           TIM2_AF1_ETRSEL_Msk                          /*!<ETRSEL[3:0] bits (TIM2 ETR source selection) */
#define TIM2_AF1_ETRSEL_0         (0x1UL << TIM2_AF1_ETRSEL_Pos)               /*!< 0x00004000 */
#define TIM2_AF1_ETRSEL_1         (0x2UL << TIM2_AF1_ETRSEL_Pos)               /*!< 0x00008000 */
#define TIM2_AF1_ETRSEL_2         (0x4UL << TIM2_AF1_ETRSEL_Pos)               /*!< 0x00010000 */
#define TIM2_AF1_ETRSEL_3         (0x8UL << TIM2_AF1_ETRSEL_Pos)               /*!< 0x00020000 */

/*******************  Bit definition for TIM3_OR1 register  *******************/
#define TIM3_OR1_OCREF_CLR_Pos     (0U)
#define TIM3_OR1_OCREF_CLR_Msk     (0x1UL << TIM3_OR1_OCREF_CLR_Pos)           /*!< 0x00000001 */
#define TIM3_OR1_OCREF_CLR         TIM3_OR1_OCREF_CLR_Msk                      /*!<OCREF clear input selection */

/*******************  Bit definition for TIM3_AF1 register  *******************/
#define TIM3_AF1_ETRSEL_Pos       (14U)
#define TIM3_AF1_ETRSEL_Msk       (0xFUL << TIM3_AF1_ETRSEL_Pos)               /*!< 0x0003C000 */
#define TIM3_AF1_ETRSEL           TIM3_AF1_ETRSEL_Msk                          /*!<ETRSEL[3:0] bits (TIM3 ETR source selection) */
#define TIM3_AF1_ETRSEL_0         (0x1UL << TIM3_AF1_ETRSEL_Pos)               /*!< 0x00004000 */
#define TIM3_AF1_ETRSEL_1         (0x2UL << TIM3_AF1_ETRSEL_Pos)               /*!< 0x00008000 */
#define TIM3_AF1_ETRSEL_2         (0x4UL << TIM3_AF1_ETRSEL_Pos)               /*!< 0x00010000 */
#define TIM3_AF1_ETRSEL_3         (0x8UL << TIM3_AF1_ETRSEL_Pos)               /*!< 0x00020000 */

/*******************  Bit definition for TIM14_AF1 register  *******************/
#define TIM14_AF1_ETRSEL_Pos      (14U)
#define TIM14_AF1_ETRSEL_Msk      (0xFUL << TIM14_AF1_ETRSEL_Pos)              /*!< 0x0003C000 */
#define TIM14_AF1_ETRSEL          TIM14_AF1_ETRSEL_Msk                         /*!<ETRSEL[3:0] bits (TIM3 ETR source selection) */
#define TIM14_AF1_ETRSEL_0        (0x1UL << TIM14_AF1_ETRSEL_Pos)              /*!< 0x00004000 */
#define TIM14_AF1_ETRSEL_1        (0x2UL << TIM14_AF1_ETRSEL_Pos)              /*!< 0x00008000 */
#define TIM14_AF1_ETRSEL_2        (0x4UL << TIM14_AF1_ETRSEL_Pos)              /*!< 0x00010000 */
#define TIM14_AF1_ETRSEL_3        (0x8UL << TIM14_AF1_ETRSEL_Pos)              /*!< 0x00020000 */

/*******************  Bit definition for TIM15_AF1 register  ******************/
#define TIM15_AF1_BKINE_Pos      (0U)
#define TIM15_AF1_BKINE_Msk      (0x1UL << TIM15_AF1_BKINE_Pos)                /*!< 0x00000001 */
#define TIM15_AF1_BKINE          TIM15_AF1_BKINE_Msk                           /*!<BRK BKIN input enable */
#define TIM15_AF1_BKCMP1E_Pos    (1U)
#define TIM15_AF1_BKCMP1E_Msk    (0x1UL << TIM15_AF1_BKCMP1E_Pos)              /*!< 0x00000002 */
#define TIM15_AF1_BKCMP1E        TIM15_AF1_BKCMP1E_Msk                         /*!<BRK COMP1 enable */
#define TIM15_AF1_BKCMP2E_Pos    (2U)
#define TIM15_AF1_BKCMP2E_Msk    (0x1UL << TIM15_AF1_BKCMP2E_Pos)              /*!< 0x00000004 */
#define TIM15_AF1_BKCMP2E        TIM15_AF1_BKCMP2E_Msk                         /*!<BRK COMP2 enable */
#define TIM15_AF1_BKINP_Pos      (9U)
#define TIM15_AF1_BKINP_Msk      (0x1UL << TIM15_AF1_BKINP_Pos)                /*!< 0x00000200 */
#define TIM15_AF1_BKINP          TIM15_AF1_BKINP_Msk                           /*!<BRK BKIN input polarity */
#define TIM15_AF1_BKCMP1P_Pos    (10U)
#define TIM15_AF1_BKCMP1P_Msk    (0x1UL << TIM15_AF1_BKCMP1P_Pos)              /*!< 0x00000400 */
#define TIM15_AF1_BKCMP1P        TIM15_AF1_BKCMP1P_Msk                         /*!<BRK COMP1 input polarity */
#define TIM15_AF1_BKCMP2P_Pos    (11U)
#define TIM15_AF1_BKCMP2P_Msk    (0x1UL << TIM15_AF1_BKCMP2P_Pos)              /*!< 0x00000800 */
#define TIM15_AF1_BKCMP2P        TIM15_AF1_BKCMP2P_Msk                         /*!<BRK COMP2 input polarity */

/*******************  Bit definition for TIM16_AF1 register  ******************/
#define TIM16_AF1_BKINE_Pos      (0U)
#define TIM16_AF1_BKINE_Msk      (0x1UL << TIM16_AF1_BKINE_Pos)                /*!< 0x00000001 */
#define TIM16_AF1_BKINE          TIM16_AF1_BKINE_Msk                           /*!<BRK BKIN input enable */
#define TIM16_AF1_BKCMP1E_Pos    (1U)
#define TIM16_AF1_BKCMP1E_Msk    (0x1UL << TIM16_AF1_BKCMP1E_Pos)              /*!< 0x00000002 */
#define TIM16_AF1_BKCMP1E        TIM16_AF1_BKCMP1E_Msk                         /*!<BRK COMP1 enable */
#define TIM16_AF1_BKCMP2E_Pos    (2U)
#define TIM16_AF1_BKCMP2E_Msk    (0x1UL << TIM16_AF1_BKCMP2E_Pos)              /*!< 0x00000004 */
#define TIM16_AF1_BKCMP2E        TIM16_AF1_BKCMP2E_Msk                         /*!<BRK COMP2 enable */
#define TIM16_AF1_BKINP_Pos      (9U)
#define TIM16_AF1_BKINP_Msk      (0x1UL << TIM16_AF1_BKINP_Pos)                /*!< 0x00000200 */
#define TIM16_AF1_BKINP          TIM16_AF1_BKINP_Msk                           /*!<BRK BKIN input polarity */
#define TIM16_AF1_BKCMP1P_Pos    (10U)
#define TIM16_AF1_BKCMP1P_Msk    (0x1UL << TIM16_AF1_BKCMP1P_Pos)              /*!< 0x00000400 */
#define TIM16_AF1_BKCMP1P        TIM16_AF1_BKCMP1P_Msk                         /*!<BRK COMP1 input polarity */
#define TIM16_AF1_BKCMP2P_Pos    (11U)
#define TIM16_AF1_BKCMP2P_Msk    (0x1UL << TIM16_AF1_BKCMP2P_Pos)              /*!< 0x00000800 */
#define TIM16_AF1_BKCMP2P        TIM16_AF1_BKCMP2P_Msk                         /*!<BRK COMP2 input polarity */

/*******************  Bit definition for TIM17_AF1 register  ******************/
#define TIM17_AF1_BKINE_Pos      (0U)
#define TIM17_AF1_BKINE_Msk      (0x1UL << TIM17_AF1_BKINE_Pos)                /*!< 0x00000001 */
#define TIM17_AF1_BKINE          TIM17_AF1_BKINE_Msk                           /*!<BRK BKIN input enable */
#define TIM17_AF1_BKCMP1E_Pos    (1U)
#define TIM17_AF1_BKCMP1E_Msk    (0x1UL << TIM17_AF1_BKCMP1E_Pos)              /*!< 0x00000002 */
#define TIM17_AF1_BKCMP1E        TIM17_AF1_BKCMP1E_Msk                         /*!<BRK COMP1 enable */
#define TIM17_AF1_BKCMP2E_Pos    (2U)
#define TIM17_AF1_BKCMP2E_Msk    (0x1UL << TIM17_AF1_BKCMP2E_Pos)              /*!< 0x00000004 */
#define TIM17_AF1_BKCMP2E        TIM17_AF1_BKCMP2E_Msk                         /*!<BRK COMP2 enable */
#define TIM17_AF1_BKINP_Pos      (9U)
#define TIM17_AF1_BKINP_Msk      (0x1UL << TIM17_AF1_BKINP_Pos)                /*!< 0x00000200 */
#define TIM17_AF1_BKINP          TIM17_AF1_BKINP_Msk                           /*!<BRK BKIN input polarity */
#define TIM17_AF1_BKCMP1P_Pos    (10U)
#define TIM17_AF1_BKCMP1P_Msk    (0x1UL << TIM17_AF1_BKCMP1P_Pos)              /*!< 0x00000400 */
#define TIM17_AF1_BKCMP1P        TIM17_AF1_BKCMP1P_Msk                         /*!<BRK COMP1 input polarity */
#define TIM17_AF1_BKCMP2P_Pos    (11U)
#define TIM17_AF1_BKCMP2P_Msk    (0x1UL << TIM17_AF1_BKCMP2P_Pos)              /*!< 0x00000800 */
#define TIM17_AF1_BKCMP2P        TIM17_AF1_BKCMP2P_Msk                         /*!<BRK COMP2 input polarity */

/*******************  Bit definition for TIM_TISEL register  *********************/
#define TIM_TISEL_TI1SEL_Pos      (0U)
#define TIM_TISEL_TI1SEL_Msk      (0xFUL << TIM_TISEL_TI1SEL_Pos)              /*!< 0x0000000F */
#define TIM_TISEL_TI1SEL          TIM_TISEL_TI1SEL_Msk                         /*!<TI1SEL[3:0] bits (TIM1 TI1 SEL)*/
#define TIM_TISEL_TI1SEL_0        (0x1UL << TIM_TISEL_TI1SEL_Pos)              /*!< 0x00000001 */
#define TIM_TISEL_TI1SEL_1        (0x2UL << TIM_TISEL_TI1SEL_Pos)              /*!< 0x00000002 */
#define TIM_TISEL_TI1SEL_2        (0x4UL << TIM_TISEL_TI1SEL_Pos)              /*!< 0x00000004 */
#define TIM_TISEL_TI1SEL_3        (0x8UL << TIM_TISEL_TI1SEL_Pos)              /*!< 0x00000008 */

#define TIM_TISEL_TI2SEL_Pos      (8U)
#define TIM_TISEL_TI2SEL_Msk      (0xFUL << TIM_TISEL_TI2SEL_Pos)              /*!< 0x00000F00 */
#define TIM_TISEL_TI2SEL          TIM_TISEL_TI2SEL_Msk                         /*!<TI2SEL[3:0] bits (TIM1 TI2 SEL)*/
#define TIM_TISEL_TI2SEL_0        (0x1UL << TIM_TISEL_TI2SEL_Pos)              /*!< 0x00000100 */
#define TIM_TISEL_TI2SEL_1        (0x2UL << TIM_TISEL_TI2SEL_Pos)              /*!< 0x00000200 */
#define TIM_TISEL_TI2SEL_2        (0x4UL << TIM_TISEL_TI2SEL_Pos)              /*!< 0x00000400 */
#define TIM_TISEL_TI2SEL_3        (0x8UL << TIM_TISEL_TI2SEL_Pos)              /*!< 0x00000800 */

#define TIM_TISEL_TI3SEL_Pos      (16U)
#define TIM_TISEL_TI3SEL_Msk      (0xFUL << TIM_TISEL_TI3SEL_Pos)              /*!< 0x000F0000 */
#define TIM_TISEL_TI3SEL          TIM_TISEL_TI3SEL_Msk                         /*!<TI3SEL[3:0] bits (TIM1 TI3 SEL)*/
#define TIM_TISEL_TI3SEL_0        (0x1UL << TIM_TISEL_TI3SEL_Pos)              /*!< 0x00010000 */
#define TIM_TISEL_TI3SEL_1        (0x2UL << TIM_TISEL_TI3SEL_Pos)              /*!< 0x00020000 */
#define TIM_TISEL_TI3SEL_2        (0x4UL << TIM_TISEL_TI3SEL_Pos)              /*!< 0x00040000 */
#define TIM_TISEL_TI3SEL_3        (0x8UL << TIM_TISEL_TI3SEL_Pos)              /*!< 0x00080000 */

#define TIM_TISEL_TI4SEL_Pos      (24U)
#define TIM_TISEL_TI4SEL_Msk      (0xFUL << TIM_TISEL_TI4SEL_Pos)              /*!< 0x0F000000 */
#define TIM_TISEL_TI4SEL          TIM_TISEL_TI4SEL_Msk                         /*!<TI4SEL[3:0] bits (TIM1 TI4 SEL)*/
#define TIM_TISEL_TI4SEL_0        (0x1UL << TIM_TISEL_TI4SEL_Pos)              /*!< 0x01000000 */
#define TIM_TISEL_TI4SEL_1        (0x2UL << TIM_TISEL_TI4SEL_Pos)              /*!< 0x02000000 */
#define TIM_TISEL_TI4SEL_2        (0x4UL << TIM_TISEL_TI4SEL_Pos)              /*!< 0x04000000 */
#define TIM_TISEL_TI4SEL_3        (0x8UL << TIM_TISEL_TI4SEL_Pos)              /*!< 0x08000000 */

/******************************************************************************/
/*                                                                            */
/*                         Low Power Timer (LPTIM)                            */
/*                                                                            */
/******************************************************************************/
/******************  Bit definition for LPTIM_ISR register  *******************/
#define LPTIM_ISR_CMPM_Pos          (0U)
#define LPTIM_ISR_CMPM_Msk          (0x1UL << LPTIM_ISR_CMPM_Pos)              /*!< 0x00000001 */
#define LPTIM_ISR_CMPM              LPTIM_ISR_CMPM_Msk                         /*!< Compare match */
#define LPTIM_ISR_ARRM_Pos          (1U)
#define LPTIM_ISR_ARRM_Msk          (0x1UL << LPTIM_ISR_ARRM_Pos)              /*!< 0x00000002 */
#define LPTIM_ISR_ARRM              LPTIM_ISR_ARRM_Msk                         /*!< Autoreload match */
#define LPTIM_ISR_EXTTRIG_Pos       (2U)
#define LPTIM_ISR_EXTTRIG_Msk       (0x1UL << LPTIM_ISR_EXTTRIG_Pos)           /*!< 0x00000004 */
#define LPTIM_ISR_EXTTRIG           LPTIM_ISR_EXTTRIG_Msk                      /*!< External trigger edge event */
#define LPTIM_ISR_CMPOK_Pos         (3U)
#define LPTIM_ISR_CMPOK_Msk         (0x1UL << LPTIM_ISR_CMPOK_Pos)             /*!< 0x00000008 */
#define LPTIM_ISR_CMPOK             LPTIM_ISR_CMPOK_Msk                        /*!< Compare register update OK */
#define LPTIM_ISR_ARROK_Pos         (4U)
#define LPTIM_ISR_ARROK_Msk         (0x1UL << LPTIM_ISR_ARROK_Pos)             /*!< 0x00000010 */
#define LPTIM_ISR_ARROK             LPTIM_ISR_ARROK_Msk                        /*!< Autoreload register update OK */
#define LPTIM_ISR_UP_Pos            (5U)
#define LPTIM_ISR_UP_Msk            (0x1UL << LPTIM_ISR_UP_Pos)                /*!< 0x00000020 */
#define LPTIM_ISR_UP                LPTIM_ISR_UP_Msk                           /*!< Counter direction change down to up */
#define LPTIM_ISR_DOWN_Pos          (6U)
#define LPTIM_ISR_DOWN_Msk          (0x1UL << LPTIM_ISR_DOWN_Pos)              /*!< 0x00000040 */
#define LPTIM_ISR_DOWN              LPTIM_ISR_DOWN_Msk                         /*!< Counter direction change up to down */

/******************  Bit definition for LPTIM_ICR register  *******************/
#define LPTIM_ICR_CMPMCF_Pos        (0U)
#define LPTIM_ICR_CMPMCF_Msk        (0x1UL << LPTIM_ICR_CMPMCF_Pos)            /*!< 0x00000001 */
#define LPTIM_ICR_CMPMCF            LPTIM_ICR_CMPMCF_Msk                       /*!< Compare match Clear Flag */
#define LPTIM_ICR_ARRMCF_Pos        (1U)
#define LPTIM_ICR_ARRMCF_Msk        (0x1UL << LPTIM_ICR_ARRMCF_Pos)            /*!< 0x00000002 */
#define LPTIM_ICR_ARRMCF            LPTIM_ICR_ARRMCF_Msk                       /*!< Autoreload match Clear Flag */
#define LPTIM_ICR_EXTTRIGCF_Pos     (2U)
#define LPTIM_ICR_EXTTRIGCF_Msk     (0x1UL << LPTIM_ICR_EXTTRIGCF_Pos)         /*!< 0x00000004 */
#define LPTIM_ICR_EXTTRIGCF         LPTIM_ICR_EXTTRIGCF_Msk                    /*!< External trigger edge event Clear Flag */
#define LPTIM_ICR_CMPOKCF_Pos       (3U)
#define LPTIM_ICR_CMPOKCF_Msk       (0x1UL << LPTIM_ICR_CMPOKCF_Pos)           /*!< 0x00000008 */
#define LPTIM_ICR_CMPOKCF           LPTIM_ICR_CMPOKCF_Msk                      /*!< Compare register update OK Clear Flag */
#define LPTIM_ICR_ARROKCF_Pos       (4U)
#define LPTIM_ICR_ARROKCF_Msk       (0x1UL << LPTIM_ICR_ARROKCF_Pos)           /*!< 0x00000010 */
#define LPTIM_ICR_ARROKCF           LPTIM_ICR_ARROKCF_Msk                      /*!< Autoreload register update OK Clear Flag */
#define LPTIM_ICR_UPCF_Pos          (5U)
#define LPTIM_ICR_UPCF_Msk          (0x1UL << LPTIM_ICR_UPCF_Pos)              /*!< 0x00000020 */
#define LPTIM_ICR_UPCF              LPTIM_ICR_UPCF_Msk                         /*!< Counter direction change down to up Clear Flag */
#define LPTIM_ICR_DOWNCF_Pos        (6U)
#define LPTIM_ICR_DOWNCF_Msk        (0x1UL << LPTIM_ICR_DOWNCF_Pos)            /*!< 0x00000040 */
#define LPTIM_ICR_DOWNCF            LPTIM_ICR_DOWNCF_Msk                       /*!< Counter direction change up to down Clear Flag */

/******************  Bit definition for LPTIM_IER register ********************/
#define LPTIM_IER_CMPMIE_Pos        (0U)
#define LPTIM_IER_CMPMIE_Msk        (0x1UL << LPTIM_IER_CMPMIE_Pos)            /*!< 0x00000001 */
#define LPTIM_IER_CMPMIE            LPTIM_IER_CMPMIE_Msk                       /*!< Compare match Interrupt Enable */
#define LPTIM_IER_ARRMIE_Pos        (1U)
#define LPTIM_IER_ARRMIE_Msk        (0x1UL << LPTIM_IER_ARRMIE_Pos)            /*!< 0x00000002 */
#define LPTIM_IER_ARRMIE            LPTIM_IER_ARRMIE_Msk                       /*!< Autoreload match Interrupt Enable */
#define LPTIM_IER_EXTTRIGIE_Pos     (2U)
#define LPTIM_IER_EXTTRIGIE_Msk     (0x1UL << LPTIM_IER_EXTTRIGIE_Pos)         /*!< 0x00000004 */
#define LPTIM_IER_EXTTRIGIE         LPTIM_IER_EXTTRIGIE_Msk                    /*!< External trigger edge event Interrupt Enable */
#define LPTIM_IER_CMPOKIE_Pos       (3U)
#define LPTIM_IER_CMPOKIE_Msk       (0x1UL << LPTIM_IER_CMPOKIE_Pos)           /*!< 0x00000008 */
#define LPTIM_IER_CMPOKIE           LPTIM_IER_CMPOKIE_Msk                      /*!< Compare register update OK Interrupt Enable */
#define LPTIM_IER_ARROKIE_Pos       (4U)
#define LPTIM_IER_ARROKIE_Msk       (0x1UL << LPTIM_IER_ARROKIE_Pos)           /*!< 0x00000010 */
#define LPTIM_IER_ARROKIE           LPTIM_IER_ARROKIE_Msk                      /*!< Autoreload register update OK Interrupt Enable */
#define LPTIM_IER_UPIE_Pos          (5U)
#define LPTIM_IER_UPIE_Msk          (0x1UL << LPTIM_IER_UPIE_Pos)              /*!< 0x00000020 */
#define LPTIM_IER_UPIE              LPTIM_IER_UPIE_Msk                         /*!< Counter direction change down to up Interrupt Enable */
#define LPTIM_IER_DOWNIE_Pos        (6U)
#define LPTIM_IER_DOWNIE_Msk        (0x1UL << LPTIM_IER_DOWNIE_Pos)            /*!< 0x00000040 */
#define LPTIM_IER_DOWNIE            LPTIM_IER_DOWNIE_Msk                       /*!< Counter direction change up to down Interrupt Enable */

/******************  Bit definition for LPTIM_CFGR register *******************/
#define LPTIM_CFGR_CKSEL_Pos        (0U)
#define LPTIM_CFGR_CKSEL_Msk        (0x1UL << LPTIM_CFGR_CKSEL_Pos)            /*!< 0x00000001 */
#define LPTIM_CFGR_CKSEL            LPTIM_CFGR_CKSEL_Msk                       /*!< Clock selector */

#define LPTIM_CFGR_CKPOL_Pos        (1U)
#define LPTIM_CFGR_CKPOL_Msk        (0x3UL << LPTIM_CFGR_CKPOL_Pos)            /*!< 0x00000006 */
#define LPTIM_CFGR_CKPOL            LPTIM_CFGR_CKPOL_Msk                       /*!< CKPOL[1:0] bits (Clock polarity) */
#define LPTIM_CFGR_CKPOL_0          (0x1UL << LPTIM_CFGR_CKPOL_Pos)            /*!< 0x00000002 */
#define LPTIM_CFGR_CKPOL_1          (0x2UL << LPTIM_CFGR_CKPOL_Pos)            /*!< 0x00000004 */

#define LPTIM_CFGR_CKFLT_Pos        (3U)
#define LPTIM_CFGR_CKFLT_Msk        (0x3UL << LPTIM_CFGR_CKFLT_Pos)            /*!< 0x00000018 */
#define LPTIM_CFGR_CKFLT            LPTIM_CFGR_CKFLT_Msk                       /*!< CKFLT[1:0] bits (Configurable digital filter for external clock) */
#define LPTIM_CFGR_CKFLT_0          (0x1UL << LPTIM_CFGR_CKFLT_Pos)            /*!< 0x00000008 */
#define LPTIM_CFGR_CKFLT_1          (0x2UL << LPTIM_CFGR_CKFLT_Pos)            /*!< 0x00000010 */

#define LPTIM_CFGR_TRGFLT_Pos       (6U)
#define LPTIM_CFGR_TRGFLT_Msk       (0x3UL << LPTIM_CFGR_TRGFLT_Pos)           /*!< 0x000000C0 */
#define LPTIM_CFGR_TRGFLT           LPTIM_CFGR_TRGFLT_Msk                      /*!< TRGFLT[1:0] bits (Configurable digital filter for trigger) */
#define LPTIM_CFGR_TRGFLT_0         (0x1UL << LPTIM_CFGR_TRGFLT_Pos)           /*!< 0x00000040 */
#define LPTIM_CFGR_TRGFLT_1         (0x2UL << LPTIM_CFGR_TRGFLT_Pos)           /*!< 0x00000080 */

#define LPTIM_CFGR_PRESC_Pos        (9U)
#define LPTIM_CFGR_PRESC_Msk        (0x7UL << LPTIM_CFGR_PRESC_Pos)            /*!< 0x00000E00 */
#define LPTIM_CFGR_PRESC            LPTIM_CFGR_PRESC_Msk                       /*!< PRESC[2:0] bits (Clock prescaler) */
#define LPTIM_CFGR_PRESC_0          (0x1UL << LPTIM_CFGR_PRESC_Pos)            /*!< 0x00000200 */
#define LPTIM_CFGR_PRESC_1          (0x2UL << LPTIM_CFGR_PRESC_Pos)            /*!< 0x00000400 */
#define LPTIM_CFGR_PRESC_2          (0x4UL << LPTIM_CFGR_PRESC_Pos)            /*!< 0x00000800 */

#define LPTIM_CFGR_TRIGSEL_Pos      (13U)
#define LPTIM_CFGR_TRIGSEL_Msk      (0x7UL << LPTIM_CFGR_TRIGSEL_Pos)          /*!< 0x0000E000 */
#define LPTIM_CFGR_TRIGSEL          LPTIM_CFGR_TRIGSEL_Msk                     /*!< TRIGSEL[2:0]] bits (Trigger selector) */
#define LPTIM_CFGR_TRIGSEL_0        (0x1UL << LPTIM_CFGR_TRIGSEL_Pos)          /*!< 0x00002000 */
#define LPTIM_CFGR_TRIGSEL_1        (0x2UL << LPTIM_CFGR_TRIGSEL_Pos)          /*!< 0x00004000 */
#define LPTIM_CFGR_TRIGSEL_2        (0x4UL << LPTIM_CFGR_TRIGSEL_Pos)          /*!< 0x00008000 */

#define LPTIM_CFGR_TRIGEN_Pos       (17U)
#define LPTIM_CFGR_TRIGEN_Msk       (0x3UL << LPTIM_CFGR_TRIGEN_Pos)           /*!< 0x00060000 */
#define LPTIM_CFGR_TRIGEN           LPTIM_CFGR_TRIGEN_Msk                      /*!< TRIGEN[1:0] bits (Trigger enable and polarity) */
#define LPTIM_CFGR_TRIGEN_0         (0x1UL << LPTIM_CFGR_TRIGEN_Pos)           /*!< 0x00020000 */
#define LPTIM_CFGR_TRIGEN_1         (0x2UL << LPTIM_CFGR_TRIGEN_Pos)           /*!< 0x00040000 */

#define LPTIM_CFGR_TIMOUT_Pos       (19U)
#define LPTIM_CFGR_TIMOUT_Msk       (0x1UL << LPTIM_CFGR_TIMOUT_Pos)           /*!< 0x00080000 */
#define LPTIM_CFGR_TIMOUT           LPTIM_CFGR_TIMOUT_Msk                      /*!< Timout enable */
#define LPTIM_CFGR_WAVE_Pos         (20U)
#define LPTIM_CFGR_WAVE_Msk         (0x1UL << LPTIM_CFGR_WAVE_Pos)             /*!< 0x00100000 */
#define LPTIM_CFGR_WAVE             LPTIM_CFGR_WAVE_Msk                        /*!< Waveform shape */
#define LPTIM_CFGR_WAVPOL_Pos       (21U)
#define LPTIM_CFGR_WAVPOL_Msk       (0x1UL << LPTIM_CFGR_WAVPOL_Pos)           /*!< 0x00200000 */
#define LPTIM_CFGR_WAVPOL           LPTIM_CFGR_WAVPOL_Msk                      /*!< Waveform shape polarity */
#define LPTIM_CFGR_PRELOAD_Pos      (22U)
#define LPTIM_CFGR_PRELOAD_Msk      (0x1UL << LPTIM_CFGR_PRELOAD_Pos)          /*!< 0x00400000 */
#define LPTIM_CFGR_PRELOAD          LPTIM_CFGR_PRELOAD_Msk                     /*!< Reg update mode */
#define LPTIM_CFGR_COUNTMODE_Pos    (23U)
#define LPTIM_CFGR_COUNTMODE_Msk    (0x1UL << LPTIM_CFGR_COUNTMODE_Pos)        /*!< 0x00800000 */
#define LPTIM_CFGR_COUNTMODE        LPTIM_CFGR_COUNTMODE_Msk                   /*!< Counter mode enable */
#define LPTIM_CFGR_ENC_Pos          (24U)
#define LPTIM_CFGR_ENC_Msk          (0x1UL << LPTIM_CFGR_ENC_Pos)              /*!< 0x01000000 */
#define LPTIM_CFGR_ENC              LPTIM_CFGR_ENC_Msk                         /*!< Encoder mode enable */

/******************  Bit definition for LPTIM_CR register  ********************/
#define LPTIM_CR_ENABLE_Pos         (0U)
#define LPTIM_CR_ENABLE_Msk         (0x1UL << LPTIM_CR_ENABLE_Pos)             /*!< 0x00000001 */
#define LPTIM_CR_ENABLE             LPTIM_CR_ENABLE_Msk                        /*!< LPTIMer enable */
#define LPTIM_CR_SNGSTRT_Pos        (1U)
#define LPTIM_CR_SNGSTRT_Msk        (0x1UL << LPTIM_CR_SNGSTRT_Pos)            /*!< 0x00000002 */
#define LPTIM_CR_SNGSTRT            LPTIM_CR_SNGSTRT_Msk                       /*!< Timer start in single mode */
#define LPTIM_CR_CNTSTRT_Pos        (2U)
#define LPTIM_CR_CNTSTRT_Msk        (0x1UL << LPTIM_CR_CNTSTRT_Pos)            /*!< 0x00000004 */
#define LPTIM_CR_CNTSTRT            LPTIM_CR_CNTSTRT_Msk                       /*!< Timer start in continuous mode */
#define LPTIM_CR_COUNTRST_Pos       (3U)
#define LPTIM_CR_COUNTRST_Msk       (0x1UL << LPTIM_CR_COUNTRST_Pos)           /*!< 0x00000008 */
#define LPTIM_CR_COUNTRST           LPTIM_CR_COUNTRST_Msk                      /*!< Counter reset */
#define LPTIM_CR_RSTARE_Pos         (4U)
#define LPTIM_CR_RSTARE_Msk         (0x1UL << LPTIM_CR_RSTARE_Pos)             /*!< 0x00000010 */
#define LPTIM_CR_RSTARE             LPTIM_CR_RSTARE_Msk                        /*!< Reset after read enable */

/******************  Bit definition for LPTIM_CMP register  *******************/
#define LPTIM_CMP_CMP_Pos           (0U)
#define LPTIM_CMP_CMP_Msk           (0xFFFFUL << LPTIM_CMP_CMP_Pos)            /*!< 0x0000FFFF */
#define LPTIM_CMP_CMP               LPTIM_CMP_CMP_Msk                          /*!< Compare register */

/******************  Bit definition for LPTIM_ARR register  *******************/
#define LPTIM_ARR_ARR_Pos           (0U)
#define LPTIM_ARR_ARR_Msk           (0xFFFFUL << LPTIM_ARR_ARR_Pos)            /*!< 0x0000FFFF */
#define LPTIM_ARR_ARR               LPTIM_ARR_ARR_Msk                          /*!< Auto reload register */

/******************  Bit definition for LPTIM_CNT register  *******************/
#define LPTIM_CNT_CNT_Pos           (0U)
#define LPTIM_CNT_CNT_Msk           (0xFFFFUL << LPTIM_CNT_CNT_Pos)            /*!< 0x0000FFFF */
#define LPTIM_CNT_CNT               LPTIM_CNT_CNT_Msk                          /*!< Counter register */

/******************  Bit definition for LPTIM_CFGR2 register  *******************/
#define LPTIM_CFGR2_IN1SEL_Pos      (0U)
#define LPTIM_CFGR2_IN1SEL_Msk      (0xFUL << LPTIM_CFGR2_IN1SEL_Pos)          /*!< 0x0000000F */
#define LPTIM_CFGR2_IN1SEL          LPTIM_CFGR2_IN1SEL_Msk                     /*!< CFGR2[3:0] bits (INPUT1 selection) */
#define LPTIM_CFGR2_IN1SEL_0        (0x1UL << LPTIM_CFGR2_IN1SEL_Pos)          /*!< 0x00000001 */
#define LPTIM_CFGR2_IN1SEL_1        (0x2UL << LPTIM_CFGR2_IN1SEL_Pos)          /*!< 0x00000002 */
#define LPTIM_CFGR2_IN1SEL_2        (0x4UL << LPTIM_CFGR2_IN1SEL_Pos)          /*!< 0x00000004 */
#define LPTIM_CFGR2_IN1SEL_3        (0x8UL << LPTIM_CFGR2_IN1SEL_Pos)          /*!< 0x00000008 */

#define LPTIM_CFGR2_IN2SEL_Pos      (4U)
#define LPTIM_CFGR2_IN2SEL_Msk      (0xFUL << LPTIM_CFGR2_IN2SEL_Pos)          /*!< 0x000000F0 */
#define LPTIM_CFGR2_IN2SEL          LPTIM_CFGR2_IN2SEL_Msk                     /*!< CFGR2[7:4] bits (INPUT2 selection) */
#define LPTIM_CFGR2_IN2SEL_0        (0x1UL << LPTIM_CFGR2_IN2SEL_Pos)          /*!< 0x00000010 */
#define LPTIM_CFGR2_IN2SEL_1        (0x2UL << LPTIM_CFGR2_IN2SEL_Pos)          /*!< 0x00000020 */
#define LPTIM_CFGR2_IN2SEL_2        (0x4UL << LPTIM_CFGR2_IN2SEL_Pos)          /*!< 0x00000040 */
#define LPTIM_CFGR2_IN2SEL_3        (0x8UL << LPTIM_CFGR2_IN2SEL_Pos)          /*!< 0x00000080 */

/******************************************************************************/
/*                                                                            */
/*                      Analog Comparators (COMP)                             */
/*                                                                            */
/******************************************************************************/
/**********************  Bit definition for COMP_CSR register  ****************/
#define COMP_CSR_EN_Pos            (0U)
#define COMP_CSR_EN_Msk            (0x1UL << COMP_CSR_EN_Pos)                  /*!< 0x00000001 */
#define COMP_CSR_EN                COMP_CSR_EN_Msk                             /*!< Comparator enable */

#define COMP_CSR_INMSEL_Pos        (4U)
#define COMP_CSR_INMSEL_Msk        (0xFUL << COMP_CSR_INMSEL_Pos)              /*!< 0x000000F0 */
#define COMP_CSR_INMSEL            COMP_CSR_INMSEL_Msk                         /*!< Comparator input minus selection */
#define COMP_CSR_INMSEL_0          (0x1UL << COMP_CSR_INMSEL_Pos)              /*!< 0x00000010 */
#define COMP_CSR_INMSEL_1          (0x2UL << COMP_CSR_INMSEL_Pos)              /*!< 0x00000020 */
#define COMP_CSR_INMSEL_2          (0x4UL << COMP_CSR_INMSEL_Pos)              /*!< 0x00000040 */
#define COMP_CSR_INMSEL_3          (0x8UL << COMP_CSR_INMSEL_Pos)              /*!< 0x00000080 */

#define COMP_CSR_INPSEL_Pos        (8U)
#define COMP_CSR_INPSEL_Msk        (0x3UL << COMP_CSR_INPSEL_Pos)              /*!< 0x00000300 */
#define COMP_CSR_INPSEL            COMP_CSR_INPSEL_Msk                         /*!< Comparator plus minus selection */
#define COMP_CSR_INPSEL_0          (0x1UL << COMP_CSR_INPSEL_Pos)              /*!< 0x00000100 */
#define COMP_CSR_INPSEL_1          (0x2UL << COMP_CSR_INPSEL_Pos)              /*!< 0x00000200 */

#define COMP_CSR_WINMODE_Pos       (11U)
#define COMP_CSR_WINMODE_Msk       (0x1UL << COMP_CSR_WINMODE_Pos)             /*!< 0x00000800 */
#define COMP_CSR_WINMODE           COMP_CSR_WINMODE_Msk                        /*!< Pair of comparators window mode. Bit intended to be used with COMP common instance (COMP_Common_TypeDef) */
#define COMP_CSR_WINOUT_Pos        (14U)
#define COMP_CSR_WINOUT_Msk        (0x1UL << COMP_CSR_WINOUT_Pos)              /*!< 0x00004000 */
#define COMP_CSR_WINOUT            COMP_CSR_WINOUT_Msk                         /*!< Pair of comparators window output level. Bit intended to be used with COMP common instance (COMP_Common_TypeDef) */

#define COMP_CSR_POLARITY_Pos      (15U)
#define COMP_CSR_POLARITY_Msk      (0x1UL << COMP_CSR_POLARITY_Pos)            /*!< 0x00008000 */
#define COMP_CSR_POLARITY          COMP_CSR_POLARITY_Msk                       /*!< Comparator output polarity */

#define COMP_CSR_HYST_Pos          (16U)
#define COMP_CSR_HYST_Msk          (0x3UL << COMP_CSR_HYST_Pos)                /*!< 0x00030000 */
#define COMP_CSR_HYST              COMP_CSR_HYST_Msk                           /*!< Comparator input hysteresis */
#define COMP_CSR_HYST_0            (0x1UL << COMP_CSR_HYST_Pos)                /*!< 0x00010000 */
#define COMP_CSR_HYST_1            (0x2UL << COMP_CSR_HYST_Pos)                /*!< 0x00020000 */

#define COMP_CSR_PWRMODE_Pos       (18U)
#define COMP_CSR_PWRMODE_Msk       (0x3UL << COMP_CSR_PWRMODE_Pos)             /*!< 0x000C0000 */
#define COMP_CSR_PWRMODE           COMP_CSR_PWRMODE_Msk                        /*!< Comparator power mode */
#define COMP_CSR_PWRMODE_0         (0x1UL << COMP_CSR_PWRMODE_Pos)             /*!< 0x00040000 */
#define COMP_CSR_PWRMODE_1         (0x2UL << COMP_CSR_PWRMODE_Pos)             /*!< 0x00080000 */

#define COMP_CSR_BLANKING_Pos      (20U)
#define COMP_CSR_BLANKING_Msk      (0x1FUL << COMP_CSR_BLANKING_Pos)           /*!< 0x01F00000 */
#define COMP_CSR_BLANKING          COMP_CSR_BLANKING_Msk                       /*!< Comparator blanking source */
#define COMP_CSR_BLANKING_0        (0x01UL << COMP_CSR_BLANKING_Pos)           /*!< 0x00100000 */
#define COMP_CSR_BLANKING_1        (0x02UL << COMP_CSR_BLANKING_Pos)           /*!< 0x00200000 */
#define COMP_CSR_BLANKING_2        (0x04UL << COMP_CSR_BLANKING_Pos)           /*!< 0x00400000 */
#define COMP_CSR_BLANKING_3        (0x08UL << COMP_CSR_BLANKING_Pos)           /*!< 0x00800000 */
#define COMP_CSR_BLANKING_4        (0x10UL << COMP_CSR_BLANKING_Pos)           /*!< 0x01000000 */

#define COMP_CSR_VALUE_Pos         (30U)
#define COMP_CSR_VALUE_Msk         (0x1UL << COMP_CSR_VALUE_Pos)               /*!< 0x40000000 */
#define COMP_CSR_VALUE             COMP_CSR_VALUE_Msk                          /*!< Comparator output level */

#define COMP_CSR_LOCK_Pos          (31U)
#define COMP_CSR_LOCK_Msk          (0x1UL << COMP_CSR_LOCK_Pos)                /*!< 0x80000000 */
#define COMP_CSR_LOCK              COMP_CSR_LOCK_Msk                           /*!< Comparator lock */

/******************************************************************************/
/*                                                                            */
/*      Universal Synchronous Asynchronous Receiver Transmitter (USART)       */
/*                                                                            */
/******************************************************************************/
/******************  Bit definition for USART_CR1 register  *******************/
#define USART_CR1_UE_Pos             (0U)
#define USART_CR1_UE_Msk             (0x1UL << USART_CR1_UE_Pos)               /*!< 0x00000001 */
#define USART_CR1_UE                 USART_CR1_UE_Msk                          /*!< USART Enable */
#define USART_CR1_UESM_Pos           (1U)
#define USART_CR1_UESM_Msk           (0x1UL << USART_CR1_UESM_Pos)             /*!< 0x00000002 */
#define USART_CR1_UESM               USART_CR1_UESM_Msk                        /*!< USART Enable in STOP Mode */
#define USART_CR1_RE_Pos             (2U)
#define USART_CR1_RE_Msk             (0x1UL << USART_CR1_RE_Pos)               /*!< 0x00000004 */
#define USART_CR1_RE                 USART_CR1_RE_Msk                          /*!< Receiver Enable */
#define USART_CR1_TE_Pos             (3U)
#define USART_CR1_TE_Msk             (0x1UL << USART_CR1_TE_Pos)               /*!< 0x00000008 */
#define USART_CR1_TE                 USART_CR1_TE_Msk                          /*!< Transmitter Enable */
#define USART_CR1_IDLEIE_Pos         (4U)
#define USART_CR1_IDLEIE_Msk         (0x1UL << USART_CR1_IDLEIE_Pos)           /*!< 0x00000010 */
#define USART_CR1_IDLEIE             USART_CR1_IDLEIE_Msk                      /*!< IDLE Interrupt Enable */
#define USART_CR1_RXNEIE_RXFNEIE_Pos   (5U)
#define USART_CR1_RXNEIE_RXFNEIE_Msk   (0x1UL << USART_CR1_RXNEIE_RXFNEIE_Pos) /*!< 0x00000020 */
#define USART_CR1_RXNEIE_RXFNEIE       USART_CR1_RXNEIE_RXFNEIE_Msk            /*!< RXNE/RXFIFO not empty Interrupt Enable */
#define USART_CR1_TCIE_Pos           (6U)
#define USART_CR1_TCIE_Msk           (0x1UL << USART_CR1_TCIE_Pos)             /*!< 0x00000040 */
#define USART_CR1_TCIE               USART_CR1_TCIE_Msk                        /*!< Transmission Complete Interrupt Enable */
#define USART_CR1_TXEIE_TXFNFIE_Pos  (7U)
#define USART_CR1_TXEIE_TXFNFIE_Msk   (0x1UL << USART_CR1_TXEIE_TXFNFIE_Pos)   /*!< 0x00000080 */
#define USART_CR1_TXEIE_TXFNFIE       USART_CR1_TXEIE_TXFNFIE_Msk              /*!< TXE/TXFIFO not full Interrupt Enable */
#define USART_CR1_PEIE_Pos           (8U)
#define USART_CR1_PEIE_Msk           (0x1UL << USART_CR1_PEIE_Pos)             /*!< 0x00000100 */
#define USART_CR1_PEIE               USART_CR1_PEIE_Msk                        /*!< PE Interrupt Enable */
#define USART_CR1_PS_Pos             (9U)
#define USART_CR1_PS_Msk             (0x1UL << USART_CR1_PS_Pos)               /*!< 0x00000200 */
#define USART_CR1_PS                 USART_CR1_PS_Msk                          /*!< Parity Selection */
#define USART_CR1_PCE_Pos            (10U)
#define USART_CR1_PCE_Msk            (0x1UL << USART_CR1_PCE_Pos)              /*!< 0x00000400 */
#define USART_CR1_PCE                USART_CR1_PCE_Msk                         /*!< Parity Control Enable */
#define USART_CR1_WAKE_Pos           (11U)
#define USART_CR1_WAKE_Msk           (0x1UL << USART_CR1_WAKE_Pos)             /*!< 0x00000800 */
#define USART_CR1_WAKE               USART_CR1_WAKE_Msk                        /*!< Receiver Wakeup method */
#define USART_CR1_M_Pos              (12U)
#define USART_CR1_M_Msk              (0x10001UL << USART_CR1_M_Pos)            /*!< 0x10001000 */
#define USART_CR1_M                  USART_CR1_M_Msk                           /*!< Word length */
#define USART_CR1_M0_Pos             (12U)
#define USART_CR1_M0_Msk             (0x1UL << USART_CR1_M0_Pos)               /*!< 0x00001000 */
#define USART_CR1_M0                 USART_CR1_M0_Msk                          /*!< Word length - Bit 0 */
#define USART_CR1_MME_Pos            (13U)
#define USART_CR1_MME_Msk            (0x1UL << USART_CR1_MME_Pos)              /*!< 0x00002000 */
#define USART_CR1_MME                USART_CR1_MME_Msk                         /*!< Mute Mode Enable */
#define USART_CR1_CMIE_Pos           (14U)
#define USART_CR1_CMIE_Msk           (0x1UL << USART_CR1_CMIE_Pos)             /*!< 0x00004000 */
#define USART_CR1_CMIE               USART_CR1_CMIE_Msk                        /*!< Character match interrupt enable */
#define USART_CR1_OVER8_Pos          (15U)
#define USART_CR1_OVER8_Msk          (0x1UL << USART_CR1_OVER8_Pos)            /*!< 0x00008000 */
#define USART_CR1_OVER8              USART_CR1_OVER8_Msk                       /*!< Oversampling by 8-bit or 16-bit mode */
#define USART_CR1_DEDT_Pos           (16U)
#define USART_CR1_DEDT_Msk           (0x1FUL << USART_CR1_DEDT_Pos)            /*!< 0x001F0000 */
#define USART_CR1_DEDT               USART_CR1_DEDT_Msk                        /*!< DEDT[4:0] bits (Driver Enable Deassertion Time) */
#define USART_CR1_DEDT_0             (0x01UL << USART_CR1_DEDT_Pos)            /*!< 0x00010000 */
#define USART_CR1_DEDT_1             (0x02UL << USART_CR1_DEDT_Pos)            /*!< 0x00020000 */
#define USART_CR1_DEDT_2             (0x04UL << USART_CR1_DEDT_Pos)            /*!< 0x00040000 */
#define USART_CR1_DEDT_3             (0x08UL << USART_CR1_DEDT_Pos)            /*!< 0x00080000 */
#define USART_CR1_DEDT_4             (0x10UL << USART_CR1_DEDT_Pos)            /*!< 0x00100000 */
#define USART_CR1_DEAT_Pos           (21U)
#define USART_CR1_DEAT_Msk           (0x1FUL << USART_CR1_DEAT_Pos)            /*!< 0x03E00000 */
#define USART_CR1_DEAT               USART_CR1_DEAT_Msk                        /*!< DEAT[4:0] bits (Driver Enable Assertion Time) */
#define USART_CR1_DEAT_0             (0x01UL << USART_CR1_DEAT_Pos)            /*!< 0x00200000 */
#define USART_CR1_DEAT_1             (0x02UL << USART_CR1_DEAT_Pos)            /*!< 0x00400000 */
#define USART_CR1_DEAT_2             (0x04UL << USART_CR1_DEAT_Pos)            /*!< 0x00800000 */
#define USART_CR1_DEAT_3             (0x08UL << USART_CR1_DEAT_Pos)            /*!< 0x01000000 */
#define USART_CR1_DEAT_4             (0x10UL << USART_CR1_DEAT_Pos)            /*!< 0x02000000 */
#define USART_CR1_RTOIE_Pos          (26U)
#define USART_CR1_RTOIE_Msk          (0x1UL << USART_CR1_RTOIE_Pos)            /*!< 0x04000000 */
#define USART_CR1_RTOIE              USART_CR1_RTOIE_Msk                       /*!< Receive Time Out interrupt enable */
#define USART_CR1_EOBIE_Pos          (27U)
#define USART_CR1_EOBIE_Msk          (0x1UL << USART_CR1_EOBIE_Pos)            /*!< 0x08000000 */
#define USART_CR1_EOBIE              USART_CR1_EOBIE_Msk                       /*!< End of Block interrupt enable */
#define USART_CR1_M1_Pos             (28U)
#define USART_CR1_M1_Msk             (0x1UL << USART_CR1_M1_Pos)               /*!< 0x10000000 */
#define USART_CR1_M1                 USART_CR1_M1_Msk                          /*!< Word length - Bit 1 */
#define USART_CR1_FIFOEN_Pos         (29U)
#define USART_CR1_FIFOEN_Msk         (0x1UL << USART_CR1_FIFOEN_Pos)           /*!< 0x20000000 */
#define USART_CR1_FIFOEN             USART_CR1_FIFOEN_Msk                      /*!< FIFO mode enable */
#define USART_CR1_TXFEIE_Pos         (30U)
#define USART_CR1_TXFEIE_Msk         (0x1UL << USART_CR1_TXFEIE_Pos)           /*!< 0x40000000 */
#define USART_CR1_TXFEIE             USART_CR1_TXFEIE_Msk                      /*!< TXFIFO empty interrupt enable */
#define USART_CR1_RXFFIE_Pos         (31U)
#define USART_CR1_RXFFIE_Msk         (0x1UL << USART_CR1_RXFFIE_Pos)           /*!< 0x80000000 */
#define USART_CR1_RXFFIE             USART_CR1_RXFFIE_Msk                      /*!< RXFIFO Full interrupt enable */

/******************  Bit definition for USART_CR2 register  *******************/
#define USART_CR2_SLVEN_Pos          (0U)
#define USART_CR2_SLVEN_Msk          (0x1UL << USART_CR2_SLVEN_Pos)            /*!< 0x00000001 */
#define USART_CR2_SLVEN              USART_CR2_SLVEN_Msk                       /*!< Synchronous Slave mode enable */
#define USART_CR2_DIS_NSS_Pos        (3U)
#define USART_CR2_DIS_NSS_Msk        (0x1UL << USART_CR2_DIS_NSS_Pos)          /*!< 0x00000008 */
#define USART_CR2_DIS_NSS            USART_CR2_DIS_NSS_Msk                     /*!< NSS input pin disable for SPI slave selection */
#define USART_CR2_ADDM7_Pos          (4U)
#define USART_CR2_ADDM7_Msk          (0x1UL << USART_CR2_ADDM7_Pos)            /*!< 0x00000010 */
#define USART_CR2_ADDM7              USART_CR2_ADDM7_Msk                       /*!< 7-bit or 4-bit Address Detection */
#define USART_CR2_LBDL_Pos           (5U)
#define USART_CR2_LBDL_Msk           (0x1UL << USART_CR2_LBDL_Pos)             /*!< 0x00000020 */
#define USART_CR2_LBDL               USART_CR2_LBDL_Msk                        /*!< LIN Break Detection Length */
#define USART_CR2_LBDIE_Pos          (6U)
#define USART_CR2_LBDIE_Msk          (0x1UL << USART_CR2_LBDIE_Pos)            /*!< 0x00000040 */
#define USART_CR2_LBDIE              USART_CR2_LBDIE_Msk                       /*!< LIN Break Detection Interrupt Enable */
#define USART_CR2_LBCL_Pos           (8U)
#define USART_CR2_LBCL_Msk           (0x1UL << USART_CR2_LBCL_Pos)             /*!< 0x00000100 */
#define USART_CR2_LBCL               USART_CR2_LBCL_Msk                        /*!< Last Bit Clock pulse */
#define USART_CR2_CPHA_Pos           (9U)
#define USART_CR2_CPHA_Msk           (0x1UL << USART_CR2_CPHA_Pos)             /*!< 0x00000200 */
#define USART_CR2_CPHA               USART_CR2_CPHA_Msk                        /*!< Clock Phase */
#define USART_CR2_CPOL_Pos           (10U)
#define USART_CR2_CPOL_Msk           (0x1UL << USART_CR2_CPOL_Pos)             /*!< 0x00000400 */
#define USART_CR2_CPOL               USART_CR2_CPOL_Msk                        /*!< Clock Polarity */
#define USART_CR2_CLKEN_Pos          (11U)
#define USART_CR2_CLKEN_Msk          (0x1UL << USART_CR2_CLKEN_Pos)            /*!< 0x00000800 */
#define USART_CR2_CLKEN              USART_CR2_CLKEN_Msk                       /*!< Clock Enable */
#define USART_CR2_STOP_Pos           (12U)
#define USART_CR2_STOP_Msk           (0x3UL << USART_CR2_STOP_Pos)             /*!< 0x00003000 */
#define USART_CR2_STOP               USART_CR2_STOP_Msk                        /*!< STOP[1:0] bits (STOP bits) */
#define USART_CR2_STOP_0             (0x1UL << USART_CR2_STOP_Pos)             /*!< 0x00001000 */
#define USART_CR2_STOP_1             (0x2UL << USART_CR2_STOP_Pos)             /*!< 0x00002000 */
#define USART_CR2_LINEN_Pos          (14U)
#define USART_CR2_LINEN_Msk          (0x1UL << USART_CR2_LINEN_Pos)            /*!< 0x00004000 */
#define USART_CR2_LINEN              USART_CR2_LINEN_Msk                       /*!< LIN mode enable */
#define USART_CR2_SWAP_Pos           (15U)
#define USART_CR2_SWAP_Msk           (0x1UL << USART_CR2_SWAP_Pos)             /*!< 0x00008000 */
#define USART_CR2_SWAP               USART_CR2_SWAP_Msk                        /*!< SWAP TX/RX pins */
#define USART_CR2_RXINV_Pos          (16U)
#define USART_CR2_RXINV_Msk          (0x1UL << USART_CR2_RXINV_Pos)            /*!< 0x00010000 */
#define USART_CR2_RXINV              USART_CR2_RXINV_Msk                       /*!< RX pin active level inversion */
#define USART_CR2_TXINV_Pos          (17U)
#define USART_CR2_TXINV_Msk          (0x1UL << USART_CR2_TXINV_Pos)            /*!< 0x00020000 */
#define USART_CR2_TXINV              USART_CR2_TXINV_Msk                       /*!< TX pin active level inversion */
#define USART_CR2_DATAINV_Pos        (18U)
#define USART_CR2_DATAINV_Msk        (0x1UL << USART_CR2_DATAINV_Pos)          /*!< 0x00040000 */
#define USART_CR2_DATAINV            USART_CR2_DATAINV_Msk                     /*!< Binary data inversion */
#define USART_CR2_MSBFIRST_Pos       (19U)
#define USART_CR2_MSBFIRST_Msk       (0x1UL << USART_CR2_MSBFIRST_Pos)         /*!< 0x00080000 */
#define USART_CR2_MSBFIRST           USART_CR2_MSBFIRST_Msk                    /*!< Most Significant Bit First */
#define USART_CR2_ABREN_Pos          (20U)
#define USART_CR2_ABREN_Msk          (0x1UL << USART_CR2_ABREN_Pos)            /*!< 0x00100000 */
#define USART_CR2_ABREN              USART_CR2_ABREN_Msk                       /*!< Auto Baud-Rate Enable*/
#define USART_CR2_ABRMODE_Pos        (21U)
#define USART_CR2_ABRMODE_Msk        (0x3UL << USART_CR2_ABRMODE_Pos)          /*!< 0x00600000 */
#define USART_CR2_ABRMODE            USART_CR2_ABRMODE_Msk                     /*!< ABRMOD[1:0] bits (Auto Baud-Rate Mode) */
#define USART_CR2_ABRMODE_0          (0x1UL << USART_CR2_ABRMODE_Pos)          /*!< 0x00200000 */
#define USART_CR2_ABRMODE_1          (0x2UL << USART_CR2_ABRMODE_Pos)          /*!< 0x00400000 */
#define USART_CR2_RTOEN_Pos          (23U)
#define USART_CR2_RTOEN_Msk          (0x1UL << USART_CR2_RTOEN_Pos)            /*!< 0x00800000 */
#define USART_CR2_RTOEN              USART_CR2_RTOEN_Msk                       /*!< Receiver Time-Out enable */
#define USART_CR2_ADD_Pos            (24U)
#define USART_CR2_ADD_Msk            (0xFFUL << USART_CR2_ADD_Pos)             /*!< 0xFF000000 */
#define USART_CR2_ADD                USART_CR2_ADD_Msk                         /*!< Address of the USART node */

/******************  Bit definition for USART_CR3 register  *******************/
#define USART_CR3_EIE_Pos            (0U)
#define USART_CR3_EIE_Msk            (0x1UL << USART_CR3_EIE_Pos)              /*!< 0x00000001 */
#define USART_CR3_EIE                USART_CR3_EIE_Msk                         /*!< Error Interrupt Enable */
#define USART_CR3_IREN_Pos           (1U)
#define USART_CR3_IREN_Msk           (0x1UL << USART_CR3_IREN_Pos)             /*!< 0x00000002 */
#define USART_CR3_IREN               USART_CR3_IREN_Msk                        /*!< IrDA mode Enable */
#define USART_CR3_IRLP_Pos           (2U)
#define USART_CR3_IRLP_Msk           (0x1UL << USART_CR3_IRLP_Pos)             /*!< 0x00000004 */
#define USART_CR3_IRLP               USART_CR3_IRLP_Msk                        /*!< IrDA Low-Power */
#define USART_CR3_HDSEL_Pos          (3U)
#define USART_CR3_HDSEL_Msk          (0x1UL << USART_CR3_HDSEL_Pos)            /*!< 0x00000008 */
#define USART_CR3_HDSEL              USART_CR3_HDSEL_Msk                       /*!< Half-Duplex Selection */
#define USART_CR3_NACK_Pos           (4U)
#define USART_CR3_NACK_Msk           (0x1UL << USART_CR3_NACK_Pos)             /*!< 0x00000010 */
#define USART_CR3_NACK               USART_CR3_NACK_Msk                        /*!< SmartCard NACK enable */
#define USART_CR3_SCEN_Pos           (5U)
#define USART_CR3_SCEN_Msk           (0x1UL << USART_CR3_SCEN_Pos)             /*!< 0x00000020 */
#define USART_CR3_SCEN               USART_CR3_SCEN_Msk                        /*!< SmartCard mode enable */
#define USART_CR3_DMAR_Pos           (6U)
#define USART_CR3_DMAR_Msk           (0x1UL << USART_CR3_DMAR_Pos)             /*!< 0x00000040 */
#define USART_CR3_DMAR               USART_CR3_DMAR_Msk                        /*!< DMA Enable Receiver */
#define USART_CR3_DMAT_Pos           (7U)
#define USART_CR3_DMAT_Msk           (0x1UL << USART_CR3_DMAT_Pos)             /*!< 0x00000080 */
#define USART_CR3_DMAT               USART_CR3_DMAT_Msk                        /*!< DMA Enable Transmitter */
#define USART_CR3_RTSE_Pos           (8U)
#define USART_CR3_RTSE_Msk           (0x1UL << USART_CR3_RTSE_Pos)             /*!< 0x00000100 */
#define USART_CR3_RTSE               USART_CR3_RTSE_Msk                        /*!< RTS Enable */
#define USART_CR3_CTSE_Pos           (9U)
#define USART_CR3_CTSE_Msk           (0x1UL << USART_CR3_CTSE_Pos)             /*!< 0x00000200 */
#define USART_CR3_CTSE               USART_CR3_CTSE_Msk                        /*!< CTS Enable */
#define USART_CR3_CTSIE_Pos          (10U)
#define USART_CR3_CTSIE_Msk          (0x1UL << USART_CR3_CTSIE_Pos)            /*!< 0x00000400 */
#define USART_CR3_CTSIE              USART_CR3_CTSIE_Msk                       /*!< CTS Interrupt Enable */
#define USART_CR3_ONEBIT_Pos         (11U)
#define USART_CR3_ONEBIT_Msk         (0x1UL << USART_CR3_ONEBIT_Pos)           /*!< 0x00000800 */
#define USART_CR3_ONEBIT             USART_CR3_ONEBIT_Msk                      /*!< One sample bit method enable */
#define USART_CR3_OVRDIS_Pos         (12U)
#define USART_CR3_OVRDIS_Msk         (0x1UL << USART_CR3_OVRDIS_Pos)           /*!< 0x00001000 */
#define USART_CR3_OVRDIS             USART_CR3_OVRDIS_Msk                      /*!< Overrun Disable */
#define USART_CR3_DDRE_Pos           (13U)
#define USART_CR3_DDRE_Msk           (0x1UL << USART_CR3_DDRE_Pos)             /*!< 0x00002000 */
#define USART_CR3_DDRE               USART_CR3_DDRE_Msk                        /*!< DMA Disable on Reception Error */
#define USART_CR3_DEM_Pos            (14U)
#define USART_CR3_DEM_Msk            (0x1UL << USART_CR3_DEM_Pos)              /*!< 0x00004000 */
#define USART_CR3_DEM                USART_CR3_DEM_Msk                         /*!< Driver Enable Mode */
#define USART_CR3_DEP_Pos            (15U)
#define USART_CR3_DEP_Msk            (0x1UL << USART_CR3_DEP_Pos)              /*!< 0x00008000 */
#define USART_CR3_DEP                USART_CR3_DEP_Msk                         /*!< Driver Enable Polarity Selection */
#define USART_CR3_SCARCNT_Pos        (17U)
#define USART_CR3_SCARCNT_Msk        (0x7UL << USART_CR3_SCARCNT_Pos)          /*!< 0x000E0000 */
#define USART_CR3_SCARCNT            USART_CR3_SCARCNT_Msk                     /*!< SCARCNT[2:0] bits (SmartCard Auto-Retry Count) */
#define USART_CR3_SCARCNT_0          (0x1UL << USART_CR3_SCARCNT_Pos)          /*!< 0x00020000 */
#define USART_CR3_SCARCNT_1          (0x2UL << USART_CR3_SCARCNT_Pos)          /*!< 0x00040000 */
#define USART_CR3_SCARCNT_2          (0x4UL << USART_CR3_SCARCNT_Pos)          /*!< 0x00080000 */
#define USART_CR3_WUS_Pos            (20U)
#define USART_CR3_WUS_Msk            (0x3UL << USART_CR3_WUS_Pos)              /*!< 0x00300000 */
#define USART_CR3_WUS                USART_CR3_WUS_Msk                         /*!< WUS[1:0] bits (Wake UP Interrupt Flag Selection) */
#define USART_CR3_WUS_0              (0x1UL << USART_CR3_WUS_Pos)              /*!< 0x00100000 */
#define USART_CR3_WUS_1              (0x2UL << USART_CR3_WUS_Pos)              /*!< 0x00200000 */
#define USART_CR3_WUFIE_Pos          (22U)
#define USART_CR3_WUFIE_Msk          (0x1UL << USART_CR3_WUFIE_Pos)            /*!< 0x00400000 */
#define USART_CR3_WUFIE              USART_CR3_WUFIE_Msk                       /*!< Wake Up Interrupt Enable */
#define USART_CR3_TXFTIE_Pos         (23U)
#define USART_CR3_TXFTIE_Msk         (0x1UL << USART_CR3_TXFTIE_Pos)           /*!< 0x00800000 */
#define USART_CR3_TXFTIE             USART_CR3_TXFTIE_Msk                      /*!< TXFIFO threshold interrupt enable */
#define USART_CR3_TCBGTIE_Pos        (24U)
#define USART_CR3_TCBGTIE_Msk        (0x1UL << USART_CR3_TCBGTIE_Pos)          /*!< 0x01000000 */
#define USART_CR3_TCBGTIE            USART_CR3_TCBGTIE_Msk                     /*!< Transmission Complete Before Guard Time Interrupt Enable */
#define USART_CR3_RXFTCFG_Pos        (25U)
#define USART_CR3_RXFTCFG_Msk        (0x7UL << USART_CR3_RXFTCFG_Pos)          /*!< 0x0E000000 */
#define USART_CR3_RXFTCFG            USART_CR3_RXFTCFG_Msk                     /*!< RXFIFO FIFO threshold configuration */
#define USART_CR3_RXFTCFG_0          (0x1UL << USART_CR3_RXFTCFG_Pos)          /*!< 0x02000000 */
#define USART_CR3_RXFTCFG_1          (0x2UL << USART_CR3_RXFTCFG_Pos)          /*!< 0x04000000 */
#define USART_CR3_RXFTCFG_2          (0x4UL << USART_CR3_RXFTCFG_Pos)          /*!< 0x08000000 */
#define USART_CR3_RXFTIE_Pos         (28U)
#define USART_CR3_RXFTIE_Msk         (0x1UL << USART_CR3_RXFTIE_Pos)           /*!< 0x10000000 */
#define USART_CR3_RXFTIE             USART_CR3_RXFTIE_Msk                      /*!< RXFIFO threshold interrupt enable */
#define USART_CR3_TXFTCFG_Pos        (29U)
#define USART_CR3_TXFTCFG_Msk        (0x7UL << USART_CR3_TXFTCFG_Pos)          /*!< 0xE0000000 */
#define USART_CR3_TXFTCFG            USART_CR3_TXFTCFG_Msk                     /*!< TXFIFO threshold configuration */
#define USART_CR3_TXFTCFG_0          (0x1UL << USART_CR3_TXFTCFG_Pos)          /*!< 0x20000000 */
#define USART_CR3_TXFTCFG_1          (0x2UL << USART_CR3_TXFTCFG_Pos)          /*!< 0x40000000 */
#define USART_CR3_TXFTCFG_2          (0x4UL << USART_CR3_TXFTCFG_Pos)          /*!< 0x80000000 */

/******************  Bit definition for USART_BRR register  *******************/
#define USART_BRR_LPUART_Pos         (0U)
#define USART_BRR_LPUART_Msk         (0xFFFFFUL << USART_BRR_LPUART_Pos)       /*!< 0x000FFFFF */
#define USART_BRR_LPUART             USART_BRR_LPUART_Msk                      /*!< LPUART Baud rate register [19:0] */
#define USART_BRR_BRR                ((uint16_t)0xFFFF)                        /*!< USART  Baud rate register [15:0] */

/******************  Bit definition for USART_GTPR register  ******************/
#define USART_GTPR_PSC_Pos           (0U)
#define USART_GTPR_PSC_Msk           (0xFFUL << USART_GTPR_PSC_Pos)            /*!< 0x000000FF */
#define USART_GTPR_PSC               USART_GTPR_PSC_Msk                        /*!< PSC[7:0] bits (Prescaler value) */
#define USART_GTPR_GT_Pos            (8U)
#define USART_GTPR_GT_Msk            (0xFFUL << USART_GTPR_GT_Pos)             /*!< 0x0000FF00 */
#define USART_GTPR_GT                USART_GTPR_GT_Msk                         /*!< GT[7:0] bits (Guard time value) */

/*******************  Bit definition for USART_RTOR register  *****************/
#define USART_RTOR_RTO_Pos           (0U)
#define USART_RTOR_RTO_Msk           (0xFFFFFFUL << USART_RTOR_RTO_Pos)        /*!< 0x00FFFFFF */
#define USART_RTOR_RTO               USART_RTOR_RTO_Msk                        /*!< Receiver Time Out Value */
#define USART_RTOR_BLEN_Pos          (24U)
#define USART_RTOR_BLEN_Msk          (0xFFUL << USART_RTOR_BLEN_Pos)           /*!< 0xFF000000 */
#define USART_RTOR_BLEN              USART_RTOR_BLEN_Msk                       /*!< Block Length */

/*******************  Bit definition for USART_RQR register  ******************/
#define USART_RQR_ABRRQ        ((uint16_t)0x0001)                              /*!< Auto-Baud Rate Request */
#define USART_RQR_SBKRQ        ((uint16_t)0x0002)                              /*!< Send Break Request */
#define USART_RQR_MMRQ         ((uint16_t)0x0004)                              /*!< Mute Mode Request */
#define USART_RQR_RXFRQ        ((uint16_t)0x0008)                              /*!< Receive Data flush Request */
#define USART_RQR_TXFRQ        ((uint16_t)0x0010)                              /*!< Transmit data flush Request */

/*******************  Bit definition for USART_ISR register  ******************/
#define USART_ISR_PE_Pos             (0U)
#define USART_ISR_PE_Msk             (0x1UL << USART_ISR_PE_Pos)               /*!< 0x00000001 */
#define USART_ISR_PE                 USART_ISR_PE_Msk                          /*!< Parity Error */
#define USART_ISR_FE_Pos             (1U)
#define USART_ISR_FE_Msk             (0x1UL << USART_ISR_FE_Pos)               /*!< 0x00000002 */
#define USART_ISR_FE                 USART_ISR_FE_Msk                          /*!< Framing Error */
#define USART_ISR_NE_Pos             (2U)
#define USART_ISR_NE_Msk             (0x1UL << USART_ISR_NE_Pos)               /*!< 0x00000004 */
#define USART_ISR_NE                 USART_ISR_NE_Msk                          /*!< Noise detected Flag */
#define USART_ISR_ORE_Pos            (3U)
#define USART_ISR_ORE_Msk            (0x1UL << USART_ISR_ORE_Pos)              /*!< 0x00000008 */
#define USART_ISR_ORE                USART_ISR_ORE_Msk                         /*!< OverRun Error */
#define USART_ISR_IDLE_Pos           (4U)
#define USART_ISR_IDLE_Msk           (0x1UL << USART_ISR_IDLE_Pos)             /*!< 0x00000010 */
#define USART_ISR_IDLE               USART_ISR_IDLE_Msk                        /*!< IDLE line detected */
#define USART_ISR_RXNE_RXFNE_Pos     (5U)
#define USART_ISR_RXNE_RXFNE_Msk     (0x1UL << USART_ISR_RXNE_RXFNE_Pos)       /*!< 0x00000020 */
#define USART_ISR_RXNE_RXFNE         USART_ISR_RXNE_RXFNE_Msk                  /*!< Read Data Register Not Empty/RXFIFO Not Empty */
#define USART_ISR_TC_Pos             (6U)
#define USART_ISR_TC_Msk             (0x1UL << USART_ISR_TC_Pos)               /*!< 0x00000040 */
#define USART_ISR_TC                 USART_ISR_TC_Msk                          /*!< Transmission Complete */
#define USART_ISR_TXE_TXFNF_Pos      (7U)
#define USART_ISR_TXE_TXFNF_Msk      (0x1UL << USART_ISR_TXE_TXFNF_Pos)        /*!< 0x00000080 */
#define USART_ISR_TXE_TXFNF          USART_ISR_TXE_TXFNF_Msk                   /*!< Transmit Data Register Empty/TXFIFO Not Full */
#define USART_ISR_LBDF_Pos           (8U)
#define USART_ISR_LBDF_Msk           (0x1UL << USART_ISR_LBDF_Pos)             /*!< 0x00000100 */
#define USART_ISR_LBDF               USART_ISR_LBDF_Msk                        /*!< LIN Break Detection Flag */
#define USART_ISR_CTSIF_Pos          (9U)
#define USART_ISR_CTSIF_Msk          (0x1UL << USART_ISR_CTSIF_Pos)            /*!< 0x00000200 */
#define USART_ISR_CTSIF              USART_ISR_CTSIF_Msk                       /*!< CTS interrupt flag */
#define USART_ISR_CTS_Pos            (10U)
#define USART_ISR_CTS_Msk            (0x1UL << USART_ISR_CTS_Pos)              /*!< 0x00000400 */
#define USART_ISR_CTS                USART_ISR_CTS_Msk                         /*!< CTS flag */
#define USART_ISR_RTOF_Pos           (11U)
#define USART_ISR_RTOF_Msk           (0x1UL << USART_ISR_RTOF_Pos)             /*!< 0x00000800 */
#define USART_ISR_RTOF               USART_ISR_RTOF_Msk                        /*!< Receiver Time Out */
#define USART_ISR_EOBF_Pos           (12U)
#define USART_ISR_EOBF_Msk           (0x1UL << USART_ISR_EOBF_Pos)             /*!< 0x00001000 */
#define USART_ISR_EOBF               USART_ISR_EOBF_Msk                        /*!< End Of Block Flag */
#define USART_ISR_UDR_Pos            (13U)
#define USART_ISR_UDR_Msk            (0x1UL << USART_ISR_UDR_Pos)              /*!< 0x00002000 */
#define USART_ISR_UDR                 USART_ISR_UDR_Msk                        /*!< SPI Slave Underrun Error Flag */
#define USART_ISR_ABRE_Pos           (14U)
#define USART_ISR_ABRE_Msk           (0x1UL << USART_ISR_ABRE_Pos)             /*!< 0x00004000 */
#define USART_ISR_ABRE               USART_ISR_ABRE_Msk                        /*!< Auto-Baud Rate Error */
#define USART_ISR_ABRF_Pos           (15U)
#define USART_ISR_ABRF_Msk           (0x1UL << USART_ISR_ABRF_Pos)             /*!< 0x00008000 */
#define USART_ISR_ABRF               USART_ISR_ABRF_Msk                        /*!< Auto-Baud Rate Flag */
#define USART_ISR_BUSY_Pos           (16U)
#define USART_ISR_BUSY_Msk           (0x1UL << USART_ISR_BUSY_Pos)             /*!< 0x00010000 */
#define USART_ISR_BUSY               USART_ISR_BUSY_Msk                        /*!< Busy Flag */
#define USART_ISR_CMF_Pos            (17U)
#define USART_ISR_CMF_Msk            (0x1UL << USART_ISR_CMF_Pos)              /*!< 0x00020000 */
#define USART_ISR_CMF                USART_ISR_CMF_Msk                         /*!< Character Match Flag */
#define USART_ISR_SBKF_Pos           (18U)
#define USART_ISR_SBKF_Msk           (0x1UL << USART_ISR_SBKF_Pos)             /*!< 0x00040000 */
#define USART_ISR_SBKF               USART_ISR_SBKF_Msk                        /*!< Send Break Flag */
#define USART_ISR_RWU_Pos            (19U)
#define USART_ISR_RWU_Msk            (0x1UL << USART_ISR_RWU_Pos)              /*!< 0x00080000 */
#define USART_ISR_RWU                USART_ISR_RWU_Msk                         /*!< Receive Wake Up from mute mode Flag */
#define USART_ISR_WUF_Pos            (20U)
#define USART_ISR_WUF_Msk            (0x1UL << USART_ISR_WUF_Pos)              /*!< 0x00100000 */
#define USART_ISR_WUF                USART_ISR_WUF_Msk                         /*!< Wake Up from stop mode Flag */
#define USART_ISR_TEACK_Pos          (21U)
#define USART_ISR_TEACK_Msk          (0x1UL << USART_ISR_TEACK_Pos)            /*!< 0x00200000 */
#define USART_ISR_TEACK              USART_ISR_TEACK_Msk                       /*!< Transmit Enable Acknowledge Flag */
#define USART_ISR_REACK_Pos          (22U)
#define USART_ISR_REACK_Msk          (0x1UL << USART_ISR_REACK_Pos)            /*!< 0x00400000 */
#define USART_ISR_REACK              USART_ISR_REACK_Msk                       /*!< Receive Enable Acknowledge Flag */
#define USART_ISR_TXFE_Pos           (23U)
#define USART_ISR_TXFE_Msk           (0x1UL << USART_ISR_TXFE_Pos)             /*!< 0x00800000 */
#define USART_ISR_TXFE               USART_ISR_TXFE_Msk                        /*!< TXFIFO Empty Flag */
#define USART_ISR_RXFF_Pos           (24U)
#define USART_ISR_RXFF_Msk           (0x1UL << USART_ISR_RXFF_Pos)             /*!< 0x01000000 */
#define USART_ISR_RXFF               USART_ISR_RXFF_Msk                        /*!< RXFIFO Full Flag */
#define USART_ISR_TCBGT_Pos          (25U)
#define USART_ISR_TCBGT_Msk          (0x1UL << USART_ISR_TCBGT_Pos)            /*!< 0x02000000 */
#define USART_ISR_TCBGT              USART_ISR_TCBGT_Msk                       /*!< Transmission Complete Before Guard Time Completion Flag */
#define USART_ISR_RXFT_Pos           (26U)
#define USART_ISR_RXFT_Msk           (0x1UL << USART_ISR_RXFT_Pos)             /*!< 0x04000000 */
#define USART_ISR_RXFT               USART_ISR_RXFT_Msk                        /*!< RXFIFO Threshold Flag */
#define USART_ISR_TXFT_Pos           (27U)
#define USART_ISR_TXFT_Msk           (0x1UL << USART_ISR_TXFT_Pos)             /*!< 0x08000000 */
#define USART_ISR_TXFT               USART_ISR_TXFT_Msk                        /*!< TXFIFO Threshold Flag */

/*******************  Bit definition for USART_ICR register  ******************/
#define USART_ICR_PECF_Pos           (0U)
#define USART_ICR_PECF_Msk           (0x1UL << USART_ICR_PECF_Pos)             /*!< 0x00000001 */
#define USART_ICR_PECF               USART_ICR_PECF_Msk                        /*!< Parity Error Clear Flag */
#define USART_ICR_FECF_Pos           (1U)
#define USART_ICR_FECF_Msk           (0x1UL << USART_ICR_FECF_Pos)             /*!< 0x00000002 */
#define USART_ICR_FECF               USART_ICR_FECF_Msk                        /*!< Framing Error Clear Flag */
#define USART_ICR_NECF_Pos           (2U)
#define USART_ICR_NECF_Msk           (0x1UL << USART_ICR_NECF_Pos)             /*!< 0x00000004 */
#define USART_ICR_NECF               USART_ICR_NECF_Msk                        /*!< Noise Error detected Clear Flag */
#define USART_ICR_ORECF_Pos          (3U)
#define USART_ICR_ORECF_Msk          (0x1UL << USART_ICR_ORECF_Pos)            /*!< 0x00000008 */
#define USART_ICR_ORECF              USART_ICR_ORECF_Msk                       /*!< OverRun Error Clear Flag */
#define USART_ICR_IDLECF_Pos         (4U)
#define USART_ICR_IDLECF_Msk         (0x1UL << USART_ICR_IDLECF_Pos)           /*!< 0x00000010 */
#define USART_ICR_IDLECF             USART_ICR_IDLECF_Msk                      /*!< IDLE line detected Clear Flag */
#define USART_ICR_TXFECF_Pos         (5U)
#define USART_ICR_TXFECF_Msk         (0x1UL << USART_ICR_TXFECF_Pos)           /*!< 0x00000020 */
#define USART_ICR_TXFECF             USART_ICR_TXFECF_Msk                      /*!< TXFIFO Empty Clear Flag */
#define USART_ICR_TCCF_Pos           (6U)
#define USART_ICR_TCCF_Msk           (0x1UL << USART_ICR_TCCF_Pos)             /*!< 0x00000040 */
#define USART_ICR_TCCF               USART_ICR_TCCF_Msk                        /*!< Transmission Complete Clear Flag */
#define USART_ICR_TCBGTCF_Pos        (7U)
#define USART_ICR_TCBGTCF_Msk        (0x1UL << USART_ICR_TCBGTCF_Pos)          /*!< 0x00000080 */
#define USART_ICR_TCBGTCF            USART_ICR_TCBGTCF_Msk                     /*!< Transmission Complete Before Guard Time Clear Flag */
#define USART_ICR_LBDCF_Pos          (8U)
#define USART_ICR_LBDCF_Msk          (0x1UL << USART_ICR_LBDCF_Pos)            /*!< 0x00000100 */
#define USART_ICR_LBDCF              USART_ICR_LBDCF_Msk                       /*!< LIN Break Detection Clear Flag */
#define USART_ICR_CTSCF_Pos          (9U)
#define USART_ICR_CTSCF_Msk          (0x1UL << USART_ICR_CTSCF_Pos)            /*!< 0x00000200 */
#define USART_ICR_CTSCF              USART_ICR_CTSCF_Msk                       /*!< CTS Interrupt Clear Flag */
#define USART_ICR_RTOCF_Pos          (11U)
#define USART_ICR_RTOCF_Msk          (0x1UL << USART_ICR_RTOCF_Pos)            /*!< 0x00000800 */
#define USART_ICR_RTOCF              USART_ICR_RTOCF_Msk                       /*!< Receiver Time Out Clear Flag */
#define USART_ICR_EOBCF_Pos          (12U)
#define USART_ICR_EOBCF_Msk          (0x1UL << USART_ICR_EOBCF_Pos)            /*!< 0x00001000 */
#define USART_ICR_EOBCF              USART_ICR_EOBCF_Msk                       /*!< End Of Block Clear Flag */
#define USART_ICR_UDRCF_Pos          (13U)
#define USART_ICR_UDRCF_Msk          (0x1UL << USART_ICR_UDRCF_Pos)            /*!< 0x00002000 */
#define USART_ICR_UDRCF              USART_ICR_UDRCF_Msk                       /*!< SPI Slave Underrun Clear Flag */
#define USART_ICR_CMCF_Pos           (17U)
#define USART_ICR_CMCF_Msk           (0x1UL << USART_ICR_CMCF_Pos)             /*!< 0x00020000 */
#define USART_ICR_CMCF               USART_ICR_CMCF_Msk                        /*!< Character Match Clear Flag */
#define USART_ICR_WUCF_Pos           (20U)
#define USART_ICR_WUCF_Msk           (0x1UL << USART_ICR_WUCF_Pos)             /*!< 0x00100000 */
#define USART_ICR_WUCF               USART_ICR_WUCF_Msk                        /*!< Wake Up from stop mode Clear Flag */

/*******************  Bit definition for USART_RDR register  ******************/
#define USART_RDR_RDR_Pos             (0U)
#define USART_RDR_RDR_Msk             (0x1FFUL << USART_RDR_RDR_Pos)           /*!< 0x000001FF */
#define USART_RDR_RDR                 USART_RDR_RDR_Msk                        /*!< RDR[8:0] bits (Receive Data value) */

/*******************  Bit definition for USART_TDR register  ******************/
#define USART_TDR_TDR_Pos             (0U)
#define USART_TDR_TDR_Msk             (0x1FFUL << USART_TDR_TDR_Pos)           /*!< 0x000001FF */
#define USART_TDR_TDR                 USART_TDR_TDR_Msk                        /*!< TDR[8:0] bits (Transmit Data value) */

/*******************  Bit definition for USART_PRESC register  ****************/
#define USART_PRESC_PRESCALER_Pos    (0U)
#define USART_PRESC_PRESCALER_Msk    (0xFUL << USART_PRESC_PRESCALER_Pos)      /*!< 0x0000000F */
#define USART_PRESC_PRESCALER        USART_PRESC_PRESCALER_Msk                 /*!< PRESCALER[3:0] bits (Clock prescaler) */
#define USART_PRESC_PRESCALER_0      (0x1UL << USART_PRESC_PRESCALER_Pos)      /*!< 0x00000001 */
#define USART_PRESC_PRESCALER_1      (0x2UL << USART_PRESC_PRESCALER_Pos)      /*!< 0x00000002 */
#define USART_PRESC_PRESCALER_2      (0x4UL << USART_PRESC_PRESCALER_Pos)      /*!< 0x00000004 */
#define USART_PRESC_PRESCALER_3      (0x8UL << USART_PRESC_PRESCALER_Pos)      /*!< 0x00000008 */


/******************************************************************************/
/*                                                                            */
/*                                 VREFBUF                                    */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for VREFBUF_CSR register  ****************/
#define VREFBUF_CSR_ENVR_Pos    (0U)
#define VREFBUF_CSR_ENVR_Msk    (0x1UL << VREFBUF_CSR_ENVR_Pos)                /*!< 0x00000001 */
#define VREFBUF_CSR_ENVR        VREFBUF_CSR_ENVR_Msk                           /*!<Voltage reference buffer enable */
#define VREFBUF_CSR_HIZ_Pos     (1U)
#define VREFBUF_CSR_HIZ_Msk     (0x1UL << VREFBUF_CSR_HIZ_Pos)                 /*!< 0x00000002 */
#define VREFBUF_CSR_HIZ         VREFBUF_CSR_HIZ_Msk                            /*!<High impedance mode             */
#define VREFBUF_CSR_VRS_Pos     (2U)
#define VREFBUF_CSR_VRS_Msk     (0x1UL << VREFBUF_CSR_VRS_Pos)                 /*!< 0x00000004 */
#define VREFBUF_CSR_VRS         VREFBUF_CSR_VRS_Msk                            /*!<Voltage reference scale         */
#define VREFBUF_CSR_VRR_Pos     (3U)
#define VREFBUF_CSR_VRR_Msk     (0x1UL << VREFBUF_CSR_VRR_Pos)                 /*!< 0x00000008 */
#define VREFBUF_CSR_VRR         VREFBUF_CSR_VRR_Msk                            /*!<Voltage reference buffer ready  */

/*******************  Bit definition for VREFBUF_CCR register  ******************/
#define VREFBUF_CCR_TRIM_Pos    (0U)
#define VREFBUF_CCR_TRIM_Msk    (0x3FUL << VREFBUF_CCR_TRIM_Pos)               /*!< 0x0000003F */
#define VREFBUF_CCR_TRIM        VREFBUF_CCR_TRIM_Msk                           /*!<TRIM[5:0] bits (Trimming code)  */

/******************************************************************************/
/*                                                                            */
/*                            Window WATCHDOG                                 */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for WWDG_CR register  ********************/
#define WWDG_CR_T_Pos           (0U)
#define WWDG_CR_T_Msk           (0x7FUL << WWDG_CR_T_Pos)                      /*!< 0x0000007F */
#define WWDG_CR_T               WWDG_CR_T_Msk                                  /*!<T[6:0] bits (7-Bit counter (MSB to LSB)) */
#define WWDG_CR_T_0             (0x01UL << WWDG_CR_T_Pos)                      /*!< 0x00000001 */
#define WWDG_CR_T_1             (0x02UL << WWDG_CR_T_Pos)                      /*!< 0x00000002 */
#define WWDG_CR_T_2             (0x04UL << WWDG_CR_T_Pos)                      /*!< 0x00000004 */
#define WWDG_CR_T_3             (0x08UL << WWDG_CR_T_Pos)                      /*!< 0x00000008 */
#define WWDG_CR_T_4             (0x10UL << WWDG_CR_T_Pos)                      /*!< 0x00000010 */
#define WWDG_CR_T_5             (0x20UL << WWDG_CR_T_Pos)                      /*!< 0x00000020 */
#define WWDG_CR_T_6             (0x40UL << WWDG_CR_T_Pos)                      /*!< 0x00000040 */

#define WWDG_CR_WDGA_Pos        (7U)
#define WWDG_CR_WDGA_Msk        (0x1UL << WWDG_CR_WDGA_Pos)                    /*!< 0x00000080 */
#define WWDG_CR_WDGA            WWDG_CR_WDGA_Msk                               /*!<Activation bit */

/*******************  Bit definition for WWDG_CFR register  *******************/
#define WWDG_CFR_W_Pos          (0U)
#define WWDG_CFR_W_Msk          (0x7FUL << WWDG_CFR_W_Pos)                     /*!< 0x0000007F */
#define WWDG_CFR_W              WWDG_CFR_W_Msk                                 /*!<W[6:0] bits (7-bit window value) */
#define WWDG_CFR_W_0            (0x01UL << WWDG_CFR_W_Pos)                     /*!< 0x00000001 */
#define WWDG_CFR_W_1            (0x02UL << WWDG_CFR_W_Pos)                     /*!< 0x00000002 */
#define WWDG_CFR_W_2            (0x04UL << WWDG_CFR_W_Pos)                     /*!< 0x00000004 */
#define WWDG_CFR_W_3            (0x08UL << WWDG_CFR_W_Pos)                     /*!< 0x00000008 */
#define WWDG_CFR_W_4            (0x10UL << WWDG_CFR_W_Pos)                     /*!< 0x00000010 */
#define WWDG_CFR_W_5            (0x20UL << WWDG_CFR_W_Pos)                     /*!< 0x00000020 */
#define WWDG_CFR_W_6            (0x40UL << WWDG_CFR_W_Pos)                     /*!< 0x00000040 */

#define WWDG_CFR_WDGTB_Pos      (11U)
#define WWDG_CFR_WDGTB_Msk      (0x7UL << WWDG_CFR_WDGTB_Pos)                  /*!< 0x00003800 */
#define WWDG_CFR_WDGTB          WWDG_CFR_WDGTB_Msk                             /*!<WDGTB[2:0] bits (Timer Base) */
#define WWDG_CFR_WDGTB_0        (0x1UL << WWDG_CFR_WDGTB_Pos)                  /*!< 0x00000800 */
#define WWDG_CFR_WDGTB_1        (0x2UL << WWDG_CFR_WDGTB_Pos)                  /*!< 0x00001000 */
#define WWDG_CFR_WDGTB_2        (0x4UL << WWDG_CFR_WDGTB_Pos)                  /*!< 0x00002000 */

#define WWDG_CFR_EWI_Pos        (9U)
#define WWDG_CFR_EWI_Msk        (0x1UL << WWDG_CFR_EWI_Pos)                    /*!< 0x00000200 */
#define WWDG_CFR_EWI            WWDG_CFR_EWI_Msk                               /*!<Early Wakeup Interrupt */

/*******************  Bit definition for WWDG_SR register  ********************/
#define WWDG_SR_EWIF_Pos        (0U)
#define WWDG_SR_EWIF_Msk        (0x1UL << WWDG_SR_EWIF_Pos)                    /*!< 0x00000001 */
#define WWDG_SR_EWIF            WWDG_SR_EWIF_Msk                               /*!<Early Wakeup Interrupt Flag */

/******************************************************************************/
/*                                                                            */
/*                                Debug MCU                                   */
/*                                                                            */
/******************************************************************************/
/********************  Bit definition for DBG_IDCODE register  *************/
#define DBG_IDCODE_DEV_ID_Pos                          (0U)
#define DBG_IDCODE_DEV_ID_Msk                          (0xFFFUL << DBG_IDCODE_DEV_ID_Pos)  /*!< 0x00000FFF */
#define DBG_IDCODE_DEV_ID                              DBG_IDCODE_DEV_ID_Msk
#define DBG_IDCODE_REV_ID_Pos                          (16U)
#define DBG_IDCODE_REV_ID_Msk                          (0xFFFFUL << DBG_IDCODE_REV_ID_Pos) /*!< 0xFFFF0000 */
#define DBG_IDCODE_REV_ID                              DBG_IDCODE_REV_ID_Msk

/********************  Bit definition for DBG_CR register  *****************/
#define DBG_CR_DBG_STOP_Pos                            (1U)
#define DBG_CR_DBG_STOP_Msk                            (0x1UL << DBG_CR_DBG_STOP_Pos)      /*!< 0x00000002 */
#define DBG_CR_DBG_STOP                                DBG_CR_DBG_STOP_Msk
#define DBG_CR_DBG_STANDBY_Pos                         (2U)
#define DBG_CR_DBG_STANDBY_Msk                         (0x1UL << DBG_CR_DBG_STANDBY_Pos)   /*!< 0x00000004 */
#define DBG_CR_DBG_STANDBY                             DBG_CR_DBG_STANDBY_Msk


/********************  Bit definition for DBG_APB_FZ1 register  ***********/
#define DBG_APB_FZ1_DBG_TIM2_STOP_Pos                  (0U)
#define DBG_APB_FZ1_DBG_TIM2_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_TIM2_STOP_Pos) /*!< 0x00000001 */
#define DBG_APB_FZ1_DBG_TIM2_STOP                      DBG_APB_FZ1_DBG_TIM2_STOP_Msk
#define DBG_APB_FZ1_DBG_TIM3_STOP_Pos                  (1U)
#define DBG_APB_FZ1_DBG_TIM3_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_TIM3_STOP_Pos) /*!< 0x00000002 */
#define DBG_APB_FZ1_DBG_TIM3_STOP                      DBG_APB_FZ1_DBG_TIM3_STOP_Msk
#define DBG_APB_FZ1_DBG_TIM6_STOP_Pos                  (4U)
#define DBG_APB_FZ1_DBG_TIM6_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_TIM6_STOP_Pos) /*!< 0x00000010 */
#define DBG_APB_FZ1_DBG_TIM6_STOP                      DBG_APB_FZ1_DBG_TIM6_STOP_Msk
#define DBG_APB_FZ1_DBG_TIM7_STOP_Pos                  (5U)
#define DBG_APB_FZ1_DBG_TIM7_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_TIM7_STOP_Pos) /*!< 0x00000020 */
#define DBG_APB_FZ1_DBG_TIM7_STOP                      DBG_APB_FZ1_DBG_TIM7_STOP_Msk
#define DBG_APB_FZ1_DBG_RTC_STOP_Pos                   (10U)
#define DBG_APB_FZ1_DBG_RTC_STOP_Msk                   (0x1UL << DBG_APB_FZ1_DBG_RTC_STOP_Pos)  /*!< 0x00000400 */
#define DBG_APB_FZ1_DBG_RTC_STOP                       DBG_APB_FZ1_DBG_RTC_STOP_Msk
#define DBG_APB_FZ1_DBG_WWDG_STOP_Pos                  (11U)
#define DBG_APB_FZ1_DBG_WWDG_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_WWDG_STOP_Pos) /*!< 0x00000800 */
#define DBG_APB_FZ1_DBG_WWDG_STOP                      DBG_APB_FZ1_DBG_WWDG_STOP_Msk
#define DBG_APB_FZ1_DBG_IWDG_STOP_Pos                  (12U)
#define DBG_APB_FZ1_DBG_IWDG_STOP_Msk                  (0x1UL << DBG_APB_FZ1_DBG_IWDG_STOP_Pos) /*!< 0x00001000 */
#define DBG_APB_FZ1_DBG_IWDG_STOP                      DBG_APB_FZ1_DBG_IWDG_STOP_Msk
#define DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP_Pos    (21U)
#define DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP_Msk    (0x1UL << DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP_Pos) /*!< 0x00200000 */
#define DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP        DBG_APB_FZ1_DBG_I2C1_SMBUS_TIMEOUT_STOP_Msk
#define DBG_APB_FZ1_DBG_LPTIM2_STOP_Pos                (30U)
#define DBG_APB_FZ1_DBG_LPTIM2_STOP_Msk                (0x1UL << DBG_APB_FZ1_DBG_LPTIM2_STOP_Pos) /*!< 0x40000000 */
#define DBG_APB_FZ1_DBG_LPTIM2_STOP                    DBG_APB_FZ1_DBG_LPTIM2_STOP_Msk
#define DBG_APB_FZ1_DBG_LPTIM1_STOP_Pos                (31U)
#define DBG_APB_FZ1_DBG_LPTIM1_STOP_Msk                (0x1UL << DBG_APB_FZ1_DBG_LPTIM1_STOP_Pos) /*!< 0x80000000 */
#define DBG_APB_FZ1_DBG_LPTIM1_STOP                    DBG_APB_FZ1_DBG_LPTIM1_STOP_Msk

/********************  Bit definition for DBG_APB_FZ2 register  ************/
#define DBG_APB_FZ2_DBG_TIM1_STOP_Pos                  (11U)
#define DBG_APB_FZ2_DBG_TIM1_STOP_Msk                  (0x1UL << DBG_APB_FZ2_DBG_TIM1_STOP_Pos)  /*!< 0x00000800 */
#define DBG_APB_FZ2_DBG_TIM1_STOP                      DBG_APB_FZ2_DBG_TIM1_STOP_Msk
#define DBG_APB_FZ2_DBG_TIM14_STOP_Pos                 (15U)
#define DBG_APB_FZ2_DBG_TIM14_STOP_Msk                 (0x1UL << DBG_APB_FZ2_DBG_TIM14_STOP_Pos) /*!< 0x00008000 */
#define DBG_APB_FZ2_DBG_TIM14_STOP                     DBG_APB_FZ2_DBG_TIM14_STOP_Msk
#define DBG_APB_FZ2_DBG_TIM15_STOP_Pos                 (16U)
#define DBG_APB_FZ2_DBG_TIM15_STOP_Msk                 (0x1UL << DBG_APB_FZ2_DBG_TIM15_STOP_Pos) /*!< 0x00010000 */
#define DBG_APB_FZ2_DBG_TIM15_STOP                     DBG_APB_FZ2_DBG_TIM15_STOP_Msk
#define DBG_APB_FZ2_DBG_TIM16_STOP_Pos                 (17U)
#define DBG_APB_FZ2_DBG_TIM16_STOP_Msk                 (0x1UL << DBG_APB_FZ2_DBG_TIM16_STOP_Pos) /*!< 0x00020000 */
#define DBG_APB_FZ2_DBG_TIM16_STOP                     DBG_APB_FZ2_DBG_TIM16_STOP_Msk
#define DBG_APB_FZ2_DBG_TIM17_STOP_Pos                 (18U)
#define DBG_APB_FZ2_DBG_TIM17_STOP_Msk                 (0x1UL << DBG_APB_FZ2_DBG_TIM17_STOP_Pos) /*!< 0x00040000 */
#define DBG_APB_FZ2_DBG_TIM17_STOP                     DBG_APB_FZ2_DBG_TIM17_STOP_Msk

/******************************************************************************/
/*                                                                            */
/*                                    UCPD                                   */
/*                                                                            */
/******************************************************************************/
/********************  Bits definition for UCPD_CFG1 register  *******************/
#define UCPD_CFG1_HBITCLKDIV_Pos            (0U)
#define UCPD_CFG1_HBITCLKDIV_Msk            (0x3FUL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x0000003F */
#define UCPD_CFG1_HBITCLKDIV                UCPD_CFG1_HBITCLKDIV_Msk              /*!< Number of cycles (minus 1) for a half bit clock */
#define UCPD_CFG1_HBITCLKDIV_0              (0x01UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000001 */
#define UCPD_CFG1_HBITCLKDIV_1              (0x02UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000002 */
#define UCPD_CFG1_HBITCLKDIV_2              (0x04UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000004 */
#define UCPD_CFG1_HBITCLKDIV_3              (0x08UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000008 */
#define UCPD_CFG1_HBITCLKDIV_4              (0x10UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000010 */
#define UCPD_CFG1_HBITCLKDIV_5              (0x20UL << UCPD_CFG1_HBITCLKDIV_Pos)  /*!< 0x00000020 */
#define UCPD_CFG1_IFRGAP_Pos                (6U)
#define UCPD_CFG1_IFRGAP_Msk                (0x1FUL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x000007C0 */
#define UCPD_CFG1_IFRGAP                    UCPD_CFG1_IFRGAP_Msk                  /*!< Clock divider value to generates Interframe gap */
#define UCPD_CFG1_IFRGAP_0                  (0x01UL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x00000040 */
#define UCPD_CFG1_IFRGAP_1                  (0x02UL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x00000080 */
#define UCPD_CFG1_IFRGAP_2                  (0x04UL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x00000100 */
#define UCPD_CFG1_IFRGAP_3                  (0x08UL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x00000200 */
#define UCPD_CFG1_IFRGAP_4                  (0x10UL << UCPD_CFG1_IFRGAP_Pos)      /*!< 0x00000400 */
#define UCPD_CFG1_TRANSWIN_Pos              (11U)
#define UCPD_CFG1_TRANSWIN_Msk              (0x1FUL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x0000F800 */
#define UCPD_CFG1_TRANSWIN                  UCPD_CFG1_TRANSWIN_Msk                /*!< Number of cycles (minus 1) of the half bit clock */
#define UCPD_CFG1_TRANSWIN_0                (0x01UL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x00000800 */
#define UCPD_CFG1_TRANSWIN_1                (0x02UL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x00001000 */
#define UCPD_CFG1_TRANSWIN_2                (0x04UL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x00002000 */
#define UCPD_CFG1_TRANSWIN_3                (0x08UL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x00004000 */
#define UCPD_CFG1_TRANSWIN_4                (0x10UL << UCPD_CFG1_TRANSWIN_Pos)    /*!< 0x00008000 */
#define UCPD_CFG1_PSC_UCPDCLK_Pos           (17U)
#define UCPD_CFG1_PSC_UCPDCLK_Msk           (0x7UL << UCPD_CFG1_PSC_UCPDCLK_Pos)  /*!< 0x000E0000 */
#define UCPD_CFG1_PSC_UCPDCLK               UCPD_CFG1_PSC_UCPDCLK_Msk             /*!< Prescaler for UCPDCLK */
#define UCPD_CFG1_PSC_UCPDCLK_0             (0x1UL << UCPD_CFG1_PSC_UCPDCLK_Pos)  /*!< 0x00020000 */
#define UCPD_CFG1_PSC_UCPDCLK_1             (0x2UL << UCPD_CFG1_PSC_UCPDCLK_Pos)  /*!< 0x00040000 */
#define UCPD_CFG1_PSC_UCPDCLK_2             (0x4UL << UCPD_CFG1_PSC_UCPDCLK_Pos)  /*!< 0x00080000 */
#define UCPD_CFG1_RXORDSETEN_Pos            (20U)
#define UCPD_CFG1_RXORDSETEN_Msk            (0x1FFUL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x1FF00000 */
#define UCPD_CFG1_RXORDSETEN                UCPD_CFG1_RXORDSETEN_Msk              /*!< Receiver ordered set detection enable */
#define UCPD_CFG1_RXORDSETEN_0              (0x001UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x00100000 */
#define UCPD_CFG1_RXORDSETEN_1              (0x002UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x00200000 */
#define UCPD_CFG1_RXORDSETEN_2              (0x004UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x00400000 */
#define UCPD_CFG1_RXORDSETEN_3              (0x008UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x00800000 */
#define UCPD_CFG1_RXORDSETEN_4              (0x010UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x01000000 */
#define UCPD_CFG1_RXORDSETEN_5              (0x020UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x02000000 */
#define UCPD_CFG1_RXORDSETEN_6              (0x040UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x04000000 */
#define UCPD_CFG1_RXORDSETEN_7              (0x080UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x08000000 */
#define UCPD_CFG1_RXORDSETEN_8              (0x100UL << UCPD_CFG1_RXORDSETEN_Pos) /*!< 0x10000000 */
#define UCPD_CFG1_TXDMAEN_Pos               (29U)
#define UCPD_CFG1_TXDMAEN_Msk               (0x1UL << UCPD_CFG1_TXDMAEN_Pos)      /*!< 0x20000000 */
#define UCPD_CFG1_TXDMAEN                   UCPD_CFG1_TXDMAEN_Msk                 /*!< DMA transmission requests enable   */
#define UCPD_CFG1_RXDMAEN_Pos               (30U)
#define UCPD_CFG1_RXDMAEN_Msk               (0x1UL << UCPD_CFG1_RXDMAEN_Pos)      /*!< 0x40000000 */
#define UCPD_CFG1_RXDMAEN                   UCPD_CFG1_RXDMAEN_Msk                 /*!< DMA reception requests enable   */
#define UCPD_CFG1_UCPDEN_Pos                (31U)
#define UCPD_CFG1_UCPDEN_Msk                (0x1UL << UCPD_CFG1_UCPDEN_Pos)       /*!< 0x80000000 */
#define UCPD_CFG1_UCPDEN                    UCPD_CFG1_UCPDEN_Msk                  /*!< USB Power Delivery Block Enable */

/********************  Bits definition for UCPD_CFG2 register  *******************/
#define UCPD_CFG2_RXFILTDIS_Pos             (0U)
#define UCPD_CFG2_RXFILTDIS_Msk             (0x1UL << UCPD_CFG2_RXFILTDIS_Pos)    /*!< 0x00000001 */
#define UCPD_CFG2_RXFILTDIS                 UCPD_CFG2_RXFILTDIS_Msk               /*!< Enables an Rx pre-filter for the BMC decoder */
#define UCPD_CFG2_RXFILT2N3_Pos             (1U)
#define UCPD_CFG2_RXFILT2N3_Msk             (0x1UL << UCPD_CFG2_RXFILT2N3_Pos)    /*!< 0x00000002 */
#define UCPD_CFG2_RXFILT2N3                 UCPD_CFG2_RXFILT2N3_Msk               /*!< Controls the sampling method for an Rx pre-filter for the BMC decode */
#define UCPD_CFG2_FORCECLK_Pos              (2U)
#define UCPD_CFG2_FORCECLK_Msk              (0x1UL << UCPD_CFG2_FORCECLK_Pos)     /*!< 0x00000004 */
#define UCPD_CFG2_FORCECLK                  UCPD_CFG2_FORCECLK_Msk                /*!< Controls forcing of the clock request UCPDCLK_REQ */
#define UCPD_CFG2_WUPEN_Pos                 (3U)
#define UCPD_CFG2_WUPEN_Msk                 (0x1UL << UCPD_CFG2_WUPEN_Pos)        /*!< 0x00000008 */
#define UCPD_CFG2_WUPEN                     UCPD_CFG2_WUPEN_Msk                   /*!< Wakeup from STOP enable */

/********************  Bits definition for UCPD_CR register  ********************/
#define UCPD_CR_TXMODE_Pos                  (0U)
#define UCPD_CR_TXMODE_Msk                  (0x3UL << UCPD_CR_TXMODE_Pos)         /*!< 0x00000003 */
#define UCPD_CR_TXMODE                      UCPD_CR_TXMODE_Msk                    /*!< Type of Tx packet  */
#define UCPD_CR_TXMODE_0                    (0x1UL << UCPD_CR_TXMODE_Pos)         /*!< 0x00000001 */
#define UCPD_CR_TXMODE_1                    (0x2UL << UCPD_CR_TXMODE_Pos)         /*!< 0x00000002 */
#define UCPD_CR_TXSEND_Pos                  (2U)
#define UCPD_CR_TXSEND_Msk                  (0x1UL << UCPD_CR_TXSEND_Pos)         /*!< 0x00000004 */
#define UCPD_CR_TXSEND                      UCPD_CR_TXSEND_Msk                    /*!< Type of Tx packet  */
#define UCPD_CR_TXHRST_Pos                  (3U)
#define UCPD_CR_TXHRST_Msk                  (0x1UL << UCPD_CR_TXHRST_Pos)         /*!< 0x00000008 */
#define UCPD_CR_TXHRST                      UCPD_CR_TXHRST_Msk                    /*!< Command to send a Tx Hard Reset  */
#define UCPD_CR_RXMODE_Pos                  (4U)
#define UCPD_CR_RXMODE_Msk                  (0x1UL << UCPD_CR_RXMODE_Pos)         /*!< 0x00000010 */
#define UCPD_CR_RXMODE                      UCPD_CR_RXMODE_Msk                    /*!< Receiver mode  */
#define UCPD_CR_PHYRXEN_Pos                 (5U)
#define UCPD_CR_PHYRXEN_Msk                 (0x1UL << UCPD_CR_PHYRXEN_Pos)        /*!< 0x00000020 */
#define UCPD_CR_PHYRXEN                     UCPD_CR_PHYRXEN_Msk                   /*!< Controls enable of USB Power Delivery receiver  */
#define UCPD_CR_PHYCCSEL_Pos                (6U)
#define UCPD_CR_PHYCCSEL_Msk                (0x1UL << UCPD_CR_PHYCCSEL_Pos)       /*!< 0x00000040 */
#define UCPD_CR_PHYCCSEL                    UCPD_CR_PHYCCSEL_Msk                  /*!<  */
#define UCPD_CR_ANASUBMODE_Pos              (7U)
#define UCPD_CR_ANASUBMODE_Msk              (0x3UL << UCPD_CR_ANASUBMODE_Pos)     /*!< 0x00000180 */
#define UCPD_CR_ANASUBMODE                  UCPD_CR_ANASUBMODE_Msk                /*!< Analog PHY sub-mode   */
#define UCPD_CR_ANASUBMODE_0                (0x1UL << UCPD_CR_ANASUBMODE_Pos)     /*!< 0x00000080 */
#define UCPD_CR_ANASUBMODE_1                (0x2UL << UCPD_CR_ANASUBMODE_Pos)     /*!< 0x00000100 */
#define UCPD_CR_ANAMODE_Pos                 (9U)
#define UCPD_CR_ANAMODE_Msk                 (0x1UL << UCPD_CR_ANAMODE_Pos)        /*!< 0x00000200 */
#define UCPD_CR_ANAMODE                     UCPD_CR_ANAMODE_Msk                   /*!< Analog PHY working mode   */
#define UCPD_CR_CCENABLE_Pos                (10U)
#define UCPD_CR_CCENABLE_Msk                (0x3UL << UCPD_CR_CCENABLE_Pos)       /*!< 0x00000C00 */
#define UCPD_CR_CCENABLE                    UCPD_CR_CCENABLE_Msk                  /*!<  */
#define UCPD_CR_CCENABLE_0                  (0x1UL << UCPD_CR_CCENABLE_Pos)       /*!< 0x00000400 */
#define UCPD_CR_CCENABLE_1                  (0x2UL << UCPD_CR_CCENABLE_Pos)       /*!< 0x00000800 */
#define UCPD_CR_FRSRXEN_Pos                 (16U)
#define UCPD_CR_FRSRXEN_Msk                 (0x1UL << UCPD_CR_FRSRXEN_Pos)        /*!< 0x00010000 */
#define UCPD_CR_FRSRXEN                     UCPD_CR_FRSRXEN_Msk                   /*!< Enable FRS request detection function */
#define UCPD_CR_FRSTX_Pos                   (17U)
#define UCPD_CR_FRSTX_Msk                   (0x1UL << UCPD_CR_FRSTX_Pos)          /*!< 0x00020000 */
#define UCPD_CR_FRSTX                       UCPD_CR_FRSTX_Msk                     /*!< Signal Fast Role Swap request */
#define UCPD_CR_RDCH_Pos                    (18U)
#define UCPD_CR_RDCH_Msk                    (0x1UL << UCPD_CR_RDCH_Pos)           /*!< 0x00040000 */
#define UCPD_CR_RDCH                        UCPD_CR_RDCH_Msk                      /*!<  */
#define UCPD_CR_CC1TCDIS_Pos                (20U)
#define UCPD_CR_CC1TCDIS_Msk                (0x1UL << UCPD_CR_CC1TCDIS_Pos)       /*!< 0x00100000 */
#define UCPD_CR_CC1TCDIS                    UCPD_CR_CC1TCDIS_Msk                  /*!< The bit allows the Type-C detector for CC0 to be disabled. */
#define UCPD_CR_CC2TCDIS_Pos                (21U)
#define UCPD_CR_CC2TCDIS_Msk                (0x1UL << UCPD_CR_CC2TCDIS_Pos)       /*!< 0x00200000 */
#define UCPD_CR_CC2TCDIS                    UCPD_CR_CC2TCDIS_Msk                  /*!< The bit allows the Type-C detector for CC2 to be disabled. */

/********************  Bits definition for UCPD_IMR register  *******************/
#define UCPD_IMR_TXISIE_Pos                 (0U)
#define UCPD_IMR_TXISIE_Msk                 (0x1UL << UCPD_IMR_TXISIE_Pos)        /*!< 0x00000001 */
#define UCPD_IMR_TXISIE                     UCPD_IMR_TXISIE_Msk                   /*!< Enable TXIS interrupt  */
#define UCPD_IMR_TXMSGDISCIE_Pos            (1U)
#define UCPD_IMR_TXMSGDISCIE_Msk            (0x1UL << UCPD_IMR_TXMSGDISCIE_Pos)   /*!< 0x00000002 */
#define UCPD_IMR_TXMSGDISCIE                UCPD_IMR_TXMSGDISCIE_Msk              /*!< Enable TXMSGDISC interrupt  */
#define UCPD_IMR_TXMSGSENTIE_Pos            (2U)
#define UCPD_IMR_TXMSGSENTIE_Msk            (0x1UL << UCPD_IMR_TXMSGSENTIE_Pos)   /*!< 0x00000004 */
#define UCPD_IMR_TXMSGSENTIE                UCPD_IMR_TXMSGSENTIE_Msk              /*!< Enable TXMSGSENT interrupt  */
#define UCPD_IMR_TXMSGABTIE_Pos             (3U)
#define UCPD_IMR_TXMSGABTIE_Msk             (0x1UL << UCPD_IMR_TXMSGABTIE_Pos)    /*!< 0x00000008 */
#define UCPD_IMR_TXMSGABTIE                 UCPD_IMR_TXMSGABTIE_Msk               /*!< Enable TXMSGABT interrupt  */
#define UCPD_IMR_HRSTDISCIE_Pos             (4U)
#define UCPD_IMR_HRSTDISCIE_Msk             (0x1UL << UCPD_IMR_HRSTDISCIE_Pos)    /*!< 0x00000010 */
#define UCPD_IMR_HRSTDISCIE                 UCPD_IMR_HRSTDISCIE_Msk               /*!< Enable HRSTDISC interrupt  */
#define UCPD_IMR_HRSTSENTIE_Pos             (5U)
#define UCPD_IMR_HRSTSENTIE_Msk             (0x1UL << UCPD_IMR_HRSTSENTIE_Pos)    /*!< 0x00000020 */
#define UCPD_IMR_HRSTSENTIE                 UCPD_IMR_HRSTSENTIE_Msk               /*!< Enable HRSTSENT interrupt  */
#define UCPD_IMR_TXUNDIE_Pos                (6U)
#define UCPD_IMR_TXUNDIE_Msk                (0x1UL << UCPD_IMR_TXUNDIE_Pos)       /*!< 0x00000040 */
#define UCPD_IMR_TXUNDIE                    UCPD_IMR_TXUNDIE_Msk                  /*!< Enable TXUND interrupt  */
#define UCPD_IMR_RXNEIE_Pos                 (8U)
#define UCPD_IMR_RXNEIE_Msk                 (0x1UL << UCPD_IMR_RXNEIE_Pos)        /*!< 0x00000100 */
#define UCPD_IMR_RXNEIE                     UCPD_IMR_RXNEIE_Msk                   /*!< Enable RXNE interrupt  */
#define UCPD_IMR_RXORDDETIE_Pos             (9U)
#define UCPD_IMR_RXORDDETIE_Msk             (0x1UL << UCPD_IMR_RXORDDETIE_Pos)    /*!< 0x00000200 */
#define UCPD_IMR_RXORDDETIE                 UCPD_IMR_RXORDDETIE_Msk               /*!< Enable RXORDDET interrupt  */
#define UCPD_IMR_RXHRSTDETIE_Pos            (10U)
#define UCPD_IMR_RXHRSTDETIE_Msk            (0x1UL << UCPD_IMR_RXHRSTDETIE_Pos)   /*!< 0x00000400 */
#define UCPD_IMR_RXHRSTDETIE                UCPD_IMR_RXHRSTDETIE_Msk              /*!< Enable RXHRSTDET interrupt  */
#define UCPD_IMR_RXOVRIE_Pos                (11U)
#define UCPD_IMR_RXOVRIE_Msk                (0x1UL << UCPD_IMR_RXOVRIE_Pos)       /*!< 0x00000800 */
#define UCPD_IMR_RXOVRIE                    UCPD_IMR_RXOVRIE_Msk                  /*!< Enable RXOVR interrupt  */
#define UCPD_IMR_RXMSGENDIE_Pos             (12U)
#define UCPD_IMR_RXMSGENDIE_Msk             (0x1UL << UCPD_IMR_RXMSGENDIE_Pos)    /*!< 0x00001000 */
#define UCPD_IMR_RXMSGENDIE                 UCPD_IMR_RXMSGENDIE_Msk               /*!< Enable RXMSGEND interrupt  */
#define UCPD_IMR_TYPECEVT1IE_Pos            (14U)
#define UCPD_IMR_TYPECEVT1IE_Msk            (0x1UL << UCPD_IMR_TYPECEVT1IE_Pos)   /*!< 0x00004000 */
#define UCPD_IMR_TYPECEVT1IE                UCPD_IMR_TYPECEVT1IE_Msk              /*!< Enable TYPECEVT1IE interrupt  */
#define UCPD_IMR_TYPECEVT2IE_Pos            (15U)
#define UCPD_IMR_TYPECEVT2IE_Msk            (0x1UL << UCPD_IMR_TYPECEVT2IE_Pos)   /*!< 0x00008000 */
#define UCPD_IMR_TYPECEVT2IE                UCPD_IMR_TYPECEVT2IE_Msk              /*!< Enable TYPECEVT2IE interrupt  */
#define UCPD_IMR_FRSEVTIE_Pos               (20U)
#define UCPD_IMR_FRSEVTIE_Msk               (0x1UL << UCPD_IMR_FRSEVTIE_Pos)      /*!< 0x00100000 */
#define UCPD_IMR_FRSEVTIE                   UCPD_IMR_FRSEVTIE_Msk                 /*!< Fast Role Swap interrupt  */

/********************  Bits definition for UCPD_SR register  ********************/
#define UCPD_SR_TXIS_Pos                    (0U)
#define UCPD_SR_TXIS_Msk                    (0x1UL << UCPD_SR_TXIS_Pos)           /*!< 0x00000001 */
#define UCPD_SR_TXIS                        UCPD_SR_TXIS_Msk                      /*!< Transmit interrupt status  */
#define UCPD_SR_TXMSGDISC_Pos               (1U)
#define UCPD_SR_TXMSGDISC_Msk               (0x1UL << UCPD_SR_TXMSGDISC_Pos)      /*!< 0x00000002 */
#define UCPD_SR_TXMSGDISC                   UCPD_SR_TXMSGDISC_Msk                 /*!< Transmit message discarded interrupt  */
#define UCPD_SR_TXMSGSENT_Pos               (2U)
#define UCPD_SR_TXMSGSENT_Msk               (0x1UL << UCPD_SR_TXMSGSENT_Pos)      /*!< 0x00000004 */
#define UCPD_SR_TXMSGSENT                   UCPD_SR_TXMSGSENT_Msk                 /*!< Transmit message sent interrupt  */
#define UCPD_SR_TXMSGABT_Pos                (3U)
#define UCPD_SR_TXMSGABT_Msk                (0x1UL << UCPD_SR_TXMSGABT_Pos)       /*!< 0x00000008 */
#define UCPD_SR_TXMSGABT                    UCPD_SR_TXMSGABT_Msk                  /*!< Transmit message abort interrupt  */
#define UCPD_SR_HRSTDISC_Pos                (4U)
#define UCPD_SR_HRSTDISC_Msk                (0x1UL << UCPD_SR_HRSTDISC_Pos)       /*!< 0x00000010 */
#define UCPD_SR_HRSTDISC                    UCPD_SR_HRSTDISC_Msk                  /*!< HRST discarded interrupt  */
#define UCPD_SR_HRSTSENT_Pos                (5U)
#define UCPD_SR_HRSTSENT_Msk                (0x1UL << UCPD_SR_HRSTSENT_Pos)       /*!< 0x00000020 */
#define UCPD_SR_HRSTSENT                    UCPD_SR_HRSTSENT_Msk                  /*!< HRST sent interrupt  */
#define UCPD_SR_TXUND_Pos                   (6U)
#define UCPD_SR_TXUND_Msk                   (0x1UL << UCPD_SR_TXUND_Pos)          /*!< 0x00000040 */
#define UCPD_SR_TXUND                       UCPD_SR_TXUND_Msk                     /*!< Tx data underrun condition interrupt  */
#define UCPD_SR_RXNE_Pos                    (8U)
#define UCPD_SR_RXNE_Msk                    (0x1UL << UCPD_SR_RXNE_Pos)           /*!< 0x00000100 */
#define UCPD_SR_RXNE                        UCPD_SR_RXNE_Msk                      /*!< Receive data register not empty interrupt  */
#define UCPD_SR_RXORDDET_Pos                (9U)
#define UCPD_SR_RXORDDET_Msk                (0x1UL << UCPD_SR_RXORDDET_Pos)       /*!< 0x00000200 */
#define UCPD_SR_RXORDDET                    UCPD_SR_RXORDDET_Msk                  /*!< Rx ordered set (4 K-codes) detected interrupt  */
#define UCPD_SR_RXHRSTDET_Pos               (10U)
#define UCPD_SR_RXHRSTDET_Msk               (0x1UL << UCPD_SR_RXHRSTDET_Pos)      /*!< 0x00000400 */
#define UCPD_SR_RXHRSTDET                   UCPD_SR_RXHRSTDET_Msk                 /*!< Rx Hard Reset detect interrupt  */
#define UCPD_SR_RXOVR_Pos                   (11U)
#define UCPD_SR_RXOVR_Msk                   (0x1UL << UCPD_SR_RXOVR_Pos)          /*!< 0x00000800 */
#define UCPD_SR_RXOVR                       UCPD_SR_RXOVR_Msk                     /*!< Rx data overflow interrupt  */
#define UCPD_SR_RXMSGEND_Pos                (12U)
#define UCPD_SR_RXMSGEND_Msk                (0x1UL << UCPD_SR_RXMSGEND_Pos)       /*!< 0x00001000 */
#define UCPD_SR_RXMSGEND                    UCPD_SR_RXMSGEND_Msk                  /*!< Rx message received  */
#define UCPD_SR_RXERR_Pos                   (13U)
#define UCPD_SR_RXERR_Msk                   (0x1UL << UCPD_SR_RXERR_Pos)          /*!< 0x00002000 */
#define UCPD_SR_RXERR                       UCPD_SR_RXERR_Msk                     /*!< RX Error */
#define UCPD_SR_TYPECEVT1_Pos               (14U)
#define UCPD_SR_TYPECEVT1_Msk               (0x1UL << UCPD_SR_TYPECEVT1_Pos)      /*!< 0x00004000 */
#define UCPD_SR_TYPECEVT1                   UCPD_SR_TYPECEVT1_Msk                 /*!< Type C voltage level event on CC1  */
#define UCPD_SR_TYPECEVT2_Pos               (15U)
#define UCPD_SR_TYPECEVT2_Msk               (0x1UL << UCPD_SR_TYPECEVT2_Pos)      /*!< 0x00008000 */
#define UCPD_SR_TYPECEVT2                   UCPD_SR_TYPECEVT2_Msk                 /*!< Type C voltage level event on CC2  */
#define UCPD_SR_TYPEC_VSTATE_CC1_Pos        (16U)
#define UCPD_SR_TYPEC_VSTATE_CC1_Msk        (0x3UL << UCPD_SR_TYPEC_VSTATE_CC1_Pos) /*!< 0x00030000 */
#define UCPD_SR_TYPEC_VSTATE_CC1            UCPD_SR_TYPEC_VSTATE_CC1_Msk            /*!< Status of DC level on CC1 pin  */
#define UCPD_SR_TYPEC_VSTATE_CC1_0          (0x1UL << UCPD_SR_TYPEC_VSTATE_CC1_Pos) /*!< 0x00010000 */
#define UCPD_SR_TYPEC_VSTATE_CC1_1          (0x2UL << UCPD_SR_TYPEC_VSTATE_CC1_Pos) /*!< 0x00020000 */
#define UCPD_SR_TYPEC_VSTATE_CC2_Pos        (18U)
#define UCPD_SR_TYPEC_VSTATE_CC2_Msk        (0x3UL << UCPD_SR_TYPEC_VSTATE_CC2_Pos) /*!< 0x000C0000 */
#define UCPD_SR_TYPEC_VSTATE_CC2            UCPD_SR_TYPEC_VSTATE_CC2_Msk            /*!<Status of DC level on CC2 pin  */
#define UCPD_SR_TYPEC_VSTATE_CC2_0          (0x1UL << UCPD_SR_TYPEC_VSTATE_CC2_Pos) /*!< 0x00040000 */
#define UCPD_SR_TYPEC_VSTATE_CC2_1          (0x2UL << UCPD_SR_TYPEC_VSTATE_CC2_Pos) /*!< 0x00080000 */
#define UCPD_SR_FRSEVT_Pos                  (20U)
#define UCPD_SR_FRSEVT_Msk                  (0x1UL << UCPD_SR_FRSEVT_Pos)         /*!< 0x00100000 */
#define UCPD_SR_FRSEVT                      UCPD_SR_FRSEVT_Msk                    /*!< Fast Role Swap detection event  */

/********************  Bits definition for UCPD_ICR register  *******************/
#define UCPD_ICR_TXMSGDISCCF_Pos            (1U)
#define UCPD_ICR_TXMSGDISCCF_Msk            (0x1UL << UCPD_ICR_TXMSGDISCCF_Pos)   /*!< 0x00000002 */
#define UCPD_ICR_TXMSGDISCCF                UCPD_ICR_TXMSGDISCCF_Msk              /*!< Tx message discarded flag (TXMSGDISC) clear  */
#define UCPD_ICR_TXMSGSENTCF_Pos            (2U)
#define UCPD_ICR_TXMSGSENTCF_Msk            (0x1UL << UCPD_ICR_TXMSGSENTCF_Pos)   /*!< 0x00000004 */
#define UCPD_ICR_TXMSGSENTCF                UCPD_ICR_TXMSGSENTCF_Msk              /*!< Tx message sent flag (TXMSGSENT) clear  */
#define UCPD_ICR_TXMSGABTCF_Pos             (3U)
#define UCPD_ICR_TXMSGABTCF_Msk             (0x1UL << UCPD_ICR_TXMSGABTCF_Pos)    /*!< 0x00000008 */
#define UCPD_ICR_TXMSGABTCF                 UCPD_ICR_TXMSGABTCF_Msk               /*!< Tx message abort flag (TXMSGABT) clear  */
#define UCPD_ICR_HRSTDISCCF_Pos             (4U)
#define UCPD_ICR_HRSTDISCCF_Msk             (0x1UL << UCPD_ICR_HRSTDISCCF_Pos)    /*!< 0x00000010 */
#define UCPD_ICR_HRSTDISCCF                 UCPD_ICR_HRSTDISCCF_Msk               /*!< Hard reset discarded flag (HRSTDISC) clear  */
#define UCPD_ICR_HRSTSENTCF_Pos             (5U)
#define UCPD_ICR_HRSTSENTCF_Msk             (0x1UL << UCPD_ICR_HRSTSENTCF_Pos)    /*!< 0x00000020 */
#define UCPD_ICR_HRSTSENTCF                 UCPD_ICR_HRSTSENTCF_Msk               /*!< Hard reset sent flag (HRSTSENT) clear  */
#define UCPD_ICR_TXUNDCF_Pos                (6U)
#define UCPD_ICR_TXUNDCF_Msk                (0x1UL << UCPD_ICR_TXUNDCF_Pos)       /*!< 0x00000040 */
#define UCPD_ICR_TXUNDCF                    UCPD_ICR_TXUNDCF_Msk                  /*!< Tx underflow flag (TXUND) clear  */
#define UCPD_ICR_RXORDDETCF_Pos             (9U)
#define UCPD_ICR_RXORDDETCF_Msk             (0x1UL << UCPD_ICR_RXORDDETCF_Pos)    /*!< 0x00000200 */
#define UCPD_ICR_RXORDDETCF                 UCPD_ICR_RXORDDETCF_Msk               /*!< Rx ordered set detect flag (RXORDDET) clear  */
#define UCPD_ICR_RXHRSTDETCF_Pos            (10U)
#define UCPD_ICR_RXHRSTDETCF_Msk            (0x1UL << UCPD_ICR_RXHRSTDETCF_Pos)   /*!< 0x00000400 */
#define UCPD_ICR_RXHRSTDETCF                UCPD_ICR_RXHRSTDETCF_Msk              /*!< Rx Hard Reset detected flag (RXHRSTDET) clear  */
#define UCPD_ICR_RXOVRCF_Pos                (11U)
#define UCPD_ICR_RXOVRCF_Msk                (0x1UL << UCPD_ICR_RXOVRCF_Pos)       /*!< 0x00000800 */
#define UCPD_ICR_RXOVRCF                    UCPD_ICR_RXOVRCF_Msk                  /*!< Rx overflow flag (RXOVR) clear  */
#define UCPD_ICR_RXMSGENDCF_Pos             (12U)
#define UCPD_ICR_RXMSGENDCF_Msk             (0x1UL << UCPD_ICR_RXMSGENDCF_Pos)    /*!< 0x00001000 */
#define UCPD_ICR_RXMSGENDCF                 UCPD_ICR_RXMSGENDCF_Msk               /*!< Rx message received flag (RXMSGEND) clear  */
#define UCPD_ICR_TYPECEVT1CF_Pos            (14U)
#define UCPD_ICR_TYPECEVT1CF_Msk            (0x1UL << UCPD_ICR_TYPECEVT1CF_Pos)   /*!< 0x00004000 */
#define UCPD_ICR_TYPECEVT1CF                UCPD_ICR_TYPECEVT1CF_Msk              /*!< TypeC event (CC1) flag (TYPECEVT1) clear  */
#define UCPD_ICR_TYPECEVT2CF_Pos            (15U)
#define UCPD_ICR_TYPECEVT2CF_Msk            (0x1UL << UCPD_ICR_TYPECEVT2CF_Pos)   /*!< 0x00008000 */
#define UCPD_ICR_TYPECEVT2CF                UCPD_ICR_TYPECEVT2CF_Msk              /*!< TypeC event (CC2) flag (TYPECEVT2) clear  */
#define UCPD_ICR_FRSEVTCF_Pos               (20U)
#define UCPD_ICR_FRSEVTCF_Msk               (0x1UL << UCPD_ICR_FRSEVTCF_Pos)      /*!< 0x00100000 */
#define UCPD_ICR_FRSEVTCF                   UCPD_ICR_FRSEVTCF_Msk                 /*!< Fast Role Swap event flag clear  */

/********************  Bits definition for UCPD_TXORDSET register  **************/
#define UCPD_TX_ORDSET_TXORDSET_Pos         (0U)
#define UCPD_TX_ORDSET_TXORDSET_Msk         (0xFFFFFUL << UCPD_TX_ORDSET_TXORDSET_Pos) /*!< 0x000FFFFF */
#define UCPD_TX_ORDSET_TXORDSET             UCPD_TX_ORDSET_TXORDSET_Msk                /*!< Tx Ordered Set */

/********************  Bits definition for UCPD_TXPAYSZ register  ****************/
#define UCPD_TX_PAYSZ_TXPAYSZ_Pos           (0U)
#define UCPD_TX_PAYSZ_TXPAYSZ_Msk           (0x3FFUL << UCPD_TX_PAYSZ_TXPAYSZ_Pos) /*!< 0x000003FF */
#define UCPD_TX_PAYSZ_TXPAYSZ               UCPD_TX_PAYSZ_TXPAYSZ_Msk              /*!< Tx payload size in bytes  */

/********************  Bits definition for UCPD_TXDR register  *******************/
#define UCPD_TXDR_TXDATA_Pos                (0U)
#define UCPD_TXDR_TXDATA_Msk                 (0xFFUL << UCPD_TXDR_TXDATA_Pos)      /*!< 0x000000FF */
#define UCPD_TXDR_TXDATA                    UCPD_TXDR_TXDATA_Msk                   /*!< Tx Data Register */

/********************  Bits definition for UCPD_RXORDSET register  **************/
#define UCPD_RX_ORDSET_RXORDSET_Pos         (0U)
#define UCPD_RX_ORDSET_RXORDSET_Msk         (0x7UL << UCPD_RX_ORDSET_RXORDSET_Pos)  /*!< 0x00000007 */
#define UCPD_RX_ORDSET_RXORDSET             UCPD_RX_ORDSET_RXORDSET_Msk             /*!< Rx Ordered Set Code detected  */
#define UCPD_RX_ORDSET_RXORDSET_0           (0x1UL << UCPD_RX_ORDSET_RXORDSET_Pos)  /*!< 0x00000001 */
#define UCPD_RX_ORDSET_RXORDSET_1           (0x2UL << UCPD_RX_ORDSET_RXORDSET_Pos)  /*!< 0x00000002 */
#define UCPD_RX_ORDSET_RXORDSET_2           (0x4UL << UCPD_RX_ORDSET_RXORDSET_Pos)  /*!< 0x00000004 */
#define UCPD_RX_ORDSET_RXSOP3OF4_Pos        (3U)
#define UCPD_RX_ORDSET_RXSOP3OF4_Msk        (0x1UL << UCPD_RX_ORDSET_RXSOP3OF4_Pos) /*!< 0x00000008 */
#define UCPD_RX_ORDSET_RXSOP3OF4            UCPD_RX_ORDSET_RXSOP3OF4_Msk            /*!< Rx Ordered Set Debug indication */
#define UCPD_RX_ORDSET_RXSOPKINVALID_Pos    (4U)
#define UCPD_RX_ORDSET_RXSOPKINVALID_Msk    (0x7UL << UCPD_RX_ORDSET_RXSOPKINVALID_Pos) /*!< 0x00000070 */
#define UCPD_RX_ORDSET_RXSOPKINVALID        UCPD_RX_ORDSET_RXSOPKINVALID_Msk            /*!< Rx Ordered Set corrupted K-Codes (Debug) */

/********************  Bits definition for UCPD_RXPAYSZ register  ****************/
#define UCPD_RX_PAYSZ_RXPAYSZ_Pos           (0U)
#define UCPD_RX_PAYSZ_RXPAYSZ_Msk           (0x3FFUL << UCPD_RX_PAYSZ_RXPAYSZ_Pos) /*!< 0x000003FF */
#define UCPD_RX_PAYSZ_RXPAYSZ               UCPD_RX_PAYSZ_RXPAYSZ_Msk              /*!< Rx payload size in bytes  */

/********************  Bits definition for UCPD_RXDR register  *******************/
#define UCPD_RXDR_RXDATA_Pos                (0U)
#define UCPD_RXDR_RXDATA_Msk                (0xFFUL << UCPD_RXDR_RXDATA_Pos)      /*!< 0x000000FF */
#define UCPD_RXDR_RXDATA                    UCPD_RXDR_RXDATA_Msk                  /*!< 8-bit receive data  */

/********************  Bits definition for UCPD_RXORDEXT1 register  **************/
#define UCPD_RX_ORDEXT1_RXSOPX1_Pos         (0U)
#define UCPD_RX_ORDEXT1_RXSOPX1_Msk         (0xFFFFFUL << UCPD_RX_ORDEXT1_RXSOPX1_Pos) /*!< 0x000FFFFF */
#define UCPD_RX_ORDEXT1_RXSOPX1             UCPD_RX_ORDEXT1_RXSOPX1_Msk                /*!< RX Ordered Set Extension Register 1 */

/********************  Bits definition for UCPD_RXORDEXT2 register  **************/
#define UCPD_RX_ORDEXT2_RXSOPX2_Pos         (0U)
#define UCPD_RX_ORDEXT2_RXSOPX2_Msk         (0xFFFFFUL << UCPD_RX_ORDEXT2_RXSOPX2_Pos) /*!< 0x000FFFFF */
#define UCPD_RX_ORDEXT2_RXSOPX2             UCPD_RX_ORDEXT2_RXSOPX2_Msk                /*!< RX Ordered Set Extension Register 1 */


/** @addtogroup Exported_macros
  * @{
  */

/******************************* ADC Instances ********************************/
#define IS_ADC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == ADC1)

#define IS_ADC_COMMON_INSTANCE(INSTANCE) ((INSTANCE) == ADC1_COMMON)

/******************************* AES Instances ********************************/
#define IS_AES_ALL_INSTANCE(INSTANCE) ((INSTANCE) == AES)

/****************************** CEC Instances *********************************/
#define IS_CEC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == CEC)

/******************************** COMP Instances ******************************/
#define IS_COMP_ALL_INSTANCE(INSTANCE) (((INSTANCE) == COMP1) || \
                                        ((INSTANCE) == COMP2))

#define IS_COMP_COMMON_INSTANCE(COMMON_INSTANCE) ((COMMON_INSTANCE) == COMP12_COMMON)

/******************** COMP Instances with window mode capability **************/
#define IS_COMP_WINDOWMODE_INSTANCE(INSTANCE) ((INSTANCE) == COMP2)

/******************************* CRC Instances ********************************/
#define IS_CRC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == CRC)

/******************************* DAC Instances ********************************/
#define IS_DAC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == DAC1)

/******************************** DMA Instances *******************************/
#define IS_DMA_ALL_INSTANCE(INSTANCE) (((INSTANCE) == DMA1_Channel1) || \
                                       ((INSTANCE) == DMA1_Channel2) || \
                                       ((INSTANCE) == DMA1_Channel3) || \
                                       ((INSTANCE) == DMA1_Channel4) || \
                                       ((INSTANCE) == DMA1_Channel5) || \
                                       ((INSTANCE) == DMA1_Channel6) || \
                                       ((INSTANCE) == DMA1_Channel7))

/******************************** DMAMUX Instances ****************************/
#define IS_DMAMUX_ALL_INSTANCE(INSTANCE) ((INSTANCE) == DMAMUX1)

#define IS_DMAMUX_REQUEST_GEN_ALL_INSTANCE(INSTANCE) (((INSTANCE) == DMAMUX1_RequestGenerator0) || \
                                                      ((INSTANCE) == DMAMUX1_RequestGenerator1) || \
                                                      ((INSTANCE) == DMAMUX1_RequestGenerator2) || \
                                                      ((INSTANCE) == DMAMUX1_RequestGenerator3))

/******************************* GPIO Instances *******************************/
#define IS_GPIO_ALL_INSTANCE(INSTANCE) (((INSTANCE) == GPIOA) || \
                                        ((INSTANCE) == GPIOB) || \
                                        ((INSTANCE) == GPIOC) || \
                                        ((INSTANCE) == GPIOD) || \
                                        ((INSTANCE) == GPIOF))

/******************************* GPIO AF Instances ****************************/
#define IS_GPIO_AF_INSTANCE(INSTANCE)   IS_GPIO_ALL_INSTANCE(INSTANCE)

/**************************** GPIO Lock Instances *****************************/
#define IS_GPIO_LOCK_INSTANCE(INSTANCE) (((INSTANCE) == GPIOA) || \
                                         ((INSTANCE) == GPIOB) || \
                                         ((INSTANCE) == GPIOC))

/******************************** I2C Instances *******************************/
#define IS_I2C_ALL_INSTANCE(INSTANCE) (((INSTANCE) == I2C1) || \
                                       ((INSTANCE) == I2C2))

/******************************* RNG Instances ********************************/
#define IS_RNG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == RNG)

/****************************** RTC Instances *********************************/
#define IS_RTC_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == RTC)

/****************************** SMBUS Instances *******************************/
#define IS_SMBUS_ALL_INSTANCE(INSTANCE) (((INSTANCE) == I2C1))

/****************************** WAKEUP_FROMSTOP Instances *******************************/
#define IS_I2C_WAKEUP_FROMSTOP_INSTANCE(INSTANCE) (((INSTANCE) == I2C1))

/******************************** SPI Instances *******************************/
#define IS_SPI_ALL_INSTANCE(INSTANCE) (((INSTANCE) == SPI1) || \
                                       ((INSTANCE) == SPI2))
/******************************** SPI Instances *******************************/
#define IS_I2S_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == SPI1)

/****************** LPTIM Instances : All supported instances *****************/
#define IS_LPTIM_INSTANCE(INSTANCE)     (((INSTANCE) == LPTIM1) || \
                                         ((INSTANCE) == LPTIM2))

/****************** LPTIM Instances : All supported instances *****************/
#define IS_LPTIM_ENCODER_INTERFACE_INSTANCE(INSTANCE) ((INSTANCE) == LPTIM1)

/****************** TIM Instances : All supported instances *******************/
#define IS_TIM_INSTANCE(INSTANCE)       (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3)   || \
                                         ((INSTANCE) == TIM6)   || \
                                         ((INSTANCE) == TIM7)   || \
                                         ((INSTANCE) == TIM14)  || \
                                         ((INSTANCE) == TIM15)  || \
                                         ((INSTANCE) == TIM16)  || \
                                         ((INSTANCE) == TIM17))

/****************** TIM Instances : supporting 32 bits counter ****************/
#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) ((INSTANCE) == TIM2)

/****************** TIM Instances : supporting the break function *************/
#define IS_TIM_BREAK_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1)    || \
                                            ((INSTANCE) == TIM15)   || \
                                            ((INSTANCE) == TIM16)   || \
                                            ((INSTANCE) == TIM17))

/************** TIM Instances : supporting Break source selection *************/
#define IS_TIM_BREAKSOURCE_INSTANCE(INSTANCE) (((INSTANCE) == TIM1)   || \
                                               ((INSTANCE) == TIM15)  || \
                                               ((INSTANCE) == TIM16)  || \
                                               ((INSTANCE) == TIM17))

/****************** TIM Instances : supporting 2 break inputs *****************/
#define IS_TIM_BKIN2_INSTANCE(INSTANCE)    ((INSTANCE) == TIM1)

/************* TIM Instances : at least 1 capture/compare channel *************/
#define IS_TIM_CC1_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3)   || \
                                         ((INSTANCE) == TIM14)  || \
                                         ((INSTANCE) == TIM15)  || \
                                         ((INSTANCE) == TIM16)  || \
                                         ((INSTANCE) == TIM17))

/************ TIM Instances : at least 2 capture/compare channels *************/
#define IS_TIM_CC2_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3)   || \
                                         ((INSTANCE) == TIM15))

/************ TIM Instances : at least 3 capture/compare channels *************/
#define IS_TIM_CC3_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3))

/************ TIM Instances : at least 4 capture/compare channels *************/
#define IS_TIM_CC4_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3))

/****************** TIM Instances : at least 5 capture/compare channels *******/
#define IS_TIM_CC5_INSTANCE(INSTANCE)   ((INSTANCE) == TIM1)

/****************** TIM Instances : at least 6 capture/compare channels *******/
#define IS_TIM_CC6_INSTANCE(INSTANCE)   ((INSTANCE) == TIM1)

/************ TIM Instances : DMA requests generation (TIMx_DIER.COMDE) *******/
#define IS_TIM_CCDMA_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1)   || \
                                            ((INSTANCE) == TIM15)  || \
                                            ((INSTANCE) == TIM16)  || \
                                            ((INSTANCE) == TIM17))

/****************** TIM Instances : DMA requests generation (TIMx_DIER.UDE) ***/
#define IS_TIM_DMA_INSTANCE(INSTANCE)      (((INSTANCE) == TIM1)   || \
                                            ((INSTANCE) == TIM2)   || \
                                            ((INSTANCE) == TIM3)   || \
                                            ((INSTANCE) == TIM6)   || \
                                            ((INSTANCE) == TIM7)   || \
                                            ((INSTANCE) == TIM15)  || \
                                            ((INSTANCE) == TIM16)  || \
                                            ((INSTANCE) == TIM17))

/************ TIM Instances : DMA requests generation (TIMx_DIER.CCxDE) *******/
#define IS_TIM_DMA_CC_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)   || \
                                            ((INSTANCE) == TIM2)   || \
                                            ((INSTANCE) == TIM3)   || \
                                            ((INSTANCE) == TIM14)  || \
                                            ((INSTANCE) == TIM15)  || \
                                            ((INSTANCE) == TIM16)  || \
                                            ((INSTANCE) == TIM17))

/******************** TIM Instances : DMA burst feature ***********************/
#define IS_TIM_DMABURST_INSTANCE(INSTANCE) (((INSTANCE) == TIM1)   || \
                                            ((INSTANCE) == TIM2)   || \
                                            ((INSTANCE) == TIM3)   || \
                                            ((INSTANCE) == TIM15)  || \
                                            ((INSTANCE) == TIM16)  || \
                                            ((INSTANCE) == TIM17))

/******************* TIM Instances : output(s) available **********************/
#define IS_TIM_CCX_INSTANCE(INSTANCE, CHANNEL) \
    ((((INSTANCE) == TIM1) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4) ||          \
      ((CHANNEL) == TIM_CHANNEL_5) ||          \
      ((CHANNEL) == TIM_CHANNEL_6)))           \
     ||                                        \
     (((INSTANCE) == TIM2) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
     ||                                        \
     (((INSTANCE) == TIM3) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
     ||                                        \
     (((INSTANCE) == TIM14) &&                 \
     (((CHANNEL) == TIM_CHANNEL_1)))           \
     ||                                        \
     (((INSTANCE) == TIM15) &&                 \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2)))           \
     ||                                        \
     (((INSTANCE) == TIM16) &&                 \
     (((CHANNEL) == TIM_CHANNEL_1)))           \
     ||                                        \
     (((INSTANCE) == TIM17) &&                 \
      (((CHANNEL) == TIM_CHANNEL_1))))

/****************** TIM Instances : supporting complementary output(s) ********/
#define IS_TIM_CCXN_INSTANCE(INSTANCE, CHANNEL) \
   ((((INSTANCE) == TIM1) &&                    \
     (((CHANNEL) == TIM_CHANNEL_1) ||           \
      ((CHANNEL) == TIM_CHANNEL_2) ||           \
      ((CHANNEL) == TIM_CHANNEL_3)))            \
    ||                                          \
    (((INSTANCE) == TIM15) &&                   \
     ((CHANNEL) == TIM_CHANNEL_1))              \
    ||                                          \
    (((INSTANCE) == TIM16) &&                   \
     ((CHANNEL) == TIM_CHANNEL_1))              \
    ||                                          \
    (((INSTANCE) == TIM17) &&                   \
     ((CHANNEL) == TIM_CHANNEL_1)))

/****************** TIM Instances : supporting clock division *****************/
#define IS_TIM_CLOCK_DIVISION_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)    || \
                                                    ((INSTANCE) == TIM2)    || \
                                                    ((INSTANCE) == TIM3)    || \
                                                    ((INSTANCE) == TIM14)   || \
                                                    ((INSTANCE) == TIM15)   || \
                                                    ((INSTANCE) == TIM16)   || \
                                                    ((INSTANCE) == TIM17))

/****** TIM Instances : supporting external clock mode 1 for ETRF input *******/
#define IS_TIM_CLOCKSOURCE_ETRMODE1_INSTANCE(INSTANCE) (((INSTANCE) == TIM1) || \
                                                        ((INSTANCE) == TIM2) || \
                                                        ((INSTANCE) == TIM3))

/****** TIM Instances : supporting external clock mode 2 for ETRF input *******/
#define IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE(INSTANCE) (((INSTANCE) == TIM1) || \
                                                        ((INSTANCE) == TIM2) || \
                                                        ((INSTANCE) == TIM3))

/****************** TIM Instances : supporting external clock mode 1 for TIX inputs*/
#define IS_TIM_CLOCKSOURCE_TIX_INSTANCE(INSTANCE)      (((INSTANCE) == TIM1) || \
                                                        ((INSTANCE) == TIM2) || \
                                                        ((INSTANCE) == TIM3) || \
                                                        ((INSTANCE) == TIM15))

/****************** TIM Instances : supporting internal trigger inputs(ITRX) *******/
#define IS_TIM_CLOCKSOURCE_ITRX_INSTANCE(INSTANCE)     (((INSTANCE) == TIM1) || \
                                                        ((INSTANCE) == TIM2) || \
                                                        ((INSTANCE) == TIM3) || \
                                                        ((INSTANCE) == TIM15))

/****************** TIM Instances : supporting combined 3-phase PWM mode ******/
#define IS_TIM_COMBINED3PHASEPWM_INSTANCE(INSTANCE)    ((INSTANCE) == TIM1)

/****************** TIM Instances : supporting commutation event generation ***/
#define IS_TIM_COMMUTATION_EVENT_INSTANCE(INSTANCE) (((INSTANCE) == TIM1)   || \
                                                     ((INSTANCE) == TIM15)  || \
                                                     ((INSTANCE) == TIM16)  || \
                                                     ((INSTANCE) == TIM17))

/****************** TIM Instances : supporting counting mode selection ********/
#define IS_TIM_COUNTER_MODE_SELECT_INSTANCE(INSTANCE)  (((INSTANCE) == TIM1) || \
                                                        ((INSTANCE) == TIM2) || \
                                                        ((INSTANCE) == TIM3))

/****************** TIM Instances : supporting encoder interface **************/
#define IS_TIM_ENCODER_INTERFACE_INSTANCE(INSTANCE)  (((INSTANCE) == TIM1)  || \
                                                      ((INSTANCE) == TIM2)  || \
                                                      ((INSTANCE) == TIM3))

/****************** TIM Instances : supporting Hall sensor interface **********/
#define IS_TIM_HALL_SENSOR_INTERFACE_INSTANCE(INSTANCE) (((INSTANCE) == TIM1)   || \
                                                         ((INSTANCE) == TIM2)   || \
                                                         ((INSTANCE) == TIM3))

/**************** TIM Instances : external trigger input available ************/
#define IS_TIM_ETR_INSTANCE(INSTANCE)      (((INSTANCE) == TIM1)  || \
                                            ((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM3))

/************* TIM Instances : supporting ETR source selection ***************/
#define IS_TIM_ETRSEL_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1)  || \
                                             ((INSTANCE) == TIM2)  || \
                                             ((INSTANCE) == TIM3))

/****** TIM Instances : Master mode available (TIMx_CR2.MMS available )********/
#define IS_TIM_MASTER_INSTANCE(INSTANCE)   (((INSTANCE) == TIM1)  || \
                                            ((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM3)  || \
                                            ((INSTANCE) == TIM6)  || \
                                            ((INSTANCE) == TIM7)  || \
                                            ((INSTANCE) == TIM15))

/*********** TIM Instances : Slave mode available (TIMx_SMCR available )*******/
#define IS_TIM_SLAVE_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1)  || \
                                            ((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM3)  || \
                                            ((INSTANCE) == TIM15))

/****************** TIM Instances : supporting OCxREF clear *******************/
#define IS_TIM_OCXREF_CLEAR_INSTANCE(INSTANCE)        (((INSTANCE) == TIM1) || \
                                                       ((INSTANCE) == TIM2) || \
                                                       ((INSTANCE) == TIM3))

/****************** TIM Instances : remapping capability **********************/
#define IS_TIM_REMAP_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1)  || \
                                            ((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM3))

/****************** TIM Instances : supporting repetition counter *************/
#define IS_TIM_REPETITION_COUNTER_INSTANCE(INSTANCE)  (((INSTANCE) == TIM1)  || \
                                                       ((INSTANCE) == TIM15) || \
                                                       ((INSTANCE) == TIM16) || \
                                                       ((INSTANCE) == TIM17))

/****************** TIM Instances : supporting synchronization ****************/
#define IS_TIM_SYNCHRO_INSTANCE(INSTANCE)  IS_TIM_MASTER_INSTANCE(INSTANCE)

/****************** TIM Instances : supporting ADC triggering through TRGO2 ***/
#define IS_TIM_TRGO2_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1))

/******************* TIM Instances : Timer input XOR function *****************/
#define IS_TIM_XOR_INSTANCE(INSTANCE)      (((INSTANCE) == TIM1)   || \
                                            ((INSTANCE) == TIM2)   || \
                                            ((INSTANCE) == TIM3)   || \
                                            ((INSTANCE) == TIM15))

/******************* TIM Instances : Timer input selection ********************/
#define IS_TIM_TISEL_INSTANCE(INSTANCE) (((INSTANCE) == TIM1)   || \
                                         ((INSTANCE) == TIM2)   || \
                                         ((INSTANCE) == TIM3)   || \
                                         ((INSTANCE) == TIM14)  || \
                                         ((INSTANCE) == TIM15)  || \
                                         ((INSTANCE) == TIM16)  || \
                                         ((INSTANCE) == TIM17))

/************ TIM Instances : Advanced timers  ********************************/
#define IS_TIM_ADVANCED_INSTANCE(INSTANCE)    (((INSTANCE) == TIM1))

/******************** UART Instances : Asynchronous mode **********************/
#define IS_UART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2) || \
                                    ((INSTANCE) == USART3) || \
                                    ((INSTANCE) == USART4))


/******************** USART Instances : Synchronous mode **********************/
#define IS_USART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                     ((INSTANCE) == USART2) || \
                                     ((INSTANCE) == USART3) || \
                                     ((INSTANCE) == USART4))

/****************** UART Instances : Hardware Flow control ********************/
#define IS_UART_HWFLOW_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                           ((INSTANCE) == USART2) || \
                                           ((INSTANCE) == USART3) || \
                                           ((INSTANCE) == USART4) || \
                                           ((INSTANCE) == LPUART1))


/********************* USART Instances : Smard card mode ***********************/
#define IS_SMARTCARD_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                         ((INSTANCE) == USART2))

/****************** UART Instances : Auto Baud Rate detection ****************/
#define IS_USART_AUTOBAUDRATE_DETECTION_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                                            ((INSTANCE) == USART2))

/******************** UART Instances : Half-Duplex mode **********************/
#define IS_UART_HALFDUPLEX_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                                 ((INSTANCE) == USART2) || \
                                                 ((INSTANCE) == USART3) || \
                                                 ((INSTANCE) == USART4) || \
                                                 ((INSTANCE) == LPUART1))

/******************** UART Instances : LIN mode **********************/
#define IS_UART_LIN_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                          ((INSTANCE) == USART2))

/******************** UART Instances : Wake-up from Stop mode **********************/
#define IS_UART_WAKEUP_FROMSTOP_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                                      ((INSTANCE) == USART2) || \
                                                      ((INSTANCE) == LPUART1))

/****************** UART Instances : Driver Enable *****************/
#define IS_UART_DRIVER_ENABLE_INSTANCE(INSTANCE)     (((INSTANCE) == USART1) || \
                                                      ((INSTANCE) == USART2) || \
                                                      ((INSTANCE) == USART3) || \
                                                      ((INSTANCE) == USART4) || \
                                                      ((INSTANCE) == LPUART1))


/****************** UART Instances : SPI Slave selection mode ***************/
#define IS_UART_SPI_SLAVE_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                              ((INSTANCE) == USART2) || \
                                              ((INSTANCE) == USART3) || \
                                              ((INSTANCE) == USART4))


/****************** UART Instances : Driver Enable *****************/
#define IS_UART_FIFO_INSTANCE(INSTANCE)     (((INSTANCE) == USART1) || \
                                             ((INSTANCE) == USART2) || \
                                             ((INSTANCE) == LPUART1))

/*********************** UART Instances : IRDA mode ***************************/
#define IS_IRDA_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2))

/******************** LPUART Instance *****************************************/
#define IS_LPUART_INSTANCE(INSTANCE)    ((INSTANCE) == LPUART1)

/****************************** IWDG Instances ********************************/
#define IS_IWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == IWDG)

/****************************** WWDG Instances ********************************/
#define IS_WWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == WWDG)

/****************************** UCPD Instances ********************************/
#define IS_UCPD_ALL_INSTANCE(INSTANCE)  (((INSTANCE) == UCPD1) || \
                                         ((INSTANCE) == UCPD2))

/**
  * @}
  */

 /**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* STM32G081xx_H */

/**
  * @}
  */

  /**
  * @}
  */

/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
