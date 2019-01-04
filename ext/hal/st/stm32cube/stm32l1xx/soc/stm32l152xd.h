/**
  ******************************************************************************
  * @file    stm32l152xd.h
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-M3 Device Peripheral Access Layer Header File. 
  *          This file contains all the peripheral register's definitions, bits 
  *          definitions and memory mapping for STM32L1xx devices.            
  *            
  *          This file contains:
  *           - Data structures and the address mapping for all peripherals
  *           - Peripheral's registers declarations and bits definition
  *           - Macros to access peripheral’s registers hardware
  *  
  ******************************************************************************
  * @attention
  *
  * <h2><center>&copy; COPYRIGHT(c) 2017 STMicroelectronics</center></h2>
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

/** @addtogroup stm32l152xd
  * @{
  */
    
#ifndef __STM32L152xD_H
#define __STM32L152xD_H

#ifdef __cplusplus
 extern "C" {
#endif 
  

  /** @addtogroup Configuration_section_for_CMSIS
  * @{
  */
/**
  * @brief Configuration of the Cortex-M3 Processor and Core Peripherals 
 */
#define __CM3_REV                 0x200U /*!< Cortex-M3 Revision r2p0                  */
#define __MPU_PRESENT             1U     /*!< STM32L1xx provides MPU                          */
#define __NVIC_PRIO_BITS          4U     /*!< STM32L1xx uses 4 Bits for the Priority Levels    */
#define __Vendor_SysTickConfig    0U     /*!< Set to 1 if different SysTick Config is used */
 
/**
  * @}
  */
   
/** @addtogroup Peripheral_interrupt_number_definition
  * @{
  */

/**
 * @brief STM32L1xx Interrupt Number Definition, according to the selected device 
 *        in @ref Library_configuration_section 
 */

 /*!< Interrupt Number Definition */
typedef enum
{
/******  Cortex-M3 Processor Exceptions Numbers ******************************************************/
  NonMaskableInt_IRQn         = -14,    /*!< 2 Non Maskable Interrupt                                */
  HardFault_IRQn              = -13,    /*!< 3 Cortex-M3 Hard Fault Interrupt                        */
  MemoryManagement_IRQn       = -12,    /*!< 4 Cortex-M3 Memory Management Interrupt                 */
  BusFault_IRQn               = -11,    /*!< 5 Cortex-M3 Bus Fault Interrupt                         */
  UsageFault_IRQn             = -10,    /*!< 6 Cortex-M3 Usage Fault Interrupt                       */
  SVC_IRQn                    = -5,     /*!< 11 Cortex-M3 SV Call Interrupt                          */
  DebugMonitor_IRQn           = -4,     /*!< 12 Cortex-M3 Debug Monitor Interrupt                    */
  PendSV_IRQn                 = -2,     /*!< 14 Cortex-M3 Pend SV Interrupt                          */
  SysTick_IRQn                = -1,     /*!< 15 Cortex-M3 System Tick Interrupt                      */

/******  STM32L specific Interrupt Numbers ***********************************************************/
  WWDG_IRQn                   = 0,      /*!< Window WatchDog Interrupt                               */
  PVD_IRQn                    = 1,      /*!< PVD through EXTI Line detection Interrupt               */
  TAMPER_STAMP_IRQn           = 2,      /*!< Tamper and TimeStamp interrupts through the EXTI line   */
  RTC_WKUP_IRQn               = 3,      /*!< RTC Wakeup Timer through EXTI Line Interrupt            */
  FLASH_IRQn                  = 4,      /*!< FLASH global Interrupt                                  */
  RCC_IRQn                    = 5,      /*!< RCC global Interrupt                                    */
  EXTI0_IRQn                  = 6,      /*!< EXTI Line0 Interrupt                                    */
  EXTI1_IRQn                  = 7,      /*!< EXTI Line1 Interrupt                                    */
  EXTI2_IRQn                  = 8,      /*!< EXTI Line2 Interrupt                                    */
  EXTI3_IRQn                  = 9,      /*!< EXTI Line3 Interrupt                                    */
  EXTI4_IRQn                  = 10,     /*!< EXTI Line4 Interrupt                                    */
  DMA1_Channel1_IRQn          = 11,     /*!< DMA1 Channel 1 global Interrupt                         */
  DMA1_Channel2_IRQn          = 12,     /*!< DMA1 Channel 2 global Interrupt                         */
  DMA1_Channel3_IRQn          = 13,     /*!< DMA1 Channel 3 global Interrupt                         */
  DMA1_Channel4_IRQn          = 14,     /*!< DMA1 Channel 4 global Interrupt                         */
  DMA1_Channel5_IRQn          = 15,     /*!< DMA1 Channel 5 global Interrupt                         */
  DMA1_Channel6_IRQn          = 16,     /*!< DMA1 Channel 6 global Interrupt                         */
  DMA1_Channel7_IRQn          = 17,     /*!< DMA1 Channel 7 global Interrupt                         */
  ADC1_IRQn                   = 18,     /*!< ADC1 global Interrupt                                   */
  USB_HP_IRQn                 = 19,     /*!< USB High Priority Interrupt                             */
  USB_LP_IRQn                 = 20,     /*!< USB Low Priority Interrupt                              */
  DAC_IRQn                    = 21,     /*!< DAC Interrupt                                           */
  COMP_IRQn                   = 22,     /*!< Comparator through EXTI Line Interrupt                  */
  EXTI9_5_IRQn                = 23,     /*!< External Line[9:5] Interrupts                           */
  LCD_IRQn                    = 24,     /*!< LCD Interrupt                                           */
  TIM9_IRQn                   = 25,     /*!< TIM9 global Interrupt                                   */
  TIM10_IRQn                  = 26,     /*!< TIM10 global Interrupt                                  */
  TIM11_IRQn                  = 27,     /*!< TIM11 global Interrupt                                  */
  TIM2_IRQn                   = 28,     /*!< TIM2 global Interrupt                                   */
  TIM3_IRQn                   = 29,     /*!< TIM3 global Interrupt                                   */
  TIM4_IRQn                   = 30,     /*!< TIM4 global Interrupt                                   */
  I2C1_EV_IRQn                = 31,     /*!< I2C1 Event Interrupt                                    */
  I2C1_ER_IRQn                = 32,     /*!< I2C1 Error Interrupt                                    */
  I2C2_EV_IRQn                = 33,     /*!< I2C2 Event Interrupt                                    */
  I2C2_ER_IRQn                = 34,     /*!< I2C2 Error Interrupt                                    */
  SPI1_IRQn                   = 35,     /*!< SPI1 global Interrupt                                   */
  SPI2_IRQn                   = 36,     /*!< SPI2 global Interrupt                                   */
  USART1_IRQn                 = 37,     /*!< USART1 global Interrupt                                 */
  USART2_IRQn                 = 38,     /*!< USART2 global Interrupt                                 */
  USART3_IRQn                 = 39,     /*!< USART3 global Interrupt                                 */
  EXTI15_10_IRQn              = 40,     /*!< External Line[15:10] Interrupts                         */
  RTC_Alarm_IRQn              = 41,     /*!< RTC Alarm through EXTI Line Interrupt                   */
  USB_FS_WKUP_IRQn            = 42,     /*!< USB FS WakeUp from suspend through EXTI Line Interrupt  */
  TIM6_IRQn                   = 43,     /*!< TIM6 global Interrupt                                   */
  TIM7_IRQn                   = 44,     /*!< TIM7 global Interrupt                                   */
  SDIO_IRQn                   = 45,     /*!< SDIO global Interrupt                                   */
  TIM5_IRQn                   = 46,     /*!< TIM5 global Interrupt                                   */
  SPI3_IRQn                   = 47,     /*!< SPI3 global Interrupt                                   */
  UART4_IRQn                  = 48,     /*!< UART4 global Interrupt                                  */
  UART5_IRQn                  = 49,     /*!< UART5 global Interrupt                                  */
  DMA2_Channel1_IRQn          = 50,     /*!< DMA2 Channel 1 global Interrupt                         */
  DMA2_Channel2_IRQn          = 51,     /*!< DMA2 Channel 2 global Interrupt                         */
  DMA2_Channel3_IRQn          = 52,     /*!< DMA2 Channel 3 global Interrupt                         */
  DMA2_Channel4_IRQn          = 53,     /*!< DMA2 Channel 4 global Interrupt                         */
  DMA2_Channel5_IRQn          = 54,     /*!< DMA2 Channel 5 global Interrupt                         */
  COMP_ACQ_IRQn               = 56      /*!< Comparator Channel Acquisition global Interrupt         */
} IRQn_Type;

/**
  * @}
  */

#include "core_cm3.h"
#include "system_stm32l1xx.h"
#include <stdint.h>

/** @addtogroup Peripheral_registers_structures
  * @{
  */   

/** 
  * @brief Analog to Digital Converter
  */

typedef struct
{
  __IO uint32_t SR;           /*!< ADC status register,                         Address offset: 0x00 */
  __IO uint32_t CR1;          /*!< ADC control register 1,                      Address offset: 0x04 */
  __IO uint32_t CR2;          /*!< ADC control register 2,                      Address offset: 0x08 */
  __IO uint32_t SMPR1;        /*!< ADC sample time register 1,                  Address offset: 0x0C */
  __IO uint32_t SMPR2;        /*!< ADC sample time register 2,                  Address offset: 0x10 */
  __IO uint32_t SMPR3;        /*!< ADC sample time register 3,                  Address offset: 0x14 */
  __IO uint32_t JOFR1;        /*!< ADC injected channel data offset register 1, Address offset: 0x18 */
  __IO uint32_t JOFR2;        /*!< ADC injected channel data offset register 2, Address offset: 0x1C */
  __IO uint32_t JOFR3;        /*!< ADC injected channel data offset register 3, Address offset: 0x20 */
  __IO uint32_t JOFR4;        /*!< ADC injected channel data offset register 4, Address offset: 0x24 */
  __IO uint32_t HTR;          /*!< ADC watchdog higher threshold register,      Address offset: 0x28 */
  __IO uint32_t LTR;          /*!< ADC watchdog lower threshold register,       Address offset: 0x2C */
  __IO uint32_t SQR1;         /*!< ADC regular sequence register 1,             Address offset: 0x30 */
  __IO uint32_t SQR2;         /*!< ADC regular sequence register 2,             Address offset: 0x34 */
  __IO uint32_t SQR3;         /*!< ADC regular sequence register 3,             Address offset: 0x38 */
  __IO uint32_t SQR4;         /*!< ADC regular sequence register 4,             Address offset: 0x3C */
  __IO uint32_t SQR5;         /*!< ADC regular sequence register 5,             Address offset: 0x40 */
  __IO uint32_t JSQR;         /*!< ADC injected sequence register,              Address offset: 0x44 */
  __IO uint32_t JDR1;         /*!< ADC injected data register 1,                Address offset: 0x48 */
  __IO uint32_t JDR2;         /*!< ADC injected data register 2,                Address offset: 0x4C */
  __IO uint32_t JDR3;         /*!< ADC injected data register 3,                Address offset: 0x50 */
  __IO uint32_t JDR4;         /*!< ADC injected data register 4,                Address offset: 0x54 */
  __IO uint32_t DR;           /*!< ADC regular data register,                   Address offset: 0x58 */
  __IO uint32_t SMPR0;        /*!< ADC sample time register 0,                  Address offset: 0x5C */
} ADC_TypeDef;

typedef struct
{
  __IO uint32_t CSR;          /*!< ADC common status register,                  Address offset: ADC1 base address + 0x300 */
  __IO uint32_t CCR;          /*!< ADC common control register,                 Address offset: ADC1 base address + 0x304 */
} ADC_Common_TypeDef;

/** 
  * @brief Comparator 
  */

typedef struct
{
  __IO uint32_t CSR;         /*!< COMP control and status register, Address offset: 0x00 */
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
  __IO uint32_t DR;           /*!< CRC Data register,                           Address offset: 0x00 */
  __IO uint8_t  IDR;          /*!< CRC Independent data register,               Address offset: 0x04 */
  uint8_t       RESERVED0;    /*!< Reserved,                                    Address offset: 0x05 */
  uint16_t      RESERVED1;    /*!< Reserved,                                    Address offset: 0x06 */ 
  __IO uint32_t CR;           /*!< CRC Control register,                        Address offset: 0x08 */ 
} CRC_TypeDef;

/** 
  * @brief Digital to Analog Converter
  */

typedef struct
{
  __IO uint32_t CR;           /*!< DAC control register,                                     Address offset: 0x00 */
  __IO uint32_t SWTRIGR;      /*!< DAC software trigger register,                            Address offset: 0x04 */
  __IO uint32_t DHR12R1;      /*!< DAC channel1 12-bit right-aligned data holding register,  Address offset: 0x08 */
  __IO uint32_t DHR12L1;      /*!< DAC channel1 12-bit left aligned data holding register,   Address offset: 0x0C */
  __IO uint32_t DHR8R1;       /*!< DAC channel1 8-bit right aligned data holding register,   Address offset: 0x10 */
  __IO uint32_t DHR12R2;      /*!< DAC channel2 12-bit right aligned data holding register,  Address offset: 0x14 */
  __IO uint32_t DHR12L2;      /*!< DAC channel2 12-bit left aligned data holding register,   Address offset: 0x18 */
  __IO uint32_t DHR8R2;       /*!< DAC channel2 8-bit right-aligned data holding register,   Address offset: 0x1C */
  __IO uint32_t DHR12RD;      /*!< Dual DAC 12-bit right-aligned data holding register,      Address offset: 0x20 */
  __IO uint32_t DHR12LD;      /*!< DUAL DAC 12-bit left aligned data holding register,       Address offset: 0x24 */
  __IO uint32_t DHR8RD;       /*!< DUAL DAC 8-bit right aligned data holding register,       Address offset: 0x28 */
  __IO uint32_t DOR1;         /*!< DAC channel1 data output register,                        Address offset: 0x2C */
  __IO uint32_t DOR2;         /*!< DAC channel2 data output register,                        Address offset: 0x30 */
  __IO uint32_t SR;           /*!< DAC status register,                                      Address offset: 0x34 */
} DAC_TypeDef;

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
  __IO uint32_t CCR;          /*!< DMA channel x configuration register        */
  __IO uint32_t CNDTR;        /*!< DMA channel x number of data register       */
  __IO uint32_t CPAR;         /*!< DMA channel x peripheral address register   */
  __IO uint32_t CMAR;         /*!< DMA channel x memory address register       */
} DMA_Channel_TypeDef;

typedef struct
{
  __IO uint32_t ISR;          /*!< DMA interrupt status register,               Address offset: 0x00 */
  __IO uint32_t IFCR;         /*!< DMA interrupt flag clear register,           Address offset: 0x04 */
} DMA_TypeDef;

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
} EXTI_TypeDef;

/** 
  * @brief FLASH Registers
  */
typedef struct
{
  __IO uint32_t ACR;          /*!< Access control register,                     Address offset: 0x00 */
  __IO uint32_t PECR;         /*!< Program/erase control register,              Address offset: 0x04 */
  __IO uint32_t PDKEYR;       /*!< Power down key register,                     Address offset: 0x08 */
  __IO uint32_t PEKEYR;       /*!< Program/erase key register,                  Address offset: 0x0c */
  __IO uint32_t PRGKEYR;      /*!< Program memory key register,                 Address offset: 0x10 */
  __IO uint32_t OPTKEYR;      /*!< Option byte key register,                    Address offset: 0x14 */
  __IO uint32_t SR;           /*!< Status register,                             Address offset: 0x18 */
  __IO uint32_t OBR;          /*!< Option byte register,                        Address offset: 0x1c */
  __IO uint32_t WRPR1;        /*!< Write protection register 1,                 Address offset: 0x20 */
  uint32_t   RESERVED[23];    /*!< Reserved,                                    Address offset: 0x24 */
  __IO uint32_t WRPR2;        /*!< Write protection register 2,                 Address offset: 0x80 */
  __IO uint32_t WRPR3;        /*!< Write protection register 3,                 Address offset: 0x84 */
} FLASH_TypeDef;

/** 
  * @brief Option Bytes Registers
  */
typedef struct
{
  __IO uint32_t RDP;              /*!< Read protection register,               Address offset: 0x00 */
  __IO uint32_t USER;             /*!< user register,                          Address offset: 0x04 */
  __IO uint32_t WRP01;            /*!< write protection register 0 1,          Address offset: 0x08 */
  __IO uint32_t WRP23;            /*!< write protection register 2 3,          Address offset: 0x0C */
  __IO uint32_t WRP45;            /*!< write protection register 4 5,          Address offset: 0x10 */
  __IO uint32_t WRP67;            /*!< write protection register 6 7,          Address offset: 0x14 */
  __IO uint32_t WRP89;            /*!< write protection register 8 9,          Address offset: 0x18 */
  __IO uint32_t WRP1011;          /*!< write protection register 10 11,        Address offset: 0x1C */
} OB_TypeDef;

/** 
  * @brief Operational Amplifier (OPAMP)
  */
typedef struct
{
  __IO uint32_t CSR;          /*!< OPAMP control and status register,                 Address offset: 0x00 */
  __IO uint32_t OTR;          /*!< OPAMP offset trimming register for normal mode,    Address offset: 0x04 */ 
  __IO uint32_t LPOTR;        /*!< OPAMP offset trimming register for low power mode, Address offset: 0x08 */
} OPAMP_TypeDef;

typedef struct
{
  __IO uint32_t CSR;          /*!< OPAMP control and status register, used for bits common to several OPAMP instances,              Address offset: 0x00 */
  __IO uint32_t OTR;          /*!< OPAMP offset trimming register for normal mode, used for bits common to several OPAMP instances, Address offset: 0x04 */ 
} OPAMP_Common_TypeDef;

/** 
  * @brief Flexible Static Memory Controller
  */

typedef struct
{
  __IO uint32_t BTCR[8];      /*!< NOR/PSRAM chip-select control register(BCR) and chip-select timing register(BTR), Address offset: 0x00-1C */
} FSMC_Bank1_TypeDef; 

/** 
  * @brief Flexible Static Memory Controller Bank1E
  */
  
typedef struct
{
  __IO uint32_t BWTR[7];      /*!< NOR/PSRAM write timing registers, Address offset: 0x104-0x11C */
} FSMC_Bank1E_TypeDef;

/** 
  * @brief General Purpose IO
  */

typedef struct
{
  __IO uint32_t MODER;        /*!< GPIO port mode register,                     Address offset: 0x00      */
  __IO uint32_t OTYPER;       /*!< GPIO port output type register,              Address offset: 0x04      */
  __IO uint32_t OSPEEDR;      /*!< GPIO port output speed register,             Address offset: 0x08      */
  __IO uint32_t PUPDR;        /*!< GPIO port pull-up/pull-down register,        Address offset: 0x0C      */
  __IO uint32_t IDR;          /*!< GPIO port input data register,               Address offset: 0x10      */
  __IO uint32_t ODR;          /*!< GPIO port output data register,              Address offset: 0x14      */
  __IO uint32_t BSRR;         /*!< GPIO port bit set/reset registerBSRR,        Address offset: 0x18      */
  __IO uint32_t LCKR;         /*!< GPIO port configuration lock register,       Address offset: 0x1C      */
  __IO uint32_t AFR[2];       /*!< GPIO alternate function register,            Address offset: 0x20-0x24 */
  __IO uint32_t BRR;          /*!< GPIO bit reset register,                     Address offset: 0x28      */
} GPIO_TypeDef;

/** 
  * @brief SysTem Configuration
  */

typedef struct
{
  __IO uint32_t MEMRMP;       /*!< SYSCFG memory remap register,                      Address offset: 0x00      */
  __IO uint32_t PMC;          /*!< SYSCFG peripheral mode configuration register,     Address offset: 0x04      */
  __IO uint32_t EXTICR[4];    /*!< SYSCFG external interrupt configuration registers, Address offset: 0x08-0x14 */
} SYSCFG_TypeDef;

/** 
  * @brief Inter-integrated Circuit Interface
  */

typedef struct
{
  __IO uint32_t CR1;          /*!< I2C Control register 1,                      Address offset: 0x00 */
  __IO uint32_t CR2;          /*!< I2C Control register 2,                      Address offset: 0x04 */
  __IO uint32_t OAR1;         /*!< I2C Own address register 1,                  Address offset: 0x08 */
  __IO uint32_t OAR2;         /*!< I2C Own address register 2,                  Address offset: 0x0C */
  __IO uint32_t DR;           /*!< I2C Data register,                           Address offset: 0x10 */
  __IO uint32_t SR1;          /*!< I2C Status register 1,                       Address offset: 0x14 */
  __IO uint32_t SR2;          /*!< I2C Status register 2,                       Address offset: 0x18 */
  __IO uint32_t CCR;          /*!< I2C Clock control register,                  Address offset: 0x1C */
  __IO uint32_t TRISE;        /*!< I2C TRISE register,                          Address offset: 0x20 */
} I2C_TypeDef;

/** 
  * @brief Independent WATCHDOG
  */

typedef struct
{
  __IO uint32_t KR;           /*!< Key register,                                Address offset: 0x00 */
  __IO uint32_t PR;           /*!< Prescaler register,                          Address offset: 0x04 */
  __IO uint32_t RLR;          /*!< Reload register,                             Address offset: 0x08 */
  __IO uint32_t SR;           /*!< Status register,                             Address offset: 0x0C */
} IWDG_TypeDef;

/** 
  * @brief LCD
  */

typedef struct
{
  __IO uint32_t CR;        /*!< LCD control register,                           Address offset: 0x00 */
  __IO uint32_t FCR;       /*!< LCD frame control register,                     Address offset: 0x04 */
  __IO uint32_t SR;        /*!< LCD status register,                            Address offset: 0x08 */
  __IO uint32_t CLR;       /*!< LCD clear register,                             Address offset: 0x0C */
  uint32_t RESERVED;       /*!< Reserved,                                       Address offset: 0x10 */
  __IO uint32_t RAM[16];   /*!< LCD display memory,                             Address offset: 0x14-0x50 */
} LCD_TypeDef;

/** 
  * @brief Power Control
  */

typedef struct
{
  __IO uint32_t CR;   /*!< PWR power control register,                          Address offset: 0x00 */
  __IO uint32_t CSR;  /*!< PWR power control/status register,                   Address offset: 0x04 */
} PWR_TypeDef;

/** 
  * @brief Reset and Clock Control
  */

typedef struct
{
  __IO uint32_t CR;            /*!< RCC clock control register,                                   Address offset: 0x00 */
  __IO uint32_t ICSCR;         /*!< RCC Internal clock sources calibration register,              Address offset: 0x04 */
  __IO uint32_t CFGR;          /*!< RCC Clock configuration register,                             Address offset: 0x08 */
  __IO uint32_t CIR;           /*!< RCC Clock interrupt register,                                 Address offset: 0x0C */
  __IO uint32_t AHBRSTR;       /*!< RCC AHB peripheral reset register,                            Address offset: 0x10 */
  __IO uint32_t APB2RSTR;      /*!< RCC APB2 peripheral reset register,                           Address offset: 0x14 */
  __IO uint32_t APB1RSTR;      /*!< RCC APB1 peripheral reset register,                           Address offset: 0x18 */
  __IO uint32_t AHBENR;        /*!< RCC AHB peripheral clock enable register,                     Address offset: 0x1C */
  __IO uint32_t APB2ENR;       /*!< RCC APB2 peripheral clock enable register,                    Address offset: 0x20 */
  __IO uint32_t APB1ENR;       /*!< RCC APB1 peripheral clock enable register,                    Address offset: 0x24 */
  __IO uint32_t AHBLPENR;      /*!< RCC AHB peripheral clock enable in low power mode register,   Address offset: 0x28 */
  __IO uint32_t APB2LPENR;     /*!< RCC APB2 peripheral clock enable in low power mode register,  Address offset: 0x2C */
  __IO uint32_t APB1LPENR;     /*!< RCC APB1 peripheral clock enable in low power mode register,  Address offset: 0x30 */
  __IO uint32_t CSR;           /*!< RCC Control/status register,                                  Address offset: 0x34 */
} RCC_TypeDef;

/** 
  * @brief Routing Interface 
  */

typedef struct
{
  __IO uint32_t ICR;        /*!< RI input capture register,                     Address offset: 0x00 */
  __IO uint32_t ASCR1;      /*!< RI analog switches control register,       Address offset: 0x04 */
  __IO uint32_t ASCR2;      /*!< RI analog switch control register 2,        Address offset: 0x08 */
  __IO uint32_t HYSCR1;     /*!< RI hysteresis control register,                Address offset: 0x0C */
  __IO uint32_t HYSCR2;     /*!< RI Hysteresis control register,               Address offset: 0x10 */
  __IO uint32_t HYSCR3;     /*!< RI Hysteresis control register,               Address offset: 0x14 */
  __IO uint32_t HYSCR4;     /*!< RI Hysteresis control register,               Address offset: 0x18 */
  __IO uint32_t ASMR1;      /*!< RI Analog switch mode register 1,         Address offset: 0x1C */
  __IO uint32_t CMR1;       /*!< RI Channel mask register 1,                   Address offset: 0x20 */
  __IO uint32_t CICR1;      /*!< RI Channel Iden for capture register 1,  Address offset: 0x24 */
  __IO uint32_t ASMR2;      /*!< RI Analog switch mode register 2,         Address offset: 0x28 */
  __IO uint32_t CMR2;       /*!< RI Channel mask register 2,                   Address offset: 0x2C */
  __IO uint32_t CICR2;      /*!< RI Channel Iden for capture register 2,  Address offset: 0x30 */
  __IO uint32_t ASMR3;      /*!< RI Analog switch mode register 3,         Address offset: 0x34 */
  __IO uint32_t CMR3;       /*!< RI Channel mask register 3,                   Address offset: 0x38 */
  __IO uint32_t CICR3;      /*!< RI Channel Iden for capture register 3,  Address offset: 0x3C */
  __IO uint32_t ASMR4;      /*!< RI Analog switch mode register 4,         Address offset: 0x40 */
  __IO uint32_t CMR4;       /*!< RI Channel mask register 4,                   Address offset: 0x44 */
  __IO uint32_t CICR4;      /*!< RI Channel Iden for capture register 4,  Address offset: 0x48 */
  __IO uint32_t ASMR5;      /*!< RI Analog switch mode register 5,         Address offset: 0x4C */
  __IO uint32_t CMR5;       /*!< RI Channel mask register 5,                   Address offset: 0x50 */
  __IO uint32_t CICR5;      /*!< RI Channel Iden for capture register 5,  Address offset: 0x54 */
} RI_TypeDef;

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
  __IO uint32_t CALIBR;     /*!< RTC calibration register,                                  Address offset: 0x18 */
  __IO uint32_t ALRMAR;     /*!< RTC alarm A register,                                      Address offset: 0x1C */
  __IO uint32_t ALRMBR;     /*!< RTC alarm B register,                                      Address offset: 0x20 */
  __IO uint32_t WPR;        /*!< RTC write protection register,                             Address offset: 0x24 */
  __IO uint32_t SSR;        /*!< RTC sub second register,                                   Address offset: 0x28 */
  __IO uint32_t SHIFTR;     /*!< RTC shift control register,                                Address offset: 0x2C */
  __IO uint32_t TSTR;       /*!< RTC time stamp time register,                              Address offset: 0x30 */
  __IO uint32_t TSDR;       /*!< RTC time stamp date register,                              Address offset: 0x34 */
  __IO uint32_t TSSSR;      /*!< RTC time-stamp sub second register,                        Address offset: 0x38 */
  __IO uint32_t CALR;       /*!< RRTC calibration register,                                 Address offset: 0x3C */
  __IO uint32_t TAFCR;      /*!< RTC tamper and alternate function configuration register,  Address offset: 0x40 */
  __IO uint32_t ALRMASSR;   /*!< RTC alarm A sub second register,                           Address offset: 0x44 */
  __IO uint32_t ALRMBSSR;   /*!< RTC alarm B sub second register,                           Address offset: 0x48 */
  uint32_t RESERVED7;       /*!< Reserved, 0x4C                                                                  */
  __IO uint32_t BKP0R;      /*!< RTC backup register 0,                                     Address offset: 0x50 */
  __IO uint32_t BKP1R;      /*!< RTC backup register 1,                                     Address offset: 0x54 */
  __IO uint32_t BKP2R;      /*!< RTC backup register 2,                                     Address offset: 0x58 */
  __IO uint32_t BKP3R;      /*!< RTC backup register 3,                                     Address offset: 0x5C */
  __IO uint32_t BKP4R;      /*!< RTC backup register 4,                                     Address offset: 0x60 */
  __IO uint32_t BKP5R;      /*!< RTC backup register 5,                                     Address offset: 0x64 */
  __IO uint32_t BKP6R;      /*!< RTC backup register 6,                                     Address offset: 0x68 */
  __IO uint32_t BKP7R;      /*!< RTC backup register 7,                                     Address offset: 0x6C */
  __IO uint32_t BKP8R;      /*!< RTC backup register 8,                                     Address offset: 0x70 */
  __IO uint32_t BKP9R;      /*!< RTC backup register 9,                                     Address offset: 0x74 */
  __IO uint32_t BKP10R;     /*!< RTC backup register 10,                                    Address offset: 0x78 */
  __IO uint32_t BKP11R;     /*!< RTC backup register 11,                                    Address offset: 0x7C */
  __IO uint32_t BKP12R;     /*!< RTC backup register 12,                                    Address offset: 0x80 */
  __IO uint32_t BKP13R;     /*!< RTC backup register 13,                                    Address offset: 0x84 */
  __IO uint32_t BKP14R;     /*!< RTC backup register 14,                                    Address offset: 0x88 */
  __IO uint32_t BKP15R;     /*!< RTC backup register 15,                                    Address offset: 0x8C */
  __IO uint32_t BKP16R;     /*!< RTC backup register 16,                                    Address offset: 0x90 */
  __IO uint32_t BKP17R;     /*!< RTC backup register 17,                                    Address offset: 0x94 */
  __IO uint32_t BKP18R;     /*!< RTC backup register 18,                                    Address offset: 0x98 */
  __IO uint32_t BKP19R;     /*!< RTC backup register 19,                                    Address offset: 0x9C */
  __IO uint32_t BKP20R;     /*!< RTC backup register 20,                                    Address offset: 0xA0 */
  __IO uint32_t BKP21R;     /*!< RTC backup register 21,                                    Address offset: 0xA4 */
  __IO uint32_t BKP22R;     /*!< RTC backup register 22,                                    Address offset: 0xA8 */
  __IO uint32_t BKP23R;     /*!< RTC backup register 23,                                    Address offset: 0xAC */
  __IO uint32_t BKP24R;     /*!< RTC backup register 24,                                    Address offset: 0xB0 */
  __IO uint32_t BKP25R;     /*!< RTC backup register 25,                                    Address offset: 0xB4 */
  __IO uint32_t BKP26R;     /*!< RTC backup register 26,                                    Address offset: 0xB8 */
  __IO uint32_t BKP27R;     /*!< RTC backup register 27,                                    Address offset: 0xBC */
  __IO uint32_t BKP28R;     /*!< RTC backup register 28,                                    Address offset: 0xC0 */
  __IO uint32_t BKP29R;     /*!< RTC backup register 29,                                    Address offset: 0xC4 */
  __IO uint32_t BKP30R;     /*!< RTC backup register 30,                                    Address offset: 0xC8 */
  __IO uint32_t BKP31R;     /*!< RTC backup register 31,                                    Address offset: 0xCC */
} RTC_TypeDef;

/** 
  * @brief SD host Interface
  */

typedef struct
{
  __IO uint32_t POWER;          /*!< SDIO power control register,    Address offset: 0x00 */
  __IO uint32_t CLKCR;          /*!< SDI clock control register,     Address offset: 0x04 */
  __IO uint32_t ARG;            /*!< SDIO argument register,         Address offset: 0x08 */
  __IO uint32_t CMD;            /*!< SDIO command register,          Address offset: 0x0C */
  __I uint32_t  RESPCMD;  /*!< SDIO command response register, Address offset: 0x10 */
  __I uint32_t  RESP1;    /*!< SDIO response 1 register,       Address offset: 0x14 */
  __I uint32_t  RESP2;    /*!< SDIO response 2 register,       Address offset: 0x18 */
  __I uint32_t  RESP3;    /*!< SDIO response 3 register,       Address offset: 0x1C */
  __I uint32_t  RESP4;    /*!< SDIO response 4 register,       Address offset: 0x20 */
  __IO uint32_t DTIMER;         /*!< SDIO data timer register,       Address offset: 0x24 */
  __IO uint32_t DLEN;           /*!< SDIO data length register,      Address offset: 0x28 */
  __IO uint32_t DCTRL;          /*!< SDIO data control register,     Address offset: 0x2C */
  __I uint32_t  DCOUNT;   /*!< SDIO data counter register,     Address offset: 0x30 */
  __I uint32_t  STA;      /*!< SDIO status register,           Address offset: 0x34 */
  __IO uint32_t ICR;            /*!< SDIO interrupt clear register,  Address offset: 0x38 */
  __IO uint32_t MASK;           /*!< SDIO mask register,             Address offset: 0x3C */
  uint32_t      RESERVED0[2];   /*!< Reserved, 0x40-0x44                                  */
  __I uint32_t  FIFOCNT; /*!< SDIO FIFO counter register,     Address offset: 0x48 */
  uint32_t      RESERVED1[13];  /*!< Reserved, 0x4C-0x7C                                  */
  __IO uint32_t FIFO;           /*!< SDIO data FIFO register,        Address offset: 0x80 */
} SDIO_TypeDef;

/** 
  * @brief Serial Peripheral Interface
  */

typedef struct
{
  __IO uint32_t CR1;        /*!< SPI Control register 1 (not used in I2S mode),      Address offset: 0x00 */
  __IO uint32_t CR2;        /*!< SPI Control register 2,                             Address offset: 0x04 */
  __IO uint32_t SR;         /*!< SPI Status register,                                Address offset: 0x08 */
  __IO uint32_t DR;         /*!< SPI data register,                                  Address offset: 0x0C */
  __IO uint32_t CRCPR;      /*!< SPI CRC polynomial register (not used in I2S mode), Address offset: 0x10 */
  __IO uint32_t RXCRCR;     /*!< SPI Rx CRC register (not used in I2S mode),         Address offset: 0x14 */
  __IO uint32_t TXCRCR;     /*!< SPI Tx CRC register (not used in I2S mode),         Address offset: 0x18 */
  __IO uint32_t I2SCFGR;    /*!< SPI_I2S configuration register,                     Address offset: 0x1C */
  __IO uint32_t I2SPR;      /*!< SPI_I2S prescaler register,                         Address offset: 0x20 */
} SPI_TypeDef;

/** 
  * @brief TIM
  */
typedef struct
{
  __IO uint32_t CR1;          /*!< TIM control register 1,              Address offset: 0x00 */
  __IO uint32_t CR2;          /*!< TIM control register 2,              Address offset: 0x04 */
  __IO uint32_t SMCR;         /*!< TIM slave Mode Control register,     Address offset: 0x08 */
  __IO uint32_t DIER;         /*!< TIM DMA/interrupt enable register,   Address offset: 0x0C */
  __IO uint32_t SR;           /*!< TIM status register,                 Address offset: 0x10 */
  __IO uint32_t EGR;          /*!< TIM event generation register,       Address offset: 0x14 */
  __IO uint32_t CCMR1;        /*!< TIM capture/compare mode register 1, Address offset: 0x18 */
  __IO uint32_t CCMR2;        /*!< TIM capture/compare mode register 2, Address offset: 0x1C */
  __IO uint32_t CCER;         /*!< TIM capture/compare enable register, Address offset: 0x20 */
  __IO uint32_t CNT;          /*!< TIM counter register,                Address offset: 0x24 */
  __IO uint32_t PSC;          /*!< TIM prescaler register,              Address offset: 0x28 */
  __IO uint32_t ARR;          /*!< TIM auto-reload register,            Address offset: 0x2C */
  uint32_t      RESERVED12;   /*!< Reserved, 0x30                                            */    
  __IO uint32_t CCR1;         /*!< TIM capture/compare register 1,      Address offset: 0x34 */    
  __IO uint32_t CCR2;         /*!< TIM capture/compare register 2,      Address offset: 0x38 */    
  __IO uint32_t CCR3;         /*!< TIM capture/compare register 3,      Address offset: 0x3C */
  __IO uint32_t CCR4;         /*!< TIM capture/compare register 4,      Address offset: 0x40 */
  uint32_t      RESERVED17;   /*!< Reserved, 0x44                                            */ 
  __IO uint32_t DCR;          /*!< TIM DMA control register,            Address offset: 0x48 */
  __IO uint32_t DMAR;         /*!< TIM DMA address for full transfer,   Address offset: 0x4C */
  __IO uint32_t OR;           /*!< TIM option register,                 Address offset: 0x50 */
} TIM_TypeDef;
/** 
  * @brief Universal Synchronous Asynchronous Receiver Transmitter
  */
 
typedef struct
{
  __IO uint32_t SR;         /*!< USART Status register,                   Address offset: 0x00 */
  __IO uint32_t DR;         /*!< USART Data register,                     Address offset: 0x04 */
  __IO uint32_t BRR;        /*!< USART Baud rate register,                Address offset: 0x08 */
  __IO uint32_t CR1;        /*!< USART Control register 1,                Address offset: 0x0C */
  __IO uint32_t CR2;        /*!< USART Control register 2,                Address offset: 0x10 */
  __IO uint32_t CR3;        /*!< USART Control register 3,                Address offset: 0x14 */
  __IO uint32_t GTPR;       /*!< USART Guard time and prescaler register, Address offset: 0x18 */
} USART_TypeDef;

/** 
  * @brief Universal Serial Bus Full Speed Device
  */
  
typedef struct
{
  __IO uint16_t EP0R;            /*!< USB Endpoint 0 register,                Address offset: 0x00 */ 
  __IO uint16_t RESERVED0;       /*!< Reserved */     
  __IO uint16_t EP1R;            /*!< USB Endpoint 1 register,                Address offset: 0x04 */
  __IO uint16_t RESERVED1;       /*!< Reserved */       
  __IO uint16_t EP2R;            /*!< USB Endpoint 2 register,                Address offset: 0x08 */
  __IO uint16_t RESERVED2;       /*!< Reserved */       
  __IO uint16_t EP3R;            /*!< USB Endpoint 3 register,                Address offset: 0x0C */ 
  __IO uint16_t RESERVED3;       /*!< Reserved */       
  __IO uint16_t EP4R;            /*!< USB Endpoint 4 register,                Address offset: 0x10 */
  __IO uint16_t RESERVED4;       /*!< Reserved */       
  __IO uint16_t EP5R;            /*!< USB Endpoint 5 register,                Address offset: 0x14 */
  __IO uint16_t RESERVED5;       /*!< Reserved */       
  __IO uint16_t EP6R;            /*!< USB Endpoint 6 register,                Address offset: 0x18 */
  __IO uint16_t RESERVED6;       /*!< Reserved */       
  __IO uint16_t EP7R;            /*!< USB Endpoint 7 register,                Address offset: 0x1C */
  __IO uint16_t RESERVED7[17];   /*!< Reserved */     
  __IO uint16_t CNTR;            /*!< Control register,                       Address offset: 0x40 */
  __IO uint16_t RESERVED8;       /*!< Reserved */       
  __IO uint16_t ISTR;            /*!< Interrupt status register,              Address offset: 0x44 */
  __IO uint16_t RESERVED9;       /*!< Reserved */       
  __IO uint16_t FNR;             /*!< Frame number register,                  Address offset: 0x48 */
  __IO uint16_t RESERVEDA;       /*!< Reserved */       
  __IO uint16_t DADDR;           /*!< Device address register,                Address offset: 0x4C */
  __IO uint16_t RESERVEDB;       /*!< Reserved */       
  __IO uint16_t BTABLE;          /*!< Buffer Table address register,          Address offset: 0x50 */
  __IO uint16_t RESERVEDC;       /*!< Reserved */       
} USB_TypeDef;

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
  * @brief Universal Serial Bus Full Speed Device
  */
/** 
  * @}
  */
  
/** @addtogroup Peripheral_memory_map
  * @{
  */

#define FLASH_BASE            ((uint32_t)0x08000000U)              /*!< FLASH base address in the alias region */
#define FLASH_EEPROM_BASE     ((uint32_t)(FLASH_BASE + 0x80000U))  /*!< FLASH EEPROM base address in the alias region */
#define SRAM_BASE             ((uint32_t)0x20000000U)              /*!< SRAM base address in the alias region */
#define PERIPH_BASE           ((uint32_t)0x40000000U)              /*!< Peripheral base address in the alias region */
#define FSMC_BASE             ((uint32_t)0x60000000U)              /*!< FSMC base address */
#define FSMC_R_BASE           ((uint32_t)0xA0000000U)              /*!< FSMC registers base address */
#define SRAM_BB_BASE          ((uint32_t)0x22000000U)              /*!< SRAM base address in the bit-band region */
#define PERIPH_BB_BASE        ((uint32_t)0x42000000U)              /*!< Peripheral base address in the bit-band region */
#define FLASH_BANK2_BASE      ((uint32_t)0x08030000U)              /*!< FLASH BANK2 base address in the alias region */
#define FLASH_BANK1_END       ((uint32_t)0x0802FFFFU)              /*!< Program end FLASH BANK1 address */
#define FLASH_BANK2_END       ((uint32_t)0x0805FFFFU)              /*!< Program end FLASH BANK2 address */
#define FLASH_EEPROM_END      ((uint32_t)0x08082FFFU)              /*!< FLASH EEPROM end address  (12KB) */

/*!< Peripheral memory map */
#define APB1PERIPH_BASE       PERIPH_BASE
#define APB2PERIPH_BASE       (PERIPH_BASE + 0x00010000U)
#define AHBPERIPH_BASE        (PERIPH_BASE + 0x00020000U)

/*!< APB1 peripherals */
#define TIM2_BASE             (APB1PERIPH_BASE + 0x00000000U)
#define TIM3_BASE             (APB1PERIPH_BASE + 0x00000400U)
#define TIM4_BASE             (APB1PERIPH_BASE + 0x00000800U)
#define TIM5_BASE             (APB1PERIPH_BASE + 0x00000C00U)
#define TIM6_BASE             (APB1PERIPH_BASE + 0x00001000U)
#define TIM7_BASE             (APB1PERIPH_BASE + 0x00001400U)
#define LCD_BASE              (APB1PERIPH_BASE + 0x00002400U)
#define RTC_BASE              (APB1PERIPH_BASE + 0x00002800U)
#define WWDG_BASE             (APB1PERIPH_BASE + 0x00002C00U)
#define IWDG_BASE             (APB1PERIPH_BASE + 0x00003000U)
#define SPI2_BASE             (APB1PERIPH_BASE + 0x00003800U)
#define SPI3_BASE             (APB1PERIPH_BASE + 0x00003C00U)
#define USART2_BASE           (APB1PERIPH_BASE + 0x00004400U)
#define USART3_BASE           (APB1PERIPH_BASE + 0x00004800U)
#define UART4_BASE            (APB1PERIPH_BASE + 0x00004C00U)
#define UART5_BASE            (APB1PERIPH_BASE + 0x00005000U)
#define I2C1_BASE             (APB1PERIPH_BASE + 0x00005400U)
#define I2C2_BASE             (APB1PERIPH_BASE + 0x00005800U)

/* USB device FS */
#define USB_BASE              (APB1PERIPH_BASE + 0x00005C00U) /*!< USB_IP Peripheral Registers base address */
#define USB_PMAADDR           (APB1PERIPH_BASE + 0x00006000U) /*!< USB_IP Packet Memory Area base address */

/* USB device FS SRAM */
#define PWR_BASE              (APB1PERIPH_BASE + 0x00007000U)
#define DAC_BASE              (APB1PERIPH_BASE + 0x00007400U)
#define COMP_BASE             (APB1PERIPH_BASE + 0x00007C00U)
#define RI_BASE               (APB1PERIPH_BASE + 0x00007C04U)
#define OPAMP_BASE            (APB1PERIPH_BASE + 0x00007C5CU)

/*!< APB2 peripherals */
#define SYSCFG_BASE           (APB2PERIPH_BASE + 0x00000000U)
#define EXTI_BASE             (APB2PERIPH_BASE + 0x00000400U)
#define TIM9_BASE             (APB2PERIPH_BASE + 0x00000800U)
#define TIM10_BASE            (APB2PERIPH_BASE + 0x00000C00U)
#define TIM11_BASE            (APB2PERIPH_BASE + 0x00001000U)
#define ADC1_BASE             (APB2PERIPH_BASE + 0x00002400U)
#define ADC_BASE              (APB2PERIPH_BASE + 0x00002700U)
#define SDIO_BASE             (APB2PERIPH_BASE + 0x00002C00U)
#define SPI1_BASE             (APB2PERIPH_BASE + 0x00003000U)
#define USART1_BASE           (APB2PERIPH_BASE + 0x00003800U)

/*!< AHB peripherals */
#define GPIOA_BASE            (AHBPERIPH_BASE + 0x00000000U)
#define GPIOB_BASE            (AHBPERIPH_BASE + 0x00000400U)
#define GPIOC_BASE            (AHBPERIPH_BASE + 0x00000800U)
#define GPIOD_BASE            (AHBPERIPH_BASE + 0x00000C00U)
#define GPIOE_BASE            (AHBPERIPH_BASE + 0x00001000U)
#define GPIOH_BASE            (AHBPERIPH_BASE + 0x00001400U)
#define GPIOF_BASE            (AHBPERIPH_BASE + 0x00001800U)
#define GPIOG_BASE            (AHBPERIPH_BASE + 0x00001C00U)
#define CRC_BASE              (AHBPERIPH_BASE + 0x00003000U)
#define RCC_BASE              (AHBPERIPH_BASE + 0x00003800U)
#define FLASH_R_BASE          (AHBPERIPH_BASE + 0x00003C00U) /*!< FLASH registers base address */
#define OB_BASE               ((uint32_t)0x1FF80000U)        /*!< FLASH Option Bytes base address */
#define FLASHSIZE_BASE        ((uint32_t)0x1FF800CCU)        /*!< FLASH Size register base address for Cat.3, Cat.4, Cat.5 and Cat.6 devices */
#define UID_BASE              ((uint32_t)0x1FF800D0U)        /*!< Unique device ID register base address for Cat.3, Cat.4, Cat.5 and Cat.6 devices */
#define DMA1_BASE             (AHBPERIPH_BASE + 0x00006000U)
#define DMA1_Channel1_BASE    (DMA1_BASE + 0x00000008U)
#define DMA1_Channel2_BASE    (DMA1_BASE + 0x0000001CU)
#define DMA1_Channel3_BASE    (DMA1_BASE + 0x00000030U)
#define DMA1_Channel4_BASE    (DMA1_BASE + 0x00000044U)
#define DMA1_Channel5_BASE    (DMA1_BASE + 0x00000058U)
#define DMA1_Channel6_BASE    (DMA1_BASE + 0x0000006CU)
#define DMA1_Channel7_BASE    (DMA1_BASE + 0x00000080U)
#define DMA2_BASE             (AHBPERIPH_BASE + 0x00006400U)
#define DMA2_Channel1_BASE    (DMA2_BASE + 0x00000008U)
#define DMA2_Channel2_BASE    (DMA2_BASE + 0x0000001CU)
#define DMA2_Channel3_BASE    (DMA2_BASE + 0x00000030U)
#define DMA2_Channel4_BASE    (DMA2_BASE + 0x00000044U)
#define DMA2_Channel5_BASE    (DMA2_BASE + 0x00000058U)
#define FSMC_BANK1            (FSMC_BASE)               /*!< FSMC Bank1 base address */
#define FSMC_BANK1_1          (FSMC_BANK1)              /*!< FSMC Bank1_1 base address */
#define FSMC_BANK1_2          (FSMC_BANK1 + 0x04000000U) /*!< FSMC Bank1_2 base address */
#define FSMC_BANK1_3          (FSMC_BANK1 + 0x08000000U) /*!< FSMC Bank1_3 base address */
#define FSMC_BANK1_4          (FSMC_BANK1 + 0x0C000000U) /*!< FSMC Bank1_4 base address */
#define FSMC_BANK1_R_BASE     (FSMC_R_BASE + 0x0000U)    /*!< FSMC Bank1 registers base address */
#define FSMC_BANK1E_R_BASE    (FSMC_R_BASE + 0x0104U)    /*!< FSMC Bank1E registers base address */
#define DBGMCU_BASE           ((uint32_t)0xE0042000U)     /*!< Debug MCU registers base address */

/**
  * @}
  */
  
/** @addtogroup Peripheral_declaration
  * @{
  */  

#define TIM2                ((TIM_TypeDef *) TIM2_BASE)
#define TIM3                ((TIM_TypeDef *) TIM3_BASE)
#define TIM4                ((TIM_TypeDef *) TIM4_BASE)
#define TIM5                ((TIM_TypeDef *) TIM5_BASE)
#define TIM6                ((TIM_TypeDef *) TIM6_BASE)
#define TIM7                ((TIM_TypeDef *) TIM7_BASE)
#define LCD                 ((LCD_TypeDef *) LCD_BASE)
#define RTC                 ((RTC_TypeDef *) RTC_BASE)
#define WWDG                ((WWDG_TypeDef *) WWDG_BASE)
#define IWDG                ((IWDG_TypeDef *) IWDG_BASE)
#define SPI2                ((SPI_TypeDef *) SPI2_BASE)
#define SPI3                ((SPI_TypeDef *) SPI3_BASE)
#define USART2              ((USART_TypeDef *) USART2_BASE)
#define USART3              ((USART_TypeDef *) USART3_BASE)
#define UART4               ((USART_TypeDef *) UART4_BASE)
#define UART5               ((USART_TypeDef *) UART5_BASE)
#define I2C1                ((I2C_TypeDef *) I2C1_BASE)
#define I2C2                ((I2C_TypeDef *) I2C2_BASE)
/* USB device FS */
#define USB                   ((USB_TypeDef *) USB_BASE)
/* USB device FS SRAM */
#define PWR                 ((PWR_TypeDef *) PWR_BASE)

#define DAC1                ((DAC_TypeDef *) DAC_BASE)
/* Legacy define */
#define DAC                 DAC1

#define COMP                ((COMP_TypeDef *) COMP_BASE)                 /* COMP generic instance include bits of COMP1 and COMP2 mixed in the same register */
#define COMP1               ((COMP_TypeDef *) COMP_BASE)                 /* COMP1 instance definition to differentiate COMP1 and COMP2, not to be used to access comparator register */
#define COMP2               ((COMP_TypeDef *) (COMP_BASE + 0x00000001U)) /* COMP2 instance definition to differentiate COMP1 and COMP2, not to be used to access comparator register */
#define COMP12_COMMON       ((COMP_Common_TypeDef *) COMP_BASE)          /* COMP common instance definition to access comparator register bits used by both comparator instances (window mode) */

#define RI                  ((RI_TypeDef *) RI_BASE)

#define OPAMP               ((OPAMP_TypeDef *) OPAMP_BASE)
#define OPAMP1              ((OPAMP_TypeDef *) OPAMP_BASE)
#define OPAMP2              ((OPAMP_TypeDef *) (OPAMP_BASE + 0x00000001U))
#define OPAMP3              ((OPAMP_TypeDef *) (OPAMP_BASE + 0x00000002U))
#define OPAMP123_COMMON     ((OPAMP_Common_TypeDef *) OPAMP_BASE)
#define SYSCFG              ((SYSCFG_TypeDef *) SYSCFG_BASE)
#define EXTI                ((EXTI_TypeDef *) EXTI_BASE)
#define TIM9                ((TIM_TypeDef *) TIM9_BASE)
#define TIM10               ((TIM_TypeDef *) TIM10_BASE)
#define TIM11               ((TIM_TypeDef *) TIM11_BASE)

#define ADC1                ((ADC_TypeDef *) ADC1_BASE)
#define ADC1_COMMON         ((ADC_Common_TypeDef *) ADC_BASE)
/* Legacy defines */
#define ADC                 ADC1_COMMON

#define SDIO                ((SDIO_TypeDef *) SDIO_BASE)
#define SPI1                ((SPI_TypeDef *) SPI1_BASE)
#define USART1              ((USART_TypeDef *) USART1_BASE)
#define GPIOA               ((GPIO_TypeDef *) GPIOA_BASE)
#define GPIOB               ((GPIO_TypeDef *) GPIOB_BASE)
#define GPIOC               ((GPIO_TypeDef *) GPIOC_BASE)
#define GPIOD               ((GPIO_TypeDef *) GPIOD_BASE)
#define GPIOE               ((GPIO_TypeDef *) GPIOE_BASE)
#define GPIOH               ((GPIO_TypeDef *) GPIOH_BASE)
#define GPIOF               ((GPIO_TypeDef *) GPIOF_BASE)
#define GPIOG               ((GPIO_TypeDef *) GPIOG_BASE)
#define CRC                 ((CRC_TypeDef *) CRC_BASE)
#define RCC                 ((RCC_TypeDef *) RCC_BASE)
#define FLASH               ((FLASH_TypeDef *) FLASH_R_BASE)
#define OB                  ((OB_TypeDef *) OB_BASE) 
#define DMA1                ((DMA_TypeDef *) DMA1_BASE)
#define DMA1_Channel1       ((DMA_Channel_TypeDef *) DMA1_Channel1_BASE)
#define DMA1_Channel2       ((DMA_Channel_TypeDef *) DMA1_Channel2_BASE)
#define DMA1_Channel3       ((DMA_Channel_TypeDef *) DMA1_Channel3_BASE)
#define DMA1_Channel4       ((DMA_Channel_TypeDef *) DMA1_Channel4_BASE)
#define DMA1_Channel5       ((DMA_Channel_TypeDef *) DMA1_Channel5_BASE)
#define DMA1_Channel6       ((DMA_Channel_TypeDef *) DMA1_Channel6_BASE)
#define DMA1_Channel7       ((DMA_Channel_TypeDef *) DMA1_Channel7_BASE)
#define DMA2                ((DMA_TypeDef *) DMA2_BASE)
#define DMA2_Channel1       ((DMA_Channel_TypeDef *) DMA2_Channel1_BASE)
#define DMA2_Channel2       ((DMA_Channel_TypeDef *) DMA2_Channel2_BASE)
#define DMA2_Channel3       ((DMA_Channel_TypeDef *) DMA2_Channel3_BASE)
#define DMA2_Channel4       ((DMA_Channel_TypeDef *) DMA2_Channel4_BASE)
#define DMA2_Channel5       ((DMA_Channel_TypeDef *) DMA2_Channel5_BASE)
#define FSMC_Bank1          ((FSMC_Bank1_TypeDef *) FSMC_BANK1_R_BASE)
#define FSMC_Bank1E         ((FSMC_Bank1E_TypeDef *) FSMC_BANK1E_R_BASE)
#define DBGMCU              ((DBGMCU_TypeDef *) DBGMCU_BASE)

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

/********************  Bit definition for ADC_SR register  ********************/
#define ADC_SR_AWD_Pos                       (0U)                              
#define ADC_SR_AWD_Msk                       (0x1U << ADC_SR_AWD_Pos)          /*!< 0x00000001 */
#define ADC_SR_AWD                           ADC_SR_AWD_Msk                    /*!< ADC analog watchdog 1 flag */
#define ADC_SR_EOCS_Pos                      (1U)                              
#define ADC_SR_EOCS_Msk                      (0x1U << ADC_SR_EOCS_Pos)         /*!< 0x00000002 */
#define ADC_SR_EOCS                          ADC_SR_EOCS_Msk                   /*!< ADC group regular end of unitary conversion or end of sequence conversions flag */
#define ADC_SR_JEOS_Pos                      (2U)                              
#define ADC_SR_JEOS_Msk                      (0x1U << ADC_SR_JEOS_Pos)         /*!< 0x00000004 */
#define ADC_SR_JEOS                          ADC_SR_JEOS_Msk                   /*!< ADC group injected end of sequence conversions flag */
#define ADC_SR_JSTRT_Pos                     (3U)                              
#define ADC_SR_JSTRT_Msk                     (0x1U << ADC_SR_JSTRT_Pos)        /*!< 0x00000008 */
#define ADC_SR_JSTRT                         ADC_SR_JSTRT_Msk                  /*!< ADC group injected conversion start flag */
#define ADC_SR_STRT_Pos                      (4U)                              
#define ADC_SR_STRT_Msk                      (0x1U << ADC_SR_STRT_Pos)         /*!< 0x00000010 */
#define ADC_SR_STRT                          ADC_SR_STRT_Msk                   /*!< ADC group regular conversion start flag */
#define ADC_SR_OVR_Pos                       (5U)                              
#define ADC_SR_OVR_Msk                       (0x1U << ADC_SR_OVR_Pos)          /*!< 0x00000020 */
#define ADC_SR_OVR                           ADC_SR_OVR_Msk                    /*!< ADC group regular overrun flag */
#define ADC_SR_ADONS_Pos                     (6U)                              
#define ADC_SR_ADONS_Msk                     (0x1U << ADC_SR_ADONS_Pos)        /*!< 0x00000040 */
#define ADC_SR_ADONS                         ADC_SR_ADONS_Msk                  /*!< ADC ready flag */
#define ADC_SR_RCNR_Pos                      (8U)                              
#define ADC_SR_RCNR_Msk                      (0x1U << ADC_SR_RCNR_Pos)         /*!< 0x00000100 */
#define ADC_SR_RCNR                          ADC_SR_RCNR_Msk                   /*!< ADC group regular not ready flag */
#define ADC_SR_JCNR_Pos                      (9U)                              
#define ADC_SR_JCNR_Msk                      (0x1U << ADC_SR_JCNR_Pos)         /*!< 0x00000200 */
#define ADC_SR_JCNR                          ADC_SR_JCNR_Msk                   /*!< ADC group injected not ready flag */

/* Legacy defines */
#define  ADC_SR_EOC                          (ADC_SR_EOCS)
#define  ADC_SR_JEOC                         (ADC_SR_JEOS)

/*******************  Bit definition for ADC_CR1 register  ********************/
#define ADC_CR1_AWDCH_Pos                    (0U)                              
#define ADC_CR1_AWDCH_Msk                    (0x1FU << ADC_CR1_AWDCH_Pos)      /*!< 0x0000001F */
#define ADC_CR1_AWDCH                        ADC_CR1_AWDCH_Msk                 /*!< ADC analog watchdog 1 monitored channel selection */
#define ADC_CR1_AWDCH_0                      (0x01U << ADC_CR1_AWDCH_Pos)      /*!< 0x00000001 */
#define ADC_CR1_AWDCH_1                      (0x02U << ADC_CR1_AWDCH_Pos)      /*!< 0x00000002 */
#define ADC_CR1_AWDCH_2                      (0x04U << ADC_CR1_AWDCH_Pos)      /*!< 0x00000004 */
#define ADC_CR1_AWDCH_3                      (0x08U << ADC_CR1_AWDCH_Pos)      /*!< 0x00000008 */
#define ADC_CR1_AWDCH_4                      (0x10U << ADC_CR1_AWDCH_Pos)      /*!< 0x00000010 */

#define ADC_CR1_EOCSIE_Pos                   (5U)                              
#define ADC_CR1_EOCSIE_Msk                   (0x1U << ADC_CR1_EOCSIE_Pos)      /*!< 0x00000020 */
#define ADC_CR1_EOCSIE                       ADC_CR1_EOCSIE_Msk                /*!< ADC group regular end of unitary conversion or end of sequence conversions interrupt */
#define ADC_CR1_AWDIE_Pos                    (6U)                              
#define ADC_CR1_AWDIE_Msk                    (0x1U << ADC_CR1_AWDIE_Pos)       /*!< 0x00000040 */
#define ADC_CR1_AWDIE                        ADC_CR1_AWDIE_Msk                 /*!< ADC analog watchdog 1 interrupt */
#define ADC_CR1_JEOSIE_Pos                   (7U)                              
#define ADC_CR1_JEOSIE_Msk                   (0x1U << ADC_CR1_JEOSIE_Pos)      /*!< 0x00000080 */
#define ADC_CR1_JEOSIE                       ADC_CR1_JEOSIE_Msk                /*!< ADC group injected end of sequence conversions interrupt */
#define ADC_CR1_SCAN_Pos                     (8U)                              
#define ADC_CR1_SCAN_Msk                     (0x1U << ADC_CR1_SCAN_Pos)        /*!< 0x00000100 */
#define ADC_CR1_SCAN                         ADC_CR1_SCAN_Msk                  /*!< ADC scan mode */
#define ADC_CR1_AWDSGL_Pos                   (9U)                              
#define ADC_CR1_AWDSGL_Msk                   (0x1U << ADC_CR1_AWDSGL_Pos)      /*!< 0x00000200 */
#define ADC_CR1_AWDSGL                       ADC_CR1_AWDSGL_Msk                /*!< ADC analog watchdog 1 monitoring a single channel or all channels */
#define ADC_CR1_JAUTO_Pos                    (10U)                             
#define ADC_CR1_JAUTO_Msk                    (0x1U << ADC_CR1_JAUTO_Pos)       /*!< 0x00000400 */
#define ADC_CR1_JAUTO                        ADC_CR1_JAUTO_Msk                 /*!< ADC group injected automatic trigger mode */
#define ADC_CR1_DISCEN_Pos                   (11U)                             
#define ADC_CR1_DISCEN_Msk                   (0x1U << ADC_CR1_DISCEN_Pos)      /*!< 0x00000800 */
#define ADC_CR1_DISCEN                       ADC_CR1_DISCEN_Msk                /*!< ADC group regular sequencer discontinuous mode */
#define ADC_CR1_JDISCEN_Pos                  (12U)                             
#define ADC_CR1_JDISCEN_Msk                  (0x1U << ADC_CR1_JDISCEN_Pos)     /*!< 0x00001000 */
#define ADC_CR1_JDISCEN                      ADC_CR1_JDISCEN_Msk               /*!< ADC group injected sequencer discontinuous mode */

#define ADC_CR1_DISCNUM_Pos                  (13U)                             
#define ADC_CR1_DISCNUM_Msk                  (0x7U << ADC_CR1_DISCNUM_Pos)     /*!< 0x0000E000 */
#define ADC_CR1_DISCNUM                      ADC_CR1_DISCNUM_Msk               /*!< ADC group regular sequencer discontinuous number of ranks */
#define ADC_CR1_DISCNUM_0                    (0x1U << ADC_CR1_DISCNUM_Pos)     /*!< 0x00002000 */
#define ADC_CR1_DISCNUM_1                    (0x2U << ADC_CR1_DISCNUM_Pos)     /*!< 0x00004000 */
#define ADC_CR1_DISCNUM_2                    (0x4U << ADC_CR1_DISCNUM_Pos)     /*!< 0x00008000 */

#define ADC_CR1_PDD_Pos                      (16U)                             
#define ADC_CR1_PDD_Msk                      (0x1U << ADC_CR1_PDD_Pos)         /*!< 0x00010000 */
#define ADC_CR1_PDD                          ADC_CR1_PDD_Msk                   /*!< ADC power down during auto delay phase */
#define ADC_CR1_PDI_Pos                      (17U)                             
#define ADC_CR1_PDI_Msk                      (0x1U << ADC_CR1_PDI_Pos)         /*!< 0x00020000 */
#define ADC_CR1_PDI                          ADC_CR1_PDI_Msk                   /*!< ADC power down during idle phase */

#define ADC_CR1_JAWDEN_Pos                   (22U)                             
#define ADC_CR1_JAWDEN_Msk                   (0x1U << ADC_CR1_JAWDEN_Pos)      /*!< 0x00400000 */
#define ADC_CR1_JAWDEN                       ADC_CR1_JAWDEN_Msk                /*!< ADC analog watchdog 1 enable on scope ADC group injected */
#define ADC_CR1_AWDEN_Pos                    (23U)                             
#define ADC_CR1_AWDEN_Msk                    (0x1U << ADC_CR1_AWDEN_Pos)       /*!< 0x00800000 */
#define ADC_CR1_AWDEN                        ADC_CR1_AWDEN_Msk                 /*!< ADC analog watchdog 1 enable on scope ADC group regular */

#define ADC_CR1_RES_Pos                      (24U)                             
#define ADC_CR1_RES_Msk                      (0x3U << ADC_CR1_RES_Pos)         /*!< 0x03000000 */
#define ADC_CR1_RES                          ADC_CR1_RES_Msk                   /*!< ADC resolution */
#define ADC_CR1_RES_0                        (0x1U << ADC_CR1_RES_Pos)         /*!< 0x01000000 */
#define ADC_CR1_RES_1                        (0x2U << ADC_CR1_RES_Pos)         /*!< 0x02000000 */

#define ADC_CR1_OVRIE_Pos                    (26U)                             
#define ADC_CR1_OVRIE_Msk                    (0x1U << ADC_CR1_OVRIE_Pos)       /*!< 0x04000000 */
#define ADC_CR1_OVRIE                        ADC_CR1_OVRIE_Msk                 /*!< ADC group regular overrun interrupt */

/* Legacy defines */
#define  ADC_CR1_EOCIE                       (ADC_CR1_EOCSIE)
#define  ADC_CR1_JEOCIE                      (ADC_CR1_JEOSIE)

/*******************  Bit definition for ADC_CR2 register  ********************/
#define ADC_CR2_ADON_Pos                     (0U)                              
#define ADC_CR2_ADON_Msk                     (0x1U << ADC_CR2_ADON_Pos)        /*!< 0x00000001 */
#define ADC_CR2_ADON                         ADC_CR2_ADON_Msk                  /*!< ADC enable */
#define ADC_CR2_CONT_Pos                     (1U)                              
#define ADC_CR2_CONT_Msk                     (0x1U << ADC_CR2_CONT_Pos)        /*!< 0x00000002 */
#define ADC_CR2_CONT                         ADC_CR2_CONT_Msk                  /*!< ADC group regular continuous conversion mode */
#define ADC_CR2_CFG_Pos                      (2U)                              
#define ADC_CR2_CFG_Msk                      (0x1U << ADC_CR2_CFG_Pos)         /*!< 0x00000004 */
#define ADC_CR2_CFG                          ADC_CR2_CFG_Msk                   /*!< ADC channels bank selection */

#define ADC_CR2_DELS_Pos                     (4U)                              
#define ADC_CR2_DELS_Msk                     (0x7U << ADC_CR2_DELS_Pos)        /*!< 0x00000070 */
#define ADC_CR2_DELS                         ADC_CR2_DELS_Msk                  /*!< ADC auto delay selection */
#define ADC_CR2_DELS_0                       (0x1U << ADC_CR2_DELS_Pos)        /*!< 0x00000010 */
#define ADC_CR2_DELS_1                       (0x2U << ADC_CR2_DELS_Pos)        /*!< 0x00000020 */
#define ADC_CR2_DELS_2                       (0x4U << ADC_CR2_DELS_Pos)        /*!< 0x00000040 */

#define ADC_CR2_DMA_Pos                      (8U)                              
#define ADC_CR2_DMA_Msk                      (0x1U << ADC_CR2_DMA_Pos)         /*!< 0x00000100 */
#define ADC_CR2_DMA                          ADC_CR2_DMA_Msk                   /*!< ADC DMA transfer enable */
#define ADC_CR2_DDS_Pos                      (9U)                              
#define ADC_CR2_DDS_Msk                      (0x1U << ADC_CR2_DDS_Pos)         /*!< 0x00000200 */
#define ADC_CR2_DDS                          ADC_CR2_DDS_Msk                   /*!< ADC DMA transfer configuration */
#define ADC_CR2_EOCS_Pos                     (10U)                             
#define ADC_CR2_EOCS_Msk                     (0x1U << ADC_CR2_EOCS_Pos)        /*!< 0x00000400 */
#define ADC_CR2_EOCS                         ADC_CR2_EOCS_Msk                  /*!< ADC end of unitary or end of sequence conversions selection */
#define ADC_CR2_ALIGN_Pos                    (11U)                             
#define ADC_CR2_ALIGN_Msk                    (0x1U << ADC_CR2_ALIGN_Pos)       /*!< 0x00000800 */
#define ADC_CR2_ALIGN                        ADC_CR2_ALIGN_Msk                 /*!< ADC data alignement */

#define ADC_CR2_JEXTSEL_Pos                  (16U)                             
#define ADC_CR2_JEXTSEL_Msk                  (0xFU << ADC_CR2_JEXTSEL_Pos)     /*!< 0x000F0000 */
#define ADC_CR2_JEXTSEL                      ADC_CR2_JEXTSEL_Msk               /*!< ADC group injected external trigger source */
#define ADC_CR2_JEXTSEL_0                    (0x1U << ADC_CR2_JEXTSEL_Pos)     /*!< 0x00010000 */
#define ADC_CR2_JEXTSEL_1                    (0x2U << ADC_CR2_JEXTSEL_Pos)     /*!< 0x00020000 */
#define ADC_CR2_JEXTSEL_2                    (0x4U << ADC_CR2_JEXTSEL_Pos)     /*!< 0x00040000 */
#define ADC_CR2_JEXTSEL_3                    (0x8U << ADC_CR2_JEXTSEL_Pos)     /*!< 0x00080000 */

#define ADC_CR2_JEXTEN_Pos                   (20U)                             
#define ADC_CR2_JEXTEN_Msk                   (0x3U << ADC_CR2_JEXTEN_Pos)      /*!< 0x00300000 */
#define ADC_CR2_JEXTEN                       ADC_CR2_JEXTEN_Msk                /*!< ADC group injected external trigger polarity */
#define ADC_CR2_JEXTEN_0                     (0x1U << ADC_CR2_JEXTEN_Pos)      /*!< 0x00100000 */
#define ADC_CR2_JEXTEN_1                     (0x2U << ADC_CR2_JEXTEN_Pos)      /*!< 0x00200000 */

#define ADC_CR2_JSWSTART_Pos                 (22U)                             
#define ADC_CR2_JSWSTART_Msk                 (0x1U << ADC_CR2_JSWSTART_Pos)    /*!< 0x00400000 */
#define ADC_CR2_JSWSTART                     ADC_CR2_JSWSTART_Msk              /*!< ADC group injected conversion start */

#define ADC_CR2_EXTSEL_Pos                   (24U)                             
#define ADC_CR2_EXTSEL_Msk                   (0xFU << ADC_CR2_EXTSEL_Pos)      /*!< 0x0F000000 */
#define ADC_CR2_EXTSEL                       ADC_CR2_EXTSEL_Msk                /*!< ADC group regular external trigger source */
#define ADC_CR2_EXTSEL_0                     (0x1U << ADC_CR2_EXTSEL_Pos)      /*!< 0x01000000 */
#define ADC_CR2_EXTSEL_1                     (0x2U << ADC_CR2_EXTSEL_Pos)      /*!< 0x02000000 */
#define ADC_CR2_EXTSEL_2                     (0x4U << ADC_CR2_EXTSEL_Pos)      /*!< 0x04000000 */
#define ADC_CR2_EXTSEL_3                     (0x8U << ADC_CR2_EXTSEL_Pos)      /*!< 0x08000000 */

#define ADC_CR2_EXTEN_Pos                    (28U)                             
#define ADC_CR2_EXTEN_Msk                    (0x3U << ADC_CR2_EXTEN_Pos)       /*!< 0x30000000 */
#define ADC_CR2_EXTEN                        ADC_CR2_EXTEN_Msk                 /*!< ADC group regular external trigger polarity */
#define ADC_CR2_EXTEN_0                      (0x1U << ADC_CR2_EXTEN_Pos)       /*!< 0x10000000 */
#define ADC_CR2_EXTEN_1                      (0x2U << ADC_CR2_EXTEN_Pos)       /*!< 0x20000000 */

#define ADC_CR2_SWSTART_Pos                  (30U)                             
#define ADC_CR2_SWSTART_Msk                  (0x1U << ADC_CR2_SWSTART_Pos)     /*!< 0x40000000 */
#define ADC_CR2_SWSTART                      ADC_CR2_SWSTART_Msk               /*!< ADC group regular conversion start */

/******************  Bit definition for ADC_SMPR1 register  *******************/
#define ADC_SMPR1_SMP20_Pos                  (0U)                              
#define ADC_SMPR1_SMP20_Msk                  (0x7U << ADC_SMPR1_SMP20_Pos)     /*!< 0x00000007 */
#define ADC_SMPR1_SMP20                      ADC_SMPR1_SMP20_Msk               /*!< ADC channel 20 sampling time selection */
#define ADC_SMPR1_SMP20_0                    (0x1U << ADC_SMPR1_SMP20_Pos)     /*!< 0x00000001 */
#define ADC_SMPR1_SMP20_1                    (0x2U << ADC_SMPR1_SMP20_Pos)     /*!< 0x00000002 */
#define ADC_SMPR1_SMP20_2                    (0x4U << ADC_SMPR1_SMP20_Pos)     /*!< 0x00000004 */

#define ADC_SMPR1_SMP21_Pos                  (3U)                              
#define ADC_SMPR1_SMP21_Msk                  (0x7U << ADC_SMPR1_SMP21_Pos)     /*!< 0x00000038 */
#define ADC_SMPR1_SMP21                      ADC_SMPR1_SMP21_Msk               /*!< ADC channel 21 sampling time selection */
#define ADC_SMPR1_SMP21_0                    (0x1U << ADC_SMPR1_SMP21_Pos)     /*!< 0x00000008 */
#define ADC_SMPR1_SMP21_1                    (0x2U << ADC_SMPR1_SMP21_Pos)     /*!< 0x00000010 */
#define ADC_SMPR1_SMP21_2                    (0x4U << ADC_SMPR1_SMP21_Pos)     /*!< 0x00000020 */

#define ADC_SMPR1_SMP22_Pos                  (6U)                              
#define ADC_SMPR1_SMP22_Msk                  (0x7U << ADC_SMPR1_SMP22_Pos)     /*!< 0x000001C0 */
#define ADC_SMPR1_SMP22                      ADC_SMPR1_SMP22_Msk               /*!< ADC channel 22 sampling time selection */
#define ADC_SMPR1_SMP22_0                    (0x1U << ADC_SMPR1_SMP22_Pos)     /*!< 0x00000040 */
#define ADC_SMPR1_SMP22_1                    (0x2U << ADC_SMPR1_SMP22_Pos)     /*!< 0x00000080 */
#define ADC_SMPR1_SMP22_2                    (0x4U << ADC_SMPR1_SMP22_Pos)     /*!< 0x00000100 */

#define ADC_SMPR1_SMP23_Pos                  (9U)                              
#define ADC_SMPR1_SMP23_Msk                  (0x7U << ADC_SMPR1_SMP23_Pos)     /*!< 0x00000E00 */
#define ADC_SMPR1_SMP23                      ADC_SMPR1_SMP23_Msk               /*!< ADC channel 23 sampling time selection */
#define ADC_SMPR1_SMP23_0                    (0x1U << ADC_SMPR1_SMP23_Pos)     /*!< 0x00000200 */
#define ADC_SMPR1_SMP23_1                    (0x2U << ADC_SMPR1_SMP23_Pos)     /*!< 0x00000400 */
#define ADC_SMPR1_SMP23_2                    (0x4U << ADC_SMPR1_SMP23_Pos)     /*!< 0x00000800 */

#define ADC_SMPR1_SMP24_Pos                  (12U)                             
#define ADC_SMPR1_SMP24_Msk                  (0x7U << ADC_SMPR1_SMP24_Pos)     /*!< 0x00007000 */
#define ADC_SMPR1_SMP24                      ADC_SMPR1_SMP24_Msk               /*!< ADC channel 24 sampling time selection */
#define ADC_SMPR1_SMP24_0                    (0x1U << ADC_SMPR1_SMP24_Pos)     /*!< 0x00001000 */
#define ADC_SMPR1_SMP24_1                    (0x2U << ADC_SMPR1_SMP24_Pos)     /*!< 0x00002000 */
#define ADC_SMPR1_SMP24_2                    (0x4U << ADC_SMPR1_SMP24_Pos)     /*!< 0x00004000 */

#define ADC_SMPR1_SMP25_Pos                  (15U)                             
#define ADC_SMPR1_SMP25_Msk                  (0x7U << ADC_SMPR1_SMP25_Pos)     /*!< 0x00038000 */
#define ADC_SMPR1_SMP25                      ADC_SMPR1_SMP25_Msk               /*!< ADC channel 25 sampling time selection */
#define ADC_SMPR1_SMP25_0                    (0x1U << ADC_SMPR1_SMP25_Pos)     /*!< 0x00008000 */
#define ADC_SMPR1_SMP25_1                    (0x2U << ADC_SMPR1_SMP25_Pos)     /*!< 0x00010000 */
#define ADC_SMPR1_SMP25_2                    (0x4U << ADC_SMPR1_SMP25_Pos)     /*!< 0x00020000 */

#define ADC_SMPR1_SMP26_Pos                  (18U)                             
#define ADC_SMPR1_SMP26_Msk                  (0x7U << ADC_SMPR1_SMP26_Pos)     /*!< 0x001C0000 */
#define ADC_SMPR1_SMP26                      ADC_SMPR1_SMP26_Msk               /*!< ADC channel 26 sampling time selection */
#define ADC_SMPR1_SMP26_0                    (0x1U << ADC_SMPR1_SMP26_Pos)     /*!< 0x00040000 */
#define ADC_SMPR1_SMP26_1                    (0x2U << ADC_SMPR1_SMP26_Pos)     /*!< 0x00080000 */
#define ADC_SMPR1_SMP26_2                    (0x4U << ADC_SMPR1_SMP26_Pos)     /*!< 0x00100000 */

#define ADC_SMPR1_SMP27_Pos                  (21U)                             
#define ADC_SMPR1_SMP27_Msk                  (0x7U << ADC_SMPR1_SMP27_Pos)     /*!< 0x00E00000 */
#define ADC_SMPR1_SMP27                      ADC_SMPR1_SMP27_Msk               /*!< ADC channel 27 sampling time selection */
#define ADC_SMPR1_SMP27_0                    (0x1U << ADC_SMPR1_SMP27_Pos)     /*!< 0x00200000 */
#define ADC_SMPR1_SMP27_1                    (0x2U << ADC_SMPR1_SMP27_Pos)     /*!< 0x00400000 */
#define ADC_SMPR1_SMP27_2                    (0x4U << ADC_SMPR1_SMP27_Pos)     /*!< 0x00800000 */

#define ADC_SMPR1_SMP28_Pos                  (24U)                             
#define ADC_SMPR1_SMP28_Msk                  (0x7U << ADC_SMPR1_SMP28_Pos)     /*!< 0x07000000 */
#define ADC_SMPR1_SMP28                      ADC_SMPR1_SMP28_Msk               /*!< ADC channel 28 sampling time selection */
#define ADC_SMPR1_SMP28_0                    (0x1U << ADC_SMPR1_SMP28_Pos)     /*!< 0x01000000 */
#define ADC_SMPR1_SMP28_1                    (0x2U << ADC_SMPR1_SMP28_Pos)     /*!< 0x02000000 */
#define ADC_SMPR1_SMP28_2                    (0x4U << ADC_SMPR1_SMP28_Pos)     /*!< 0x04000000 */

#define ADC_SMPR1_SMP29_Pos                  (27U)                             
#define ADC_SMPR1_SMP29_Msk                  (0x7U << ADC_SMPR1_SMP29_Pos)     /*!< 0x38000000 */
#define ADC_SMPR1_SMP29                      ADC_SMPR1_SMP29_Msk               /*!< ADC channel 29 sampling time selection */
#define ADC_SMPR1_SMP29_0                    (0x1U << ADC_SMPR1_SMP29_Pos)     /*!< 0x08000000 */
#define ADC_SMPR1_SMP29_1                    (0x2U << ADC_SMPR1_SMP29_Pos)     /*!< 0x10000000 */
#define ADC_SMPR1_SMP29_2                    (0x4U << ADC_SMPR1_SMP29_Pos)     /*!< 0x20000000 */

/******************  Bit definition for ADC_SMPR2 register  *******************/
#define ADC_SMPR2_SMP10_Pos                  (0U)                              
#define ADC_SMPR2_SMP10_Msk                  (0x7U << ADC_SMPR2_SMP10_Pos)     /*!< 0x00000007 */
#define ADC_SMPR2_SMP10                      ADC_SMPR2_SMP10_Msk               /*!< ADC channel 10 sampling time selection */
#define ADC_SMPR2_SMP10_0                    (0x1U << ADC_SMPR2_SMP10_Pos)     /*!< 0x00000001 */
#define ADC_SMPR2_SMP10_1                    (0x2U << ADC_SMPR2_SMP10_Pos)     /*!< 0x00000002 */
#define ADC_SMPR2_SMP10_2                    (0x4U << ADC_SMPR2_SMP10_Pos)     /*!< 0x00000004 */

#define ADC_SMPR2_SMP11_Pos                  (3U)                              
#define ADC_SMPR2_SMP11_Msk                  (0x7U << ADC_SMPR2_SMP11_Pos)     /*!< 0x00000038 */
#define ADC_SMPR2_SMP11                      ADC_SMPR2_SMP11_Msk               /*!< ADC channel 11 sampling time selection */
#define ADC_SMPR2_SMP11_0                    (0x1U << ADC_SMPR2_SMP11_Pos)     /*!< 0x00000008 */
#define ADC_SMPR2_SMP11_1                    (0x2U << ADC_SMPR2_SMP11_Pos)     /*!< 0x00000010 */
#define ADC_SMPR2_SMP11_2                    (0x4U << ADC_SMPR2_SMP11_Pos)     /*!< 0x00000020 */

#define ADC_SMPR2_SMP12_Pos                  (6U)                              
#define ADC_SMPR2_SMP12_Msk                  (0x7U << ADC_SMPR2_SMP12_Pos)     /*!< 0x000001C0 */
#define ADC_SMPR2_SMP12                      ADC_SMPR2_SMP12_Msk               /*!< ADC channel 12 sampling time selection */
#define ADC_SMPR2_SMP12_0                    (0x1U << ADC_SMPR2_SMP12_Pos)     /*!< 0x00000040 */
#define ADC_SMPR2_SMP12_1                    (0x2U << ADC_SMPR2_SMP12_Pos)     /*!< 0x00000080 */
#define ADC_SMPR2_SMP12_2                    (0x4U << ADC_SMPR2_SMP12_Pos)     /*!< 0x00000100 */

#define ADC_SMPR2_SMP13_Pos                  (9U)                              
#define ADC_SMPR2_SMP13_Msk                  (0x7U << ADC_SMPR2_SMP13_Pos)     /*!< 0x00000E00 */
#define ADC_SMPR2_SMP13                      ADC_SMPR2_SMP13_Msk               /*!< ADC channel 13 sampling time selection */
#define ADC_SMPR2_SMP13_0                    (0x1U << ADC_SMPR2_SMP13_Pos)     /*!< 0x00000200 */
#define ADC_SMPR2_SMP13_1                    (0x2U << ADC_SMPR2_SMP13_Pos)     /*!< 0x00000400 */
#define ADC_SMPR2_SMP13_2                    (0x4U << ADC_SMPR2_SMP13_Pos)     /*!< 0x00000800 */

#define ADC_SMPR2_SMP14_Pos                  (12U)                             
#define ADC_SMPR2_SMP14_Msk                  (0x7U << ADC_SMPR2_SMP14_Pos)     /*!< 0x00007000 */
#define ADC_SMPR2_SMP14                      ADC_SMPR2_SMP14_Msk               /*!< ADC channel 14 sampling time selection */
#define ADC_SMPR2_SMP14_0                    (0x1U << ADC_SMPR2_SMP14_Pos)     /*!< 0x00001000 */
#define ADC_SMPR2_SMP14_1                    (0x2U << ADC_SMPR2_SMP14_Pos)     /*!< 0x00002000 */
#define ADC_SMPR2_SMP14_2                    (0x4U << ADC_SMPR2_SMP14_Pos)     /*!< 0x00004000 */

#define ADC_SMPR2_SMP15_Pos                  (15U)                             
#define ADC_SMPR2_SMP15_Msk                  (0x7U << ADC_SMPR2_SMP15_Pos)     /*!< 0x00038000 */
#define ADC_SMPR2_SMP15                      ADC_SMPR2_SMP15_Msk               /*!< ADC channel 5 sampling time selection */
#define ADC_SMPR2_SMP15_0                    (0x1U << ADC_SMPR2_SMP15_Pos)     /*!< 0x00008000 */
#define ADC_SMPR2_SMP15_1                    (0x2U << ADC_SMPR2_SMP15_Pos)     /*!< 0x00010000 */
#define ADC_SMPR2_SMP15_2                    (0x4U << ADC_SMPR2_SMP15_Pos)     /*!< 0x00020000 */

#define ADC_SMPR2_SMP16_Pos                  (18U)                             
#define ADC_SMPR2_SMP16_Msk                  (0x7U << ADC_SMPR2_SMP16_Pos)     /*!< 0x001C0000 */
#define ADC_SMPR2_SMP16                      ADC_SMPR2_SMP16_Msk               /*!< ADC channel 16 sampling time selection */
#define ADC_SMPR2_SMP16_0                    (0x1U << ADC_SMPR2_SMP16_Pos)     /*!< 0x00040000 */
#define ADC_SMPR2_SMP16_1                    (0x2U << ADC_SMPR2_SMP16_Pos)     /*!< 0x00080000 */
#define ADC_SMPR2_SMP16_2                    (0x4U << ADC_SMPR2_SMP16_Pos)     /*!< 0x00100000 */

#define ADC_SMPR2_SMP17_Pos                  (21U)                             
#define ADC_SMPR2_SMP17_Msk                  (0x7U << ADC_SMPR2_SMP17_Pos)     /*!< 0x00E00000 */
#define ADC_SMPR2_SMP17                      ADC_SMPR2_SMP17_Msk               /*!< ADC channel 17 sampling time selection */
#define ADC_SMPR2_SMP17_0                    (0x1U << ADC_SMPR2_SMP17_Pos)     /*!< 0x00200000 */
#define ADC_SMPR2_SMP17_1                    (0x2U << ADC_SMPR2_SMP17_Pos)     /*!< 0x00400000 */
#define ADC_SMPR2_SMP17_2                    (0x4U << ADC_SMPR2_SMP17_Pos)     /*!< 0x00800000 */

#define ADC_SMPR2_SMP18_Pos                  (24U)                             
#define ADC_SMPR2_SMP18_Msk                  (0x7U << ADC_SMPR2_SMP18_Pos)     /*!< 0x07000000 */
#define ADC_SMPR2_SMP18                      ADC_SMPR2_SMP18_Msk               /*!< ADC channel 18 sampling time selection */
#define ADC_SMPR2_SMP18_0                    (0x1U << ADC_SMPR2_SMP18_Pos)     /*!< 0x01000000 */
#define ADC_SMPR2_SMP18_1                    (0x2U << ADC_SMPR2_SMP18_Pos)     /*!< 0x02000000 */
#define ADC_SMPR2_SMP18_2                    (0x4U << ADC_SMPR2_SMP18_Pos)     /*!< 0x04000000 */

#define ADC_SMPR2_SMP19_Pos                  (27U)                             
#define ADC_SMPR2_SMP19_Msk                  (0x7U << ADC_SMPR2_SMP19_Pos)     /*!< 0x38000000 */
#define ADC_SMPR2_SMP19                      ADC_SMPR2_SMP19_Msk               /*!< ADC channel 19 sampling time selection */
#define ADC_SMPR2_SMP19_0                    (0x1U << ADC_SMPR2_SMP19_Pos)     /*!< 0x08000000 */
#define ADC_SMPR2_SMP19_1                    (0x2U << ADC_SMPR2_SMP19_Pos)     /*!< 0x10000000 */
#define ADC_SMPR2_SMP19_2                    (0x4U << ADC_SMPR2_SMP19_Pos)     /*!< 0x20000000 */

/******************  Bit definition for ADC_SMPR3 register  *******************/
#define ADC_SMPR3_SMP0_Pos                   (0U)                              
#define ADC_SMPR3_SMP0_Msk                   (0x7U << ADC_SMPR3_SMP0_Pos)      /*!< 0x00000007 */
#define ADC_SMPR3_SMP0                       ADC_SMPR3_SMP0_Msk                /*!< ADC channel 0 sampling time selection */
#define ADC_SMPR3_SMP0_0                     (0x1U << ADC_SMPR3_SMP0_Pos)      /*!< 0x00000001 */
#define ADC_SMPR3_SMP0_1                     (0x2U << ADC_SMPR3_SMP0_Pos)      /*!< 0x00000002 */
#define ADC_SMPR3_SMP0_2                     (0x4U << ADC_SMPR3_SMP0_Pos)      /*!< 0x00000004 */
 
#define ADC_SMPR3_SMP1_Pos                   (3U)                              
#define ADC_SMPR3_SMP1_Msk                   (0x7U << ADC_SMPR3_SMP1_Pos)      /*!< 0x00000038 */
#define ADC_SMPR3_SMP1                       ADC_SMPR3_SMP1_Msk                /*!< ADC channel 1 sampling time selection */
#define ADC_SMPR3_SMP1_0                     (0x1U << ADC_SMPR3_SMP1_Pos)      /*!< 0x00000008 */
#define ADC_SMPR3_SMP1_1                     (0x2U << ADC_SMPR3_SMP1_Pos)      /*!< 0x00000010 */
#define ADC_SMPR3_SMP1_2                     (0x4U << ADC_SMPR3_SMP1_Pos)      /*!< 0x00000020 */

#define ADC_SMPR3_SMP2_Pos                   (6U)                              
#define ADC_SMPR3_SMP2_Msk                   (0x7U << ADC_SMPR3_SMP2_Pos)      /*!< 0x000001C0 */
#define ADC_SMPR3_SMP2                       ADC_SMPR3_SMP2_Msk                /*!< ADC channel 2 sampling time selection */
#define ADC_SMPR3_SMP2_0                     (0x1U << ADC_SMPR3_SMP2_Pos)      /*!< 0x00000040 */
#define ADC_SMPR3_SMP2_1                     (0x2U << ADC_SMPR3_SMP2_Pos)      /*!< 0x00000080 */
#define ADC_SMPR3_SMP2_2                     (0x4U << ADC_SMPR3_SMP2_Pos)      /*!< 0x00000100 */

#define ADC_SMPR3_SMP3_Pos                   (9U)                              
#define ADC_SMPR3_SMP3_Msk                   (0x7U << ADC_SMPR3_SMP3_Pos)      /*!< 0x00000E00 */
#define ADC_SMPR3_SMP3                       ADC_SMPR3_SMP3_Msk                /*!< ADC channel 3 sampling time selection */
#define ADC_SMPR3_SMP3_0                     (0x1U << ADC_SMPR3_SMP3_Pos)      /*!< 0x00000200 */
#define ADC_SMPR3_SMP3_1                     (0x2U << ADC_SMPR3_SMP3_Pos)      /*!< 0x00000400 */
#define ADC_SMPR3_SMP3_2                     (0x4U << ADC_SMPR3_SMP3_Pos)      /*!< 0x00000800 */

#define ADC_SMPR3_SMP4_Pos                   (12U)                             
#define ADC_SMPR3_SMP4_Msk                   (0x7U << ADC_SMPR3_SMP4_Pos)      /*!< 0x00007000 */
#define ADC_SMPR3_SMP4                       ADC_SMPR3_SMP4_Msk                /*!< ADC channel 4 sampling time selection */
#define ADC_SMPR3_SMP4_0                     (0x1U << ADC_SMPR3_SMP4_Pos)      /*!< 0x00001000 */
#define ADC_SMPR3_SMP4_1                     (0x2U << ADC_SMPR3_SMP4_Pos)      /*!< 0x00002000 */
#define ADC_SMPR3_SMP4_2                     (0x4U << ADC_SMPR3_SMP4_Pos)      /*!< 0x00004000 */

#define ADC_SMPR3_SMP5_Pos                   (15U)                             
#define ADC_SMPR3_SMP5_Msk                   (0x7U << ADC_SMPR3_SMP5_Pos)      /*!< 0x00038000 */
#define ADC_SMPR3_SMP5                       ADC_SMPR3_SMP5_Msk                /*!< ADC channel 5 sampling time selection */
#define ADC_SMPR3_SMP5_0                     (0x1U << ADC_SMPR3_SMP5_Pos)      /*!< 0x00008000 */
#define ADC_SMPR3_SMP5_1                     (0x2U << ADC_SMPR3_SMP5_Pos)      /*!< 0x00010000 */
#define ADC_SMPR3_SMP5_2                     (0x4U << ADC_SMPR3_SMP5_Pos)      /*!< 0x00020000 */

#define ADC_SMPR3_SMP6_Pos                   (18U)                             
#define ADC_SMPR3_SMP6_Msk                   (0x7U << ADC_SMPR3_SMP6_Pos)      /*!< 0x001C0000 */
#define ADC_SMPR3_SMP6                       ADC_SMPR3_SMP6_Msk                /*!< ADC channel 6 sampling time selection */
#define ADC_SMPR3_SMP6_0                     (0x1U << ADC_SMPR3_SMP6_Pos)      /*!< 0x00040000 */
#define ADC_SMPR3_SMP6_1                     (0x2U << ADC_SMPR3_SMP6_Pos)      /*!< 0x00080000 */
#define ADC_SMPR3_SMP6_2                     (0x4U << ADC_SMPR3_SMP6_Pos)      /*!< 0x00100000 */

#define ADC_SMPR3_SMP7_Pos                   (21U)                             
#define ADC_SMPR3_SMP7_Msk                   (0x7U << ADC_SMPR3_SMP7_Pos)      /*!< 0x00E00000 */
#define ADC_SMPR3_SMP7                       ADC_SMPR3_SMP7_Msk                /*!< ADC channel 7 sampling time selection */
#define ADC_SMPR3_SMP7_0                     (0x1U << ADC_SMPR3_SMP7_Pos)      /*!< 0x00200000 */
#define ADC_SMPR3_SMP7_1                     (0x2U << ADC_SMPR3_SMP7_Pos)      /*!< 0x00400000 */
#define ADC_SMPR3_SMP7_2                     (0x4U << ADC_SMPR3_SMP7_Pos)      /*!< 0x00800000 */

#define ADC_SMPR3_SMP8_Pos                   (24U)                             
#define ADC_SMPR3_SMP8_Msk                   (0x7U << ADC_SMPR3_SMP8_Pos)      /*!< 0x07000000 */
#define ADC_SMPR3_SMP8                       ADC_SMPR3_SMP8_Msk                /*!< ADC channel 8 sampling time selection */
#define ADC_SMPR3_SMP8_0                     (0x1U << ADC_SMPR3_SMP8_Pos)      /*!< 0x01000000 */
#define ADC_SMPR3_SMP8_1                     (0x2U << ADC_SMPR3_SMP8_Pos)      /*!< 0x02000000 */
#define ADC_SMPR3_SMP8_2                     (0x4U << ADC_SMPR3_SMP8_Pos)      /*!< 0x04000000 */

#define ADC_SMPR3_SMP9_Pos                   (27U)                             
#define ADC_SMPR3_SMP9_Msk                   (0x7U << ADC_SMPR3_SMP9_Pos)      /*!< 0x38000000 */
#define ADC_SMPR3_SMP9                       ADC_SMPR3_SMP9_Msk                /*!< ADC channel 9 sampling time selection */
#define ADC_SMPR3_SMP9_0                     (0x1U << ADC_SMPR3_SMP9_Pos)      /*!< 0x08000000 */
#define ADC_SMPR3_SMP9_1                     (0x2U << ADC_SMPR3_SMP9_Pos)      /*!< 0x10000000 */
#define ADC_SMPR3_SMP9_2                     (0x4U << ADC_SMPR3_SMP9_Pos)      /*!< 0x20000000 */

/******************  Bit definition for ADC_JOFR1 register  *******************/
#define ADC_JOFR1_JOFFSET1_Pos               (0U)                              
#define ADC_JOFR1_JOFFSET1_Msk               (0xFFFU << ADC_JOFR1_JOFFSET1_Pos) /*!< 0x00000FFF */
#define ADC_JOFR1_JOFFSET1                   ADC_JOFR1_JOFFSET1_Msk            /*!< ADC group injected sequencer rank 1 offset value */

/******************  Bit definition for ADC_JOFR2 register  *******************/
#define ADC_JOFR2_JOFFSET2_Pos               (0U)                              
#define ADC_JOFR2_JOFFSET2_Msk               (0xFFFU << ADC_JOFR2_JOFFSET2_Pos) /*!< 0x00000FFF */
#define ADC_JOFR2_JOFFSET2                   ADC_JOFR2_JOFFSET2_Msk            /*!< ADC group injected sequencer rank 2 offset value */

/******************  Bit definition for ADC_JOFR3 register  *******************/
#define ADC_JOFR3_JOFFSET3_Pos               (0U)                              
#define ADC_JOFR3_JOFFSET3_Msk               (0xFFFU << ADC_JOFR3_JOFFSET3_Pos) /*!< 0x00000FFF */
#define ADC_JOFR3_JOFFSET3                   ADC_JOFR3_JOFFSET3_Msk            /*!< ADC group injected sequencer rank 3 offset value */

/******************  Bit definition for ADC_JOFR4 register  *******************/
#define ADC_JOFR4_JOFFSET4_Pos               (0U)                              
#define ADC_JOFR4_JOFFSET4_Msk               (0xFFFU << ADC_JOFR4_JOFFSET4_Pos) /*!< 0x00000FFF */
#define ADC_JOFR4_JOFFSET4                   ADC_JOFR4_JOFFSET4_Msk            /*!< ADC group injected sequencer rank 4 offset value */

/*******************  Bit definition for ADC_HTR register  ********************/
#define ADC_HTR_HT_Pos                       (0U)                              
#define ADC_HTR_HT_Msk                       (0xFFFU << ADC_HTR_HT_Pos)        /*!< 0x00000FFF */
#define ADC_HTR_HT                           ADC_HTR_HT_Msk                    /*!< ADC analog watchdog 1 threshold high */

/*******************  Bit definition for ADC_LTR register  ********************/
#define ADC_LTR_LT_Pos                       (0U)                              
#define ADC_LTR_LT_Msk                       (0xFFFU << ADC_LTR_LT_Pos)        /*!< 0x00000FFF */
#define ADC_LTR_LT                           ADC_LTR_LT_Msk                    /*!< ADC analog watchdog 1 threshold low */

/*******************  Bit definition for ADC_SQR1 register  *******************/
#define ADC_SQR1_L_Pos                       (20U)                             
#define ADC_SQR1_L_Msk                       (0x1FU << ADC_SQR1_L_Pos)         /*!< 0x01F00000 */
#define ADC_SQR1_L                           ADC_SQR1_L_Msk                    /*!< ADC group regular sequencer scan length */
#define ADC_SQR1_L_0                         (0x01U << ADC_SQR1_L_Pos)         /*!< 0x00100000 */
#define ADC_SQR1_L_1                         (0x02U << ADC_SQR1_L_Pos)         /*!< 0x00200000 */
#define ADC_SQR1_L_2                         (0x04U << ADC_SQR1_L_Pos)         /*!< 0x00400000 */
#define ADC_SQR1_L_3                         (0x08U << ADC_SQR1_L_Pos)         /*!< 0x00800000 */
#define ADC_SQR1_L_4                         (0x10U << ADC_SQR1_L_Pos)         /*!< 0x01000000 */

#define ADC_SQR1_SQ28_Pos                    (15U)                             
#define ADC_SQR1_SQ28_Msk                    (0x1FU << ADC_SQR1_SQ28_Pos)      /*!< 0x000F8000 */
#define ADC_SQR1_SQ28                        ADC_SQR1_SQ28_Msk                 /*!< ADC group regular sequencer rank 28 */
#define ADC_SQR1_SQ28_0                      (0x01U << ADC_SQR1_SQ28_Pos)      /*!< 0x00008000 */
#define ADC_SQR1_SQ28_1                      (0x02U << ADC_SQR1_SQ28_Pos)      /*!< 0x00010000 */
#define ADC_SQR1_SQ28_2                      (0x04U << ADC_SQR1_SQ28_Pos)      /*!< 0x00020000 */
#define ADC_SQR1_SQ28_3                      (0x08U << ADC_SQR1_SQ28_Pos)      /*!< 0x00040000 */
#define ADC_SQR1_SQ28_4                      (0x10U << ADC_SQR1_SQ28_Pos)      /*!< 0x00080000 */

#define ADC_SQR1_SQ27_Pos                    (10U)                             
#define ADC_SQR1_SQ27_Msk                    (0x1FU << ADC_SQR1_SQ27_Pos)      /*!< 0x00007C00 */
#define ADC_SQR1_SQ27                        ADC_SQR1_SQ27_Msk                 /*!< ADC group regular sequencer rank 27 */
#define ADC_SQR1_SQ27_0                      (0x01U << ADC_SQR1_SQ27_Pos)      /*!< 0x00000400 */
#define ADC_SQR1_SQ27_1                      (0x02U << ADC_SQR1_SQ27_Pos)      /*!< 0x00000800 */
#define ADC_SQR1_SQ27_2                      (0x04U << ADC_SQR1_SQ27_Pos)      /*!< 0x00001000 */
#define ADC_SQR1_SQ27_3                      (0x08U << ADC_SQR1_SQ27_Pos)      /*!< 0x00002000 */
#define ADC_SQR1_SQ27_4                      (0x10U << ADC_SQR1_SQ27_Pos)      /*!< 0x00004000 */

#define ADC_SQR1_SQ26_Pos                    (5U)                              
#define ADC_SQR1_SQ26_Msk                    (0x1FU << ADC_SQR1_SQ26_Pos)      /*!< 0x000003E0 */
#define ADC_SQR1_SQ26                        ADC_SQR1_SQ26_Msk                 /*!< ADC group regular sequencer rank 26 */
#define ADC_SQR1_SQ26_0                      (0x01U << ADC_SQR1_SQ26_Pos)      /*!< 0x00000020 */
#define ADC_SQR1_SQ26_1                      (0x02U << ADC_SQR1_SQ26_Pos)      /*!< 0x00000040 */
#define ADC_SQR1_SQ26_2                      (0x04U << ADC_SQR1_SQ26_Pos)      /*!< 0x00000080 */
#define ADC_SQR1_SQ26_3                      (0x08U << ADC_SQR1_SQ26_Pos)      /*!< 0x00000100 */
#define ADC_SQR1_SQ26_4                      (0x10U << ADC_SQR1_SQ26_Pos)      /*!< 0x00000200 */

#define ADC_SQR1_SQ25_Pos                    (0U)                              
#define ADC_SQR1_SQ25_Msk                    (0x1FU << ADC_SQR1_SQ25_Pos)      /*!< 0x0000001F */
#define ADC_SQR1_SQ25                        ADC_SQR1_SQ25_Msk                 /*!< ADC group regular sequencer rank 25 */
#define ADC_SQR1_SQ25_0                      (0x01U << ADC_SQR1_SQ25_Pos)      /*!< 0x00000001 */
#define ADC_SQR1_SQ25_1                      (0x02U << ADC_SQR1_SQ25_Pos)      /*!< 0x00000002 */
#define ADC_SQR1_SQ25_2                      (0x04U << ADC_SQR1_SQ25_Pos)      /*!< 0x00000004 */
#define ADC_SQR1_SQ25_3                      (0x08U << ADC_SQR1_SQ25_Pos)      /*!< 0x00000008 */
#define ADC_SQR1_SQ25_4                      (0x10U << ADC_SQR1_SQ25_Pos)      /*!< 0x00000010 */

/*******************  Bit definition for ADC_SQR2 register  *******************/
#define ADC_SQR2_SQ19_Pos                    (0U)                              
#define ADC_SQR2_SQ19_Msk                    (0x1FU << ADC_SQR2_SQ19_Pos)      /*!< 0x0000001F */
#define ADC_SQR2_SQ19                        ADC_SQR2_SQ19_Msk                 /*!< ADC group regular sequencer rank 19 */
#define ADC_SQR2_SQ19_0                      (0x01U << ADC_SQR2_SQ19_Pos)      /*!< 0x00000001 */
#define ADC_SQR2_SQ19_1                      (0x02U << ADC_SQR2_SQ19_Pos)      /*!< 0x00000002 */
#define ADC_SQR2_SQ19_2                      (0x04U << ADC_SQR2_SQ19_Pos)      /*!< 0x00000004 */
#define ADC_SQR2_SQ19_3                      (0x08U << ADC_SQR2_SQ19_Pos)      /*!< 0x00000008 */
#define ADC_SQR2_SQ19_4                      (0x10U << ADC_SQR2_SQ19_Pos)      /*!< 0x00000010 */

#define ADC_SQR2_SQ20_Pos                    (5U)                              
#define ADC_SQR2_SQ20_Msk                    (0x1FU << ADC_SQR2_SQ20_Pos)      /*!< 0x000003E0 */
#define ADC_SQR2_SQ20                        ADC_SQR2_SQ20_Msk                 /*!< ADC group regular sequencer rank 20 */
#define ADC_SQR2_SQ20_0                      (0x01U << ADC_SQR2_SQ20_Pos)      /*!< 0x00000020 */
#define ADC_SQR2_SQ20_1                      (0x02U << ADC_SQR2_SQ20_Pos)      /*!< 0x00000040 */
#define ADC_SQR2_SQ20_2                      (0x04U << ADC_SQR2_SQ20_Pos)      /*!< 0x00000080 */
#define ADC_SQR2_SQ20_3                      (0x08U << ADC_SQR2_SQ20_Pos)      /*!< 0x00000100 */
#define ADC_SQR2_SQ20_4                      (0x10U << ADC_SQR2_SQ20_Pos)      /*!< 0x00000200 */

#define ADC_SQR2_SQ21_Pos                    (10U)                             
#define ADC_SQR2_SQ21_Msk                    (0x1FU << ADC_SQR2_SQ21_Pos)      /*!< 0x00007C00 */
#define ADC_SQR2_SQ21                        ADC_SQR2_SQ21_Msk                 /*!< ADC group regular sequencer rank 21 */
#define ADC_SQR2_SQ21_0                      (0x01U << ADC_SQR2_SQ21_Pos)      /*!< 0x00000400 */
#define ADC_SQR2_SQ21_1                      (0x02U << ADC_SQR2_SQ21_Pos)      /*!< 0x00000800 */
#define ADC_SQR2_SQ21_2                      (0x04U << ADC_SQR2_SQ21_Pos)      /*!< 0x00001000 */
#define ADC_SQR2_SQ21_3                      (0x08U << ADC_SQR2_SQ21_Pos)      /*!< 0x00002000 */
#define ADC_SQR2_SQ21_4                      (0x10U << ADC_SQR2_SQ21_Pos)      /*!< 0x00004000 */

#define ADC_SQR2_SQ22_Pos                    (15U)                             
#define ADC_SQR2_SQ22_Msk                    (0x1FU << ADC_SQR2_SQ22_Pos)      /*!< 0x000F8000 */
#define ADC_SQR2_SQ22                        ADC_SQR2_SQ22_Msk                 /*!< ADC group regular sequencer rank 22 */
#define ADC_SQR2_SQ22_0                      (0x01U << ADC_SQR2_SQ22_Pos)      /*!< 0x00008000 */
#define ADC_SQR2_SQ22_1                      (0x02U << ADC_SQR2_SQ22_Pos)      /*!< 0x00010000 */
#define ADC_SQR2_SQ22_2                      (0x04U << ADC_SQR2_SQ22_Pos)      /*!< 0x00020000 */
#define ADC_SQR2_SQ22_3                      (0x08U << ADC_SQR2_SQ22_Pos)      /*!< 0x00040000 */
#define ADC_SQR2_SQ22_4                      (0x10U << ADC_SQR2_SQ22_Pos)      /*!< 0x00080000 */

#define ADC_SQR2_SQ23_Pos                    (20U)                             
#define ADC_SQR2_SQ23_Msk                    (0x1FU << ADC_SQR2_SQ23_Pos)      /*!< 0x01F00000 */
#define ADC_SQR2_SQ23                        ADC_SQR2_SQ23_Msk                 /*!< ADC group regular sequencer rank 23 */
#define ADC_SQR2_SQ23_0                      (0x01U << ADC_SQR2_SQ23_Pos)      /*!< 0x00100000 */
#define ADC_SQR2_SQ23_1                      (0x02U << ADC_SQR2_SQ23_Pos)      /*!< 0x00200000 */
#define ADC_SQR2_SQ23_2                      (0x04U << ADC_SQR2_SQ23_Pos)      /*!< 0x00400000 */
#define ADC_SQR2_SQ23_3                      (0x08U << ADC_SQR2_SQ23_Pos)      /*!< 0x00800000 */
#define ADC_SQR2_SQ23_4                      (0x10U << ADC_SQR2_SQ23_Pos)      /*!< 0x01000000 */

#define ADC_SQR2_SQ24_Pos                    (25U)                             
#define ADC_SQR2_SQ24_Msk                    (0x1FU << ADC_SQR2_SQ24_Pos)      /*!< 0x3E000000 */
#define ADC_SQR2_SQ24                        ADC_SQR2_SQ24_Msk                 /*!< ADC group regular sequencer rank 24 */
#define ADC_SQR2_SQ24_0                      (0x01U << ADC_SQR2_SQ24_Pos)      /*!< 0x02000000 */
#define ADC_SQR2_SQ24_1                      (0x02U << ADC_SQR2_SQ24_Pos)      /*!< 0x04000000 */
#define ADC_SQR2_SQ24_2                      (0x04U << ADC_SQR2_SQ24_Pos)      /*!< 0x08000000 */
#define ADC_SQR2_SQ24_3                      (0x08U << ADC_SQR2_SQ24_Pos)      /*!< 0x10000000 */
#define ADC_SQR2_SQ24_4                      (0x10U << ADC_SQR2_SQ24_Pos)      /*!< 0x20000000 */

/*******************  Bit definition for ADC_SQR3 register  *******************/
#define ADC_SQR3_SQ13_Pos                    (0U)                              
#define ADC_SQR3_SQ13_Msk                    (0x1FU << ADC_SQR3_SQ13_Pos)      /*!< 0x0000001F */
#define ADC_SQR3_SQ13                        ADC_SQR3_SQ13_Msk                 /*!< ADC group regular sequencer rank 13 */
#define ADC_SQR3_SQ13_0                      (0x01U << ADC_SQR3_SQ13_Pos)      /*!< 0x00000001 */
#define ADC_SQR3_SQ13_1                      (0x02U << ADC_SQR3_SQ13_Pos)      /*!< 0x00000002 */
#define ADC_SQR3_SQ13_2                      (0x04U << ADC_SQR3_SQ13_Pos)      /*!< 0x00000004 */
#define ADC_SQR3_SQ13_3                      (0x08U << ADC_SQR3_SQ13_Pos)      /*!< 0x00000008 */
#define ADC_SQR3_SQ13_4                      (0x10U << ADC_SQR3_SQ13_Pos)      /*!< 0x00000010 */

#define ADC_SQR3_SQ14_Pos                    (5U)                              
#define ADC_SQR3_SQ14_Msk                    (0x1FU << ADC_SQR3_SQ14_Pos)      /*!< 0x000003E0 */
#define ADC_SQR3_SQ14                        ADC_SQR3_SQ14_Msk                 /*!< ADC group regular sequencer rank 14 */
#define ADC_SQR3_SQ14_0                      (0x01U << ADC_SQR3_SQ14_Pos)      /*!< 0x00000020 */
#define ADC_SQR3_SQ14_1                      (0x02U << ADC_SQR3_SQ14_Pos)      /*!< 0x00000040 */
#define ADC_SQR3_SQ14_2                      (0x04U << ADC_SQR3_SQ14_Pos)      /*!< 0x00000080 */
#define ADC_SQR3_SQ14_3                      (0x08U << ADC_SQR3_SQ14_Pos)      /*!< 0x00000100 */
#define ADC_SQR3_SQ14_4                      (0x10U << ADC_SQR3_SQ14_Pos)      /*!< 0x00000200 */

#define ADC_SQR3_SQ15_Pos                    (10U)                             
#define ADC_SQR3_SQ15_Msk                    (0x1FU << ADC_SQR3_SQ15_Pos)      /*!< 0x00007C00 */
#define ADC_SQR3_SQ15                        ADC_SQR3_SQ15_Msk                 /*!< ADC group regular sequencer rank 15 */
#define ADC_SQR3_SQ15_0                      (0x01U << ADC_SQR3_SQ15_Pos)      /*!< 0x00000400 */
#define ADC_SQR3_SQ15_1                      (0x02U << ADC_SQR3_SQ15_Pos)      /*!< 0x00000800 */
#define ADC_SQR3_SQ15_2                      (0x04U << ADC_SQR3_SQ15_Pos)      /*!< 0x00001000 */
#define ADC_SQR3_SQ15_3                      (0x08U << ADC_SQR3_SQ15_Pos)      /*!< 0x00002000 */
#define ADC_SQR3_SQ15_4                      (0x10U << ADC_SQR3_SQ15_Pos)      /*!< 0x00004000 */

#define ADC_SQR3_SQ16_Pos                    (15U)                             
#define ADC_SQR3_SQ16_Msk                    (0x1FU << ADC_SQR3_SQ16_Pos)      /*!< 0x000F8000 */
#define ADC_SQR3_SQ16                        ADC_SQR3_SQ16_Msk                 /*!< ADC group regular sequencer rank 16 */
#define ADC_SQR3_SQ16_0                      (0x01U << ADC_SQR3_SQ16_Pos)      /*!< 0x00008000 */
#define ADC_SQR3_SQ16_1                      (0x02U << ADC_SQR3_SQ16_Pos)      /*!< 0x00010000 */
#define ADC_SQR3_SQ16_2                      (0x04U << ADC_SQR3_SQ16_Pos)      /*!< 0x00020000 */
#define ADC_SQR3_SQ16_3                      (0x08U << ADC_SQR3_SQ16_Pos)      /*!< 0x00040000 */
#define ADC_SQR3_SQ16_4                      (0x10U << ADC_SQR3_SQ16_Pos)      /*!< 0x00080000 */

#define ADC_SQR3_SQ17_Pos                    (20U)                             
#define ADC_SQR3_SQ17_Msk                    (0x1FU << ADC_SQR3_SQ17_Pos)      /*!< 0x01F00000 */
#define ADC_SQR3_SQ17                        ADC_SQR3_SQ17_Msk                 /*!< ADC group regular sequencer rank 17 */
#define ADC_SQR3_SQ17_0                      (0x01U << ADC_SQR3_SQ17_Pos)      /*!< 0x00100000 */
#define ADC_SQR3_SQ17_1                      (0x02U << ADC_SQR3_SQ17_Pos)      /*!< 0x00200000 */
#define ADC_SQR3_SQ17_2                      (0x04U << ADC_SQR3_SQ17_Pos)      /*!< 0x00400000 */
#define ADC_SQR3_SQ17_3                      (0x08U << ADC_SQR3_SQ17_Pos)      /*!< 0x00800000 */
#define ADC_SQR3_SQ17_4                      (0x10U << ADC_SQR3_SQ17_Pos)      /*!< 0x01000000 */

#define ADC_SQR3_SQ18_Pos                    (25U)                             
#define ADC_SQR3_SQ18_Msk                    (0x1FU << ADC_SQR3_SQ18_Pos)      /*!< 0x3E000000 */
#define ADC_SQR3_SQ18                        ADC_SQR3_SQ18_Msk                 /*!< ADC group regular sequencer rank 18 */
#define ADC_SQR3_SQ18_0                      (0x01U << ADC_SQR3_SQ18_Pos)      /*!< 0x02000000 */
#define ADC_SQR3_SQ18_1                      (0x02U << ADC_SQR3_SQ18_Pos)      /*!< 0x04000000 */
#define ADC_SQR3_SQ18_2                      (0x04U << ADC_SQR3_SQ18_Pos)      /*!< 0x08000000 */
#define ADC_SQR3_SQ18_3                      (0x08U << ADC_SQR3_SQ18_Pos)      /*!< 0x10000000 */
#define ADC_SQR3_SQ18_4                      (0x10U << ADC_SQR3_SQ18_Pos)      /*!< 0x20000000 */

/*******************  Bit definition for ADC_SQR4 register  *******************/
#define ADC_SQR4_SQ7_Pos                     (0U)                              
#define ADC_SQR4_SQ7_Msk                     (0x1FU << ADC_SQR4_SQ7_Pos)       /*!< 0x0000001F */
#define ADC_SQR4_SQ7                         ADC_SQR4_SQ7_Msk                  /*!< ADC group regular sequencer rank 7 */
#define ADC_SQR4_SQ7_0                       (0x01U << ADC_SQR4_SQ7_Pos)       /*!< 0x00000001 */
#define ADC_SQR4_SQ7_1                       (0x02U << ADC_SQR4_SQ7_Pos)       /*!< 0x00000002 */
#define ADC_SQR4_SQ7_2                       (0x04U << ADC_SQR4_SQ7_Pos)       /*!< 0x00000004 */
#define ADC_SQR4_SQ7_3                       (0x08U << ADC_SQR4_SQ7_Pos)       /*!< 0x00000008 */
#define ADC_SQR4_SQ7_4                       (0x10U << ADC_SQR4_SQ7_Pos)       /*!< 0x00000010 */

#define ADC_SQR4_SQ8_Pos                     (5U)                              
#define ADC_SQR4_SQ8_Msk                     (0x1FU << ADC_SQR4_SQ8_Pos)       /*!< 0x000003E0 */
#define ADC_SQR4_SQ8                         ADC_SQR4_SQ8_Msk                  /*!< ADC group regular sequencer rank 8 */
#define ADC_SQR4_SQ8_0                       (0x01U << ADC_SQR4_SQ8_Pos)       /*!< 0x00000020 */
#define ADC_SQR4_SQ8_1                       (0x02U << ADC_SQR4_SQ8_Pos)       /*!< 0x00000040 */
#define ADC_SQR4_SQ8_2                       (0x04U << ADC_SQR4_SQ8_Pos)       /*!< 0x00000080 */
#define ADC_SQR4_SQ8_3                       (0x08U << ADC_SQR4_SQ8_Pos)       /*!< 0x00000100 */
#define ADC_SQR4_SQ8_4                       (0x10U << ADC_SQR4_SQ8_Pos)       /*!< 0x00000200 */

#define ADC_SQR4_SQ9_Pos                     (10U)                             
#define ADC_SQR4_SQ9_Msk                     (0x1FU << ADC_SQR4_SQ9_Pos)       /*!< 0x00007C00 */
#define ADC_SQR4_SQ9                         ADC_SQR4_SQ9_Msk                  /*!< ADC group regular sequencer rank 9 */
#define ADC_SQR4_SQ9_0                       (0x01U << ADC_SQR4_SQ9_Pos)       /*!< 0x00000400 */
#define ADC_SQR4_SQ9_1                       (0x02U << ADC_SQR4_SQ9_Pos)       /*!< 0x00000800 */
#define ADC_SQR4_SQ9_2                       (0x04U << ADC_SQR4_SQ9_Pos)       /*!< 0x00001000 */
#define ADC_SQR4_SQ9_3                       (0x08U << ADC_SQR4_SQ9_Pos)       /*!< 0x00002000 */
#define ADC_SQR4_SQ9_4                       (0x10U << ADC_SQR4_SQ9_Pos)       /*!< 0x00004000 */

#define ADC_SQR4_SQ10_Pos                    (15U)                             
#define ADC_SQR4_SQ10_Msk                    (0x1FU << ADC_SQR4_SQ10_Pos)      /*!< 0x000F8000 */
#define ADC_SQR4_SQ10                        ADC_SQR4_SQ10_Msk                 /*!< ADC group regular sequencer rank 10 */
#define ADC_SQR4_SQ10_0                      (0x01U << ADC_SQR4_SQ10_Pos)      /*!< 0x00008000 */
#define ADC_SQR4_SQ10_1                      (0x02U << ADC_SQR4_SQ10_Pos)      /*!< 0x00010000 */
#define ADC_SQR4_SQ10_2                      (0x04U << ADC_SQR4_SQ10_Pos)      /*!< 0x00020000 */
#define ADC_SQR4_SQ10_3                      (0x08U << ADC_SQR4_SQ10_Pos)      /*!< 0x00040000 */
#define ADC_SQR4_SQ10_4                      (0x10U << ADC_SQR4_SQ10_Pos)      /*!< 0x00080000 */

#define ADC_SQR4_SQ11_Pos                    (20U)                             
#define ADC_SQR4_SQ11_Msk                    (0x1FU << ADC_SQR4_SQ11_Pos)      /*!< 0x01F00000 */
#define ADC_SQR4_SQ11                        ADC_SQR4_SQ11_Msk                 /*!< ADC group regular sequencer rank 11 */
#define ADC_SQR4_SQ11_0                      (0x01U << ADC_SQR4_SQ11_Pos)      /*!< 0x00100000 */
#define ADC_SQR4_SQ11_1                      (0x02U << ADC_SQR4_SQ11_Pos)      /*!< 0x00200000 */
#define ADC_SQR4_SQ11_2                      (0x04U << ADC_SQR4_SQ11_Pos)      /*!< 0x00400000 */
#define ADC_SQR4_SQ11_3                      (0x08U << ADC_SQR4_SQ11_Pos)      /*!< 0x00800000 */
#define ADC_SQR4_SQ11_4                      (0x10U << ADC_SQR4_SQ11_Pos)      /*!< 0x01000000 */

#define ADC_SQR4_SQ12_Pos                    (25U)                             
#define ADC_SQR4_SQ12_Msk                    (0x1FU << ADC_SQR4_SQ12_Pos)      /*!< 0x3E000000 */
#define ADC_SQR4_SQ12                        ADC_SQR4_SQ12_Msk                 /*!< ADC group regular sequencer rank 12 */
#define ADC_SQR4_SQ12_0                      (0x01U << ADC_SQR4_SQ12_Pos)      /*!< 0x02000000 */
#define ADC_SQR4_SQ12_1                      (0x02U << ADC_SQR4_SQ12_Pos)      /*!< 0x04000000 */
#define ADC_SQR4_SQ12_2                      (0x04U << ADC_SQR4_SQ12_Pos)      /*!< 0x08000000 */
#define ADC_SQR4_SQ12_3                      (0x08U << ADC_SQR4_SQ12_Pos)      /*!< 0x10000000 */
#define ADC_SQR4_SQ12_4                      (0x10U << ADC_SQR4_SQ12_Pos)      /*!< 0x20000000 */

/*******************  Bit definition for ADC_SQR5 register  *******************/
#define ADC_SQR5_SQ1_Pos                     (0U)                              
#define ADC_SQR5_SQ1_Msk                     (0x1FU << ADC_SQR5_SQ1_Pos)       /*!< 0x0000001F */
#define ADC_SQR5_SQ1                         ADC_SQR5_SQ1_Msk                  /*!< ADC group regular sequencer rank 1 */
#define ADC_SQR5_SQ1_0                       (0x01U << ADC_SQR5_SQ1_Pos)       /*!< 0x00000001 */
#define ADC_SQR5_SQ1_1                       (0x02U << ADC_SQR5_SQ1_Pos)       /*!< 0x00000002 */
#define ADC_SQR5_SQ1_2                       (0x04U << ADC_SQR5_SQ1_Pos)       /*!< 0x00000004 */
#define ADC_SQR5_SQ1_3                       (0x08U << ADC_SQR5_SQ1_Pos)       /*!< 0x00000008 */
#define ADC_SQR5_SQ1_4                       (0x10U << ADC_SQR5_SQ1_Pos)       /*!< 0x00000010 */

#define ADC_SQR5_SQ2_Pos                     (5U)                              
#define ADC_SQR5_SQ2_Msk                     (0x1FU << ADC_SQR5_SQ2_Pos)       /*!< 0x000003E0 */
#define ADC_SQR5_SQ2                         ADC_SQR5_SQ2_Msk                  /*!< ADC group regular sequencer rank 2 */
#define ADC_SQR5_SQ2_0                       (0x01U << ADC_SQR5_SQ2_Pos)       /*!< 0x00000020 */
#define ADC_SQR5_SQ2_1                       (0x02U << ADC_SQR5_SQ2_Pos)       /*!< 0x00000040 */
#define ADC_SQR5_SQ2_2                       (0x04U << ADC_SQR5_SQ2_Pos)       /*!< 0x00000080 */
#define ADC_SQR5_SQ2_3                       (0x08U << ADC_SQR5_SQ2_Pos)       /*!< 0x00000100 */
#define ADC_SQR5_SQ2_4                       (0x10U << ADC_SQR5_SQ2_Pos)       /*!< 0x00000200 */

#define ADC_SQR5_SQ3_Pos                     (10U)                             
#define ADC_SQR5_SQ3_Msk                     (0x1FU << ADC_SQR5_SQ3_Pos)       /*!< 0x00007C00 */
#define ADC_SQR5_SQ3                         ADC_SQR5_SQ3_Msk                  /*!< ADC group regular sequencer rank 3 */
#define ADC_SQR5_SQ3_0                       (0x01U << ADC_SQR5_SQ3_Pos)       /*!< 0x00000400 */
#define ADC_SQR5_SQ3_1                       (0x02U << ADC_SQR5_SQ3_Pos)       /*!< 0x00000800 */
#define ADC_SQR5_SQ3_2                       (0x04U << ADC_SQR5_SQ3_Pos)       /*!< 0x00001000 */
#define ADC_SQR5_SQ3_3                       (0x08U << ADC_SQR5_SQ3_Pos)       /*!< 0x00002000 */
#define ADC_SQR5_SQ3_4                       (0x10U << ADC_SQR5_SQ3_Pos)       /*!< 0x00004000 */

#define ADC_SQR5_SQ4_Pos                     (15U)                             
#define ADC_SQR5_SQ4_Msk                     (0x1FU << ADC_SQR5_SQ4_Pos)       /*!< 0x000F8000 */
#define ADC_SQR5_SQ4                         ADC_SQR5_SQ4_Msk                  /*!< ADC group regular sequencer rank 4 */
#define ADC_SQR5_SQ4_0                       (0x01U << ADC_SQR5_SQ4_Pos)       /*!< 0x00008000 */
#define ADC_SQR5_SQ4_1                       (0x02U << ADC_SQR5_SQ4_Pos)       /*!< 0x00010000 */
#define ADC_SQR5_SQ4_2                       (0x04U << ADC_SQR5_SQ4_Pos)       /*!< 0x00020000 */
#define ADC_SQR5_SQ4_3                       (0x08U << ADC_SQR5_SQ4_Pos)       /*!< 0x00040000 */
#define ADC_SQR5_SQ4_4                       (0x10U << ADC_SQR5_SQ4_Pos)       /*!< 0x00080000 */

#define ADC_SQR5_SQ5_Pos                     (20U)                             
#define ADC_SQR5_SQ5_Msk                     (0x1FU << ADC_SQR5_SQ5_Pos)       /*!< 0x01F00000 */
#define ADC_SQR5_SQ5                         ADC_SQR5_SQ5_Msk                  /*!< ADC group regular sequencer rank 5 */
#define ADC_SQR5_SQ5_0                       (0x01U << ADC_SQR5_SQ5_Pos)       /*!< 0x00100000 */
#define ADC_SQR5_SQ5_1                       (0x02U << ADC_SQR5_SQ5_Pos)       /*!< 0x00200000 */
#define ADC_SQR5_SQ5_2                       (0x04U << ADC_SQR5_SQ5_Pos)       /*!< 0x00400000 */
#define ADC_SQR5_SQ5_3                       (0x08U << ADC_SQR5_SQ5_Pos)       /*!< 0x00800000 */
#define ADC_SQR5_SQ5_4                       (0x10U << ADC_SQR5_SQ5_Pos)       /*!< 0x01000000 */

#define ADC_SQR5_SQ6_Pos                     (25U)                             
#define ADC_SQR5_SQ6_Msk                     (0x1FU << ADC_SQR5_SQ6_Pos)       /*!< 0x3E000000 */
#define ADC_SQR5_SQ6                         ADC_SQR5_SQ6_Msk                  /*!< ADC group regular sequencer rank 6 */
#define ADC_SQR5_SQ6_0                       (0x01U << ADC_SQR5_SQ6_Pos)       /*!< 0x02000000 */
#define ADC_SQR5_SQ6_1                       (0x02U << ADC_SQR5_SQ6_Pos)       /*!< 0x04000000 */
#define ADC_SQR5_SQ6_2                       (0x04U << ADC_SQR5_SQ6_Pos)       /*!< 0x08000000 */
#define ADC_SQR5_SQ6_3                       (0x08U << ADC_SQR5_SQ6_Pos)       /*!< 0x10000000 */
#define ADC_SQR5_SQ6_4                       (0x10U << ADC_SQR5_SQ6_Pos)       /*!< 0x20000000 */


/*******************  Bit definition for ADC_JSQR register  *******************/
#define ADC_JSQR_JSQ1_Pos                    (0U)                              
#define ADC_JSQR_JSQ1_Msk                    (0x1FU << ADC_JSQR_JSQ1_Pos)      /*!< 0x0000001F */
#define ADC_JSQR_JSQ1                        ADC_JSQR_JSQ1_Msk                 /*!< ADC group injected sequencer rank 1 */
#define ADC_JSQR_JSQ1_0                      (0x01U << ADC_JSQR_JSQ1_Pos)      /*!< 0x00000001 */
#define ADC_JSQR_JSQ1_1                      (0x02U << ADC_JSQR_JSQ1_Pos)      /*!< 0x00000002 */
#define ADC_JSQR_JSQ1_2                      (0x04U << ADC_JSQR_JSQ1_Pos)      /*!< 0x00000004 */
#define ADC_JSQR_JSQ1_3                      (0x08U << ADC_JSQR_JSQ1_Pos)      /*!< 0x00000008 */
#define ADC_JSQR_JSQ1_4                      (0x10U << ADC_JSQR_JSQ1_Pos)      /*!< 0x00000010 */

#define ADC_JSQR_JSQ2_Pos                    (5U)                              
#define ADC_JSQR_JSQ2_Msk                    (0x1FU << ADC_JSQR_JSQ2_Pos)      /*!< 0x000003E0 */
#define ADC_JSQR_JSQ2                        ADC_JSQR_JSQ2_Msk                 /*!< ADC group injected sequencer rank 2 */
#define ADC_JSQR_JSQ2_0                      (0x01U << ADC_JSQR_JSQ2_Pos)      /*!< 0x00000020 */
#define ADC_JSQR_JSQ2_1                      (0x02U << ADC_JSQR_JSQ2_Pos)      /*!< 0x00000040 */
#define ADC_JSQR_JSQ2_2                      (0x04U << ADC_JSQR_JSQ2_Pos)      /*!< 0x00000080 */
#define ADC_JSQR_JSQ2_3                      (0x08U << ADC_JSQR_JSQ2_Pos)      /*!< 0x00000100 */
#define ADC_JSQR_JSQ2_4                      (0x10U << ADC_JSQR_JSQ2_Pos)      /*!< 0x00000200 */

#define ADC_JSQR_JSQ3_Pos                    (10U)                             
#define ADC_JSQR_JSQ3_Msk                    (0x1FU << ADC_JSQR_JSQ3_Pos)      /*!< 0x00007C00 */
#define ADC_JSQR_JSQ3                        ADC_JSQR_JSQ3_Msk                 /*!< ADC group injected sequencer rank 3 */
#define ADC_JSQR_JSQ3_0                      (0x01U << ADC_JSQR_JSQ3_Pos)      /*!< 0x00000400 */
#define ADC_JSQR_JSQ3_1                      (0x02U << ADC_JSQR_JSQ3_Pos)      /*!< 0x00000800 */
#define ADC_JSQR_JSQ3_2                      (0x04U << ADC_JSQR_JSQ3_Pos)      /*!< 0x00001000 */
#define ADC_JSQR_JSQ3_3                      (0x08U << ADC_JSQR_JSQ3_Pos)      /*!< 0x00002000 */
#define ADC_JSQR_JSQ3_4                      (0x10U << ADC_JSQR_JSQ3_Pos)      /*!< 0x00004000 */

#define ADC_JSQR_JSQ4_Pos                    (15U)                             
#define ADC_JSQR_JSQ4_Msk                    (0x1FU << ADC_JSQR_JSQ4_Pos)      /*!< 0x000F8000 */
#define ADC_JSQR_JSQ4                        ADC_JSQR_JSQ4_Msk                 /*!< ADC group injected sequencer rank 4 */
#define ADC_JSQR_JSQ4_0                      (0x01U << ADC_JSQR_JSQ4_Pos)      /*!< 0x00008000 */
#define ADC_JSQR_JSQ4_1                      (0x02U << ADC_JSQR_JSQ4_Pos)      /*!< 0x00010000 */
#define ADC_JSQR_JSQ4_2                      (0x04U << ADC_JSQR_JSQ4_Pos)      /*!< 0x00020000 */
#define ADC_JSQR_JSQ4_3                      (0x08U << ADC_JSQR_JSQ4_Pos)      /*!< 0x00040000 */
#define ADC_JSQR_JSQ4_4                      (0x10U << ADC_JSQR_JSQ4_Pos)      /*!< 0x00080000 */

#define ADC_JSQR_JL_Pos                      (20U)                             
#define ADC_JSQR_JL_Msk                      (0x3U << ADC_JSQR_JL_Pos)         /*!< 0x00300000 */
#define ADC_JSQR_JL                          ADC_JSQR_JL_Msk                   /*!< ADC group injected sequencer scan length */
#define ADC_JSQR_JL_0                        (0x1U << ADC_JSQR_JL_Pos)         /*!< 0x00100000 */
#define ADC_JSQR_JL_1                        (0x2U << ADC_JSQR_JL_Pos)         /*!< 0x00200000 */

/*******************  Bit definition for ADC_JDR1 register  *******************/
#define ADC_JDR1_JDATA_Pos                   (0U)                              
#define ADC_JDR1_JDATA_Msk                   (0xFFFFU << ADC_JDR1_JDATA_Pos)   /*!< 0x0000FFFF */
#define ADC_JDR1_JDATA                       ADC_JDR1_JDATA_Msk                /*!< ADC group injected sequencer rank 1 conversion data */

/*******************  Bit definition for ADC_JDR2 register  *******************/
#define ADC_JDR2_JDATA_Pos                   (0U)                              
#define ADC_JDR2_JDATA_Msk                   (0xFFFFU << ADC_JDR2_JDATA_Pos)   /*!< 0x0000FFFF */
#define ADC_JDR2_JDATA                       ADC_JDR2_JDATA_Msk                /*!< ADC group injected sequencer rank 2 conversion data */

/*******************  Bit definition for ADC_JDR3 register  *******************/
#define ADC_JDR3_JDATA_Pos                   (0U)                              
#define ADC_JDR3_JDATA_Msk                   (0xFFFFU << ADC_JDR3_JDATA_Pos)   /*!< 0x0000FFFF */
#define ADC_JDR3_JDATA                       ADC_JDR3_JDATA_Msk                /*!< ADC group injected sequencer rank 3 conversion data */

/*******************  Bit definition for ADC_JDR4 register  *******************/
#define ADC_JDR4_JDATA_Pos                   (0U)                              
#define ADC_JDR4_JDATA_Msk                   (0xFFFFU << ADC_JDR4_JDATA_Pos)   /*!< 0x0000FFFF */
#define ADC_JDR4_JDATA                       ADC_JDR4_JDATA_Msk                /*!< ADC group injected sequencer rank 4 conversion data */

/********************  Bit definition for ADC_DR register  ********************/
#define ADC_DR_DATA_Pos                      (0U)                              
#define ADC_DR_DATA_Msk                      (0xFFFFU << ADC_DR_DATA_Pos)      /*!< 0x0000FFFF */
#define ADC_DR_DATA                          ADC_DR_DATA_Msk                   /*!< ADC group regular conversion data */

/******************  Bit definition for ADC_SMPR0 register  *******************/
#define ADC_SMPR0_SMP30_Pos                  (0U)                              
#define ADC_SMPR0_SMP30_Msk                  (0x7U << ADC_SMPR0_SMP30_Pos)     /*!< 0x00000007 */
#define ADC_SMPR0_SMP30                      ADC_SMPR0_SMP30_Msk               /*!< ADC channel 30 sampling time selection */
#define ADC_SMPR0_SMP30_0                    (0x1U << ADC_SMPR0_SMP30_Pos)     /*!< 0x00000001 */
#define ADC_SMPR0_SMP30_1                    (0x2U << ADC_SMPR0_SMP30_Pos)     /*!< 0x00000002 */
#define ADC_SMPR0_SMP30_2                    (0x4U << ADC_SMPR0_SMP30_Pos)     /*!< 0x00000004 */
 
#define ADC_SMPR0_SMP31_Pos                  (3U)                              
#define ADC_SMPR0_SMP31_Msk                  (0x7U << ADC_SMPR0_SMP31_Pos)     /*!< 0x00000038 */
#define ADC_SMPR0_SMP31                      ADC_SMPR0_SMP31_Msk               /*!< ADC channel 31 sampling time selection */
#define ADC_SMPR0_SMP31_0                    (0x1U << ADC_SMPR0_SMP31_Pos)     /*!< 0x00000008 */
#define ADC_SMPR0_SMP31_1                    (0x2U << ADC_SMPR0_SMP31_Pos)     /*!< 0x00000010 */
#define ADC_SMPR0_SMP31_2                    (0x4U << ADC_SMPR0_SMP31_Pos)     /*!< 0x00000020 */

/*******************  Bit definition for ADC_CSR register  ********************/
#define ADC_CSR_AWD1_Pos                     (0U)                              
#define ADC_CSR_AWD1_Msk                     (0x1U << ADC_CSR_AWD1_Pos)        /*!< 0x00000001 */
#define ADC_CSR_AWD1                         ADC_CSR_AWD1_Msk                  /*!< ADC multimode master analog watchdog 1 flag */
#define ADC_CSR_EOCS1_Pos                    (1U)                              
#define ADC_CSR_EOCS1_Msk                    (0x1U << ADC_CSR_EOCS1_Pos)       /*!< 0x00000002 */
#define ADC_CSR_EOCS1                        ADC_CSR_EOCS1_Msk                 /*!< ADC multimode master group regular end of unitary conversion or end of sequence conversions flag */
#define ADC_CSR_JEOS1_Pos                    (2U)                              
#define ADC_CSR_JEOS1_Msk                    (0x1U << ADC_CSR_JEOS1_Pos)       /*!< 0x00000004 */
#define ADC_CSR_JEOS1                        ADC_CSR_JEOS1_Msk                 /*!< ADC multimode master group injected end of sequence conversions flag */
#define ADC_CSR_JSTRT1_Pos                   (3U)                              
#define ADC_CSR_JSTRT1_Msk                   (0x1U << ADC_CSR_JSTRT1_Pos)      /*!< 0x00000008 */
#define ADC_CSR_JSTRT1                       ADC_CSR_JSTRT1_Msk                /*!< ADC multimode master group injected conversion start flag */
#define ADC_CSR_STRT1_Pos                    (4U)                              
#define ADC_CSR_STRT1_Msk                    (0x1U << ADC_CSR_STRT1_Pos)       /*!< 0x00000010 */
#define ADC_CSR_STRT1                        ADC_CSR_STRT1_Msk                 /*!< ADC multimode master group regular conversion start flag */
#define ADC_CSR_OVR1_Pos                     (5U)                              
#define ADC_CSR_OVR1_Msk                     (0x1U << ADC_CSR_OVR1_Pos)        /*!< 0x00000020 */
#define ADC_CSR_OVR1                         ADC_CSR_OVR1_Msk                  /*!< ADC multimode master group regular overrun flag */
#define ADC_CSR_ADONS1_Pos                   (6U)                              
#define ADC_CSR_ADONS1_Msk                   (0x1U << ADC_CSR_ADONS1_Pos)      /*!< 0x00000040 */
#define ADC_CSR_ADONS1                       ADC_CSR_ADONS1_Msk                /*!< ADC multimode master ready flag */

/* Legacy defines */
#define  ADC_CSR_EOC1                        (ADC_CSR_EOCS1)
#define  ADC_CSR_JEOC1                       (ADC_CSR_JEOS1)

/*******************  Bit definition for ADC_CCR register  ********************/
#define ADC_CCR_ADCPRE_Pos                   (16U)                             
#define ADC_CCR_ADCPRE_Msk                   (0x3U << ADC_CCR_ADCPRE_Pos)      /*!< 0x00030000 */
#define ADC_CCR_ADCPRE                       ADC_CCR_ADCPRE_Msk                /*!< ADC clock source asynchronous prescaler */
#define ADC_CCR_ADCPRE_0                     (0x1U << ADC_CCR_ADCPRE_Pos)      /*!< 0x00010000 */
#define ADC_CCR_ADCPRE_1                     (0x2U << ADC_CCR_ADCPRE_Pos)      /*!< 0x00020000 */
#define ADC_CCR_TSVREFE_Pos                  (23U)                             
#define ADC_CCR_TSVREFE_Msk                  (0x1U << ADC_CCR_TSVREFE_Pos)     /*!< 0x00800000 */
#define ADC_CCR_TSVREFE                      ADC_CCR_TSVREFE_Msk               /*!< ADC internal path to VrefInt and temperature sensor enable */

/******************************************************************************/
/*                                                                            */
/*                      Analog Comparators (COMP)                             */
/*                                                                            */
/******************************************************************************/

/******************  Bit definition for COMP_CSR register  ********************/
#define COMP_CSR_10KPU                      (0x00000001U)                      /*!< Comparator 1 input plus 10K pull-up resistor */
#define COMP_CSR_400KPU                     (0x00000002U)                      /*!< Comparator 1 input plus 400K pull-up resistor */
#define COMP_CSR_10KPD                      (0x00000004U)                      /*!< Comparator 1 input plus 10K pull-down resistor */
#define COMP_CSR_400KPD                     (0x00000008U)                      /*!< Comparator 1 input plus 400K pull-down resistor */
#define COMP_CSR_CMP1EN_Pos                 (4U)                               
#define COMP_CSR_CMP1EN_Msk                 (0x1U << COMP_CSR_CMP1EN_Pos)      /*!< 0x00000010 */
#define COMP_CSR_CMP1EN                     COMP_CSR_CMP1EN_Msk                /*!< Comparator 1 enable */
#define COMP_CSR_CMP1OUT_Pos                (7U)                               
#define COMP_CSR_CMP1OUT_Msk                (0x1U << COMP_CSR_CMP1OUT_Pos)     /*!< 0x00000080 */
#define COMP_CSR_CMP1OUT                    COMP_CSR_CMP1OUT_Msk               /*!< Comparator 1 output level */
#define COMP_CSR_SPEED_Pos                  (12U)                              
#define COMP_CSR_SPEED_Msk                  (0x1U << COMP_CSR_SPEED_Pos)       /*!< 0x00001000 */
#define COMP_CSR_SPEED                      COMP_CSR_SPEED_Msk                 /*!< Comparator 2 power mode */
#define COMP_CSR_CMP2OUT_Pos                (13U)                              
#define COMP_CSR_CMP2OUT_Msk                (0x1U << COMP_CSR_CMP2OUT_Pos)     /*!< 0x00002000 */
#define COMP_CSR_CMP2OUT                    COMP_CSR_CMP2OUT_Msk               /*!< Comparator 2 output level */

#define COMP_CSR_WNDWE_Pos                  (17U)                              
#define COMP_CSR_WNDWE_Msk                  (0x1U << COMP_CSR_WNDWE_Pos)       /*!< 0x00020000 */
#define COMP_CSR_WNDWE                      COMP_CSR_WNDWE_Msk                 /*!< Pair of comparators window mode. Bit intended to be used with COMP common instance (COMP_Common_TypeDef)  */

#define COMP_CSR_INSEL_Pos                  (18U)                              
#define COMP_CSR_INSEL_Msk                  (0x7U << COMP_CSR_INSEL_Pos)       /*!< 0x001C0000 */
#define COMP_CSR_INSEL                      COMP_CSR_INSEL_Msk                 /*!< Comparator 2 input minus selection */
#define COMP_CSR_INSEL_0                    (0x1U << COMP_CSR_INSEL_Pos)       /*!< 0x00040000 */
#define COMP_CSR_INSEL_1                    (0x2U << COMP_CSR_INSEL_Pos)       /*!< 0x00080000 */
#define COMP_CSR_INSEL_2                    (0x4U << COMP_CSR_INSEL_Pos)       /*!< 0x00100000 */
#define COMP_CSR_OUTSEL_Pos                 (21U)                              
#define COMP_CSR_OUTSEL_Msk                 (0x7U << COMP_CSR_OUTSEL_Pos)      /*!< 0x00E00000 */
#define COMP_CSR_OUTSEL                     COMP_CSR_OUTSEL_Msk                /*!< Comparator 2 output redirection */
#define COMP_CSR_OUTSEL_0                   (0x1U << COMP_CSR_OUTSEL_Pos)      /*!< 0x00200000 */
#define COMP_CSR_OUTSEL_1                   (0x2U << COMP_CSR_OUTSEL_Pos)      /*!< 0x00400000 */
#define COMP_CSR_OUTSEL_2                   (0x4U << COMP_CSR_OUTSEL_Pos)      /*!< 0x00800000 */

/* Bits present in COMP register but not related to comparator */
/* (or partially related to comparator, in addition to other peripherals) */
#define COMP_CSR_SW1_Pos                    (5U)                               
#define COMP_CSR_SW1_Msk                    (0x1U << COMP_CSR_SW1_Pos)         /*!< 0x00000020 */
#define COMP_CSR_SW1                        COMP_CSR_SW1_Msk                   /*!< SW1 analog switch enable */
#define COMP_CSR_VREFOUTEN_Pos              (16U)                              
#define COMP_CSR_VREFOUTEN_Msk              (0x1U << COMP_CSR_VREFOUTEN_Pos)   /*!< 0x00010000 */
#define COMP_CSR_VREFOUTEN                  COMP_CSR_VREFOUTEN_Msk             /*!< VrefInt output enable on GPIO group 3 */

#define COMP_CSR_FCH3_Pos                   (26U)                              
#define COMP_CSR_FCH3_Msk                   (0x1U << COMP_CSR_FCH3_Pos)        /*!< 0x04000000 */
#define COMP_CSR_FCH3                       COMP_CSR_FCH3_Msk                  /*!< Bit 26 */
#define COMP_CSR_FCH8_Pos                   (27U)                              
#define COMP_CSR_FCH8_Msk                   (0x1U << COMP_CSR_FCH8_Pos)        /*!< 0x08000000 */
#define COMP_CSR_FCH8                       COMP_CSR_FCH8_Msk                  /*!< Bit 27 */
#define COMP_CSR_RCH13_Pos                  (28U)                              
#define COMP_CSR_RCH13_Msk                  (0x1U << COMP_CSR_RCH13_Pos)       /*!< 0x10000000 */
#define COMP_CSR_RCH13                      COMP_CSR_RCH13_Msk                 /*!< Bit 28 */

#define COMP_CSR_CAIE_Pos                   (29U)                              
#define COMP_CSR_CAIE_Msk                   (0x1U << COMP_CSR_CAIE_Pos)        /*!< 0x20000000 */
#define COMP_CSR_CAIE                       COMP_CSR_CAIE_Msk                  /*!< Bit 29 */
#define COMP_CSR_CAIF_Pos                   (30U)                              
#define COMP_CSR_CAIF_Msk                   (0x1U << COMP_CSR_CAIF_Pos)        /*!< 0x40000000 */
#define COMP_CSR_CAIF                       COMP_CSR_CAIF_Msk                  /*!< Bit 30 */
#define COMP_CSR_TSUSP_Pos                  (31U)                              
#define COMP_CSR_TSUSP_Msk                  (0x1U << COMP_CSR_TSUSP_Pos)       /*!< 0x80000000 */
#define COMP_CSR_TSUSP                      COMP_CSR_TSUSP_Msk                 /*!< Bit 31 */

/******************************************************************************/
/*                                                                            */
/*                         Operational Amplifier (OPAMP)                      */
/*                                                                            */
/******************************************************************************/
/*******************  Bit definition for OPAMP_CSR register  ******************/
#define OPAMP_CSR_OPA1PD_Pos                  (0U)                             
#define OPAMP_CSR_OPA1PD_Msk                  (0x1U << OPAMP_CSR_OPA1PD_Pos)   /*!< 0x00000001 */
#define OPAMP_CSR_OPA1PD                      OPAMP_CSR_OPA1PD_Msk             /*!< OPAMP1 disable */
#define OPAMP_CSR_S3SEL1_Pos                  (1U)                             
#define OPAMP_CSR_S3SEL1_Msk                  (0x1U << OPAMP_CSR_S3SEL1_Pos)   /*!< 0x00000002 */
#define OPAMP_CSR_S3SEL1                      OPAMP_CSR_S3SEL1_Msk             /*!< Switch 3 for OPAMP1 Enable */
#define OPAMP_CSR_S4SEL1_Pos                  (2U)                             
#define OPAMP_CSR_S4SEL1_Msk                  (0x1U << OPAMP_CSR_S4SEL1_Pos)   /*!< 0x00000004 */
#define OPAMP_CSR_S4SEL1                      OPAMP_CSR_S4SEL1_Msk             /*!< Switch 4 for OPAMP1 Enable */
#define OPAMP_CSR_S5SEL1_Pos                  (3U)                             
#define OPAMP_CSR_S5SEL1_Msk                  (0x1U << OPAMP_CSR_S5SEL1_Pos)   /*!< 0x00000008 */
#define OPAMP_CSR_S5SEL1                      OPAMP_CSR_S5SEL1_Msk             /*!< Switch 5 for OPAMP1 Enable */
#define OPAMP_CSR_S6SEL1_Pos                  (4U)                             
#define OPAMP_CSR_S6SEL1_Msk                  (0x1U << OPAMP_CSR_S6SEL1_Pos)   /*!< 0x00000010 */
#define OPAMP_CSR_S6SEL1                      OPAMP_CSR_S6SEL1_Msk             /*!< Switch 6 for OPAMP1 Enable */
#define OPAMP_CSR_OPA1CAL_L_Pos               (5U)                             
#define OPAMP_CSR_OPA1CAL_L_Msk               (0x1U << OPAMP_CSR_OPA1CAL_L_Pos) /*!< 0x00000020 */
#define OPAMP_CSR_OPA1CAL_L                   OPAMP_CSR_OPA1CAL_L_Msk          /*!< OPAMP1 Offset calibration for P differential pair */
#define OPAMP_CSR_OPA1CAL_H_Pos               (6U)                             
#define OPAMP_CSR_OPA1CAL_H_Msk               (0x1U << OPAMP_CSR_OPA1CAL_H_Pos) /*!< 0x00000040 */
#define OPAMP_CSR_OPA1CAL_H                   OPAMP_CSR_OPA1CAL_H_Msk          /*!< OPAMP1 Offset calibration for N differential pair */
#define OPAMP_CSR_OPA1LPM_Pos                 (7U)                             
#define OPAMP_CSR_OPA1LPM_Msk                 (0x1U << OPAMP_CSR_OPA1LPM_Pos)  /*!< 0x00000080 */
#define OPAMP_CSR_OPA1LPM                     OPAMP_CSR_OPA1LPM_Msk            /*!< OPAMP1 Low power enable */
#define OPAMP_CSR_OPA2PD_Pos                  (8U)                             
#define OPAMP_CSR_OPA2PD_Msk                  (0x1U << OPAMP_CSR_OPA2PD_Pos)   /*!< 0x00000100 */
#define OPAMP_CSR_OPA2PD                      OPAMP_CSR_OPA2PD_Msk             /*!< OPAMP2 disable */
#define OPAMP_CSR_S3SEL2_Pos                  (9U)                             
#define OPAMP_CSR_S3SEL2_Msk                  (0x1U << OPAMP_CSR_S3SEL2_Pos)   /*!< 0x00000200 */
#define OPAMP_CSR_S3SEL2                      OPAMP_CSR_S3SEL2_Msk             /*!< Switch 3 for OPAMP2 Enable */
#define OPAMP_CSR_S4SEL2_Pos                  (10U)                            
#define OPAMP_CSR_S4SEL2_Msk                  (0x1U << OPAMP_CSR_S4SEL2_Pos)   /*!< 0x00000400 */
#define OPAMP_CSR_S4SEL2                      OPAMP_CSR_S4SEL2_Msk             /*!< Switch 4 for OPAMP2 Enable */
#define OPAMP_CSR_S5SEL2_Pos                  (11U)                            
#define OPAMP_CSR_S5SEL2_Msk                  (0x1U << OPAMP_CSR_S5SEL2_Pos)   /*!< 0x00000800 */
#define OPAMP_CSR_S5SEL2                      OPAMP_CSR_S5SEL2_Msk             /*!< Switch 5 for OPAMP2 Enable */
#define OPAMP_CSR_S6SEL2_Pos                  (12U)                            
#define OPAMP_CSR_S6SEL2_Msk                  (0x1U << OPAMP_CSR_S6SEL2_Pos)   /*!< 0x00001000 */
#define OPAMP_CSR_S6SEL2                      OPAMP_CSR_S6SEL2_Msk             /*!< Switch 6 for OPAMP2 Enable */
#define OPAMP_CSR_OPA2CAL_L_Pos               (13U)                            
#define OPAMP_CSR_OPA2CAL_L_Msk               (0x1U << OPAMP_CSR_OPA2CAL_L_Pos) /*!< 0x00002000 */
#define OPAMP_CSR_OPA2CAL_L                   OPAMP_CSR_OPA2CAL_L_Msk          /*!< OPAMP2 Offset calibration for P differential pair */
#define OPAMP_CSR_OPA2CAL_H_Pos               (14U)                            
#define OPAMP_CSR_OPA2CAL_H_Msk               (0x1U << OPAMP_CSR_OPA2CAL_H_Pos) /*!< 0x00004000 */
#define OPAMP_CSR_OPA2CAL_H                   OPAMP_CSR_OPA2CAL_H_Msk          /*!< OPAMP2 Offset calibration for N differential pair */
#define OPAMP_CSR_OPA2LPM_Pos                 (15U)                            
#define OPAMP_CSR_OPA2LPM_Msk                 (0x1U << OPAMP_CSR_OPA2LPM_Pos)  /*!< 0x00008000 */
#define OPAMP_CSR_OPA2LPM                     OPAMP_CSR_OPA2LPM_Msk            /*!< OPAMP2 Low power enable */
#define OPAMP_CSR_OPA3PD_Pos                  (16U)                            
#define OPAMP_CSR_OPA3PD_Msk                  (0x1U << OPAMP_CSR_OPA3PD_Pos)   /*!< 0x00010000 */
#define OPAMP_CSR_OPA3PD                      OPAMP_CSR_OPA3PD_Msk             /*!< OPAMP3 disable */
#define OPAMP_CSR_S3SEL3_Pos                  (17U)                            
#define OPAMP_CSR_S3SEL3_Msk                  (0x1U << OPAMP_CSR_S3SEL3_Pos)   /*!< 0x00020000 */
#define OPAMP_CSR_S3SEL3                      OPAMP_CSR_S3SEL3_Msk             /*!< Switch 3 for OPAMP3 Enable */
#define OPAMP_CSR_S4SEL3_Pos                  (18U)                            
#define OPAMP_CSR_S4SEL3_Msk                  (0x1U << OPAMP_CSR_S4SEL3_Pos)   /*!< 0x00040000 */
#define OPAMP_CSR_S4SEL3                      OPAMP_CSR_S4SEL3_Msk             /*!< Switch 4 for OPAMP3 Enable */
#define OPAMP_CSR_S5SEL3_Pos                  (19U)                            
#define OPAMP_CSR_S5SEL3_Msk                  (0x1U << OPAMP_CSR_S5SEL3_Pos)   /*!< 0x00080000 */
#define OPAMP_CSR_S5SEL3                      OPAMP_CSR_S5SEL3_Msk             /*!< Switch 5 for OPAMP3 Enable */
#define OPAMP_CSR_S6SEL3_Pos                  (20U)                            
#define OPAMP_CSR_S6SEL3_Msk                  (0x1U << OPAMP_CSR_S6SEL3_Pos)   /*!< 0x00100000 */
#define OPAMP_CSR_S6SEL3                      OPAMP_CSR_S6SEL3_Msk             /*!< Switch 6 for OPAMP3 Enable */
#define OPAMP_CSR_OPA3CAL_L_Pos               (21U)                            
#define OPAMP_CSR_OPA3CAL_L_Msk               (0x1U << OPAMP_CSR_OPA3CAL_L_Pos) /*!< 0x00200000 */
#define OPAMP_CSR_OPA3CAL_L                   OPAMP_CSR_OPA3CAL_L_Msk          /*!< OPAMP3 Offset calibration for P differential pair */
#define OPAMP_CSR_OPA3CAL_H_Pos               (22U)                            
#define OPAMP_CSR_OPA3CAL_H_Msk               (0x1U << OPAMP_CSR_OPA3CAL_H_Pos) /*!< 0x00400000 */
#define OPAMP_CSR_OPA3CAL_H                   OPAMP_CSR_OPA3CAL_H_Msk          /*!< OPAMP3 Offset calibration for N differential pair */
#define OPAMP_CSR_OPA3LPM_Pos                 (23U)                            
#define OPAMP_CSR_OPA3LPM_Msk                 (0x1U << OPAMP_CSR_OPA3LPM_Pos)  /*!< 0x00800000 */
#define OPAMP_CSR_OPA3LPM                     OPAMP_CSR_OPA3LPM_Msk            /*!< OPAMP3 Low power enable */
#define OPAMP_CSR_ANAWSEL1_Pos                (24U)                            
#define OPAMP_CSR_ANAWSEL1_Msk                (0x1U << OPAMP_CSR_ANAWSEL1_Pos) /*!< 0x01000000 */
#define OPAMP_CSR_ANAWSEL1                    OPAMP_CSR_ANAWSEL1_Msk           /*!< Switch ANA Enable for OPAMP1 */ 
#define OPAMP_CSR_ANAWSEL2_Pos                (25U)                            
#define OPAMP_CSR_ANAWSEL2_Msk                (0x1U << OPAMP_CSR_ANAWSEL2_Pos) /*!< 0x02000000 */
#define OPAMP_CSR_ANAWSEL2                    OPAMP_CSR_ANAWSEL2_Msk           /*!< Switch ANA Enable for OPAMP2 */
#define OPAMP_CSR_ANAWSEL3_Pos                (26U)                            
#define OPAMP_CSR_ANAWSEL3_Msk                (0x1U << OPAMP_CSR_ANAWSEL3_Pos) /*!< 0x04000000 */
#define OPAMP_CSR_ANAWSEL3                    OPAMP_CSR_ANAWSEL3_Msk           /*!< Switch ANA Enable for OPAMP3 */
#define OPAMP_CSR_S7SEL2_Pos                  (27U)                            
#define OPAMP_CSR_S7SEL2_Msk                  (0x1U << OPAMP_CSR_S7SEL2_Pos)   /*!< 0x08000000 */
#define OPAMP_CSR_S7SEL2                      OPAMP_CSR_S7SEL2_Msk             /*!< Switch 7 for OPAMP2 Enable */
#define OPAMP_CSR_AOP_RANGE_Pos               (28U)                            
#define OPAMP_CSR_AOP_RANGE_Msk               (0x1U << OPAMP_CSR_AOP_RANGE_Pos) /*!< 0x10000000 */
#define OPAMP_CSR_AOP_RANGE                   OPAMP_CSR_AOP_RANGE_Msk          /*!< Common to several OPAMP instances: Operational amplifier voltage supply range. Bit intended to be used with OPAMP common instance (OPAMP_Common_TypeDef) */
#define OPAMP_CSR_OPA1CALOUT_Pos              (29U)                            
#define OPAMP_CSR_OPA1CALOUT_Msk              (0x1U << OPAMP_CSR_OPA1CALOUT_Pos) /*!< 0x20000000 */
#define OPAMP_CSR_OPA1CALOUT                  OPAMP_CSR_OPA1CALOUT_Msk         /*!< OPAMP1 calibration output */
#define OPAMP_CSR_OPA2CALOUT_Pos              (30U)                            
#define OPAMP_CSR_OPA2CALOUT_Msk              (0x1U << OPAMP_CSR_OPA2CALOUT_Pos) /*!< 0x40000000 */
#define OPAMP_CSR_OPA2CALOUT                  OPAMP_CSR_OPA2CALOUT_Msk         /*!< OPAMP2 calibration output */
#define OPAMP_CSR_OPA3CALOUT_Pos              (31U)                            
#define OPAMP_CSR_OPA3CALOUT_Msk              (0x1U << OPAMP_CSR_OPA3CALOUT_Pos) /*!< 0x80000000 */
#define OPAMP_CSR_OPA3CALOUT                  OPAMP_CSR_OPA3CALOUT_Msk         /*!< OPAMP3 calibration output */

/*******************  Bit definition for OPAMP_OTR register  ******************/
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW_Pos (0U)                             
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW_Msk (0x1FU << OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW_Pos) /*!< 0x0000001F */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW     OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP1 */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH_Pos (5U)                            
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH_Msk (0x1FU << OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH_Pos) /*!< 0x000003E0 */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH    OPAMP_OTR_AO1_OPT_OFFSET_TRIM_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP1 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos (10U)                            
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Msk (0x1FU << OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Pos) /*!< 0x00007C00 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW     OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP2 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_HIGH_Pos (15U)                           
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_HIGH_Msk (0x1FU << OPAMP_OTR_AO2_OPT_OFFSET_TRIM_HIGH_Pos) /*!< 0x000F8000 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_HIGH    OPAMP_OTR_AO2_OPT_OFFSET_TRIM_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP2 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LOW_Pos (20U)                            
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LOW_Msk (0x1FU << OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LOW_Pos) /*!< 0x01F00000 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LOW     OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP3 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_HIGH_Pos (25U)                           
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_HIGH_Msk (0x1FU << OPAMP_OTR_AO3_OPT_OFFSET_TRIM_HIGH_Pos) /*!< 0x3E000000 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_HIGH    OPAMP_OTR_AO3_OPT_OFFSET_TRIM_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP3 */
#define OPAMP_OTR_OT_USER_Pos                 (31U)                            
#define OPAMP_OTR_OT_USER_Msk                 (0x1U << OPAMP_OTR_OT_USER_Pos)  /*!< 0x80000000 */
#define OPAMP_OTR_OT_USER                     OPAMP_OTR_OT_USER_Msk            /*!< Switch to OPAMP offset user trimmed values */

/*******************  Bit definition for OPAMP_LPOTR register  ****************/
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_LOW_Pos (0U)                          
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_LOW_Msk (0x1FU << OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_LOW_Pos) /*!< 0x0000001F */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_LOW  OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP1 */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_HIGH_Pos (5U)                         
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_HIGH_Msk (0x1FU << OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_HIGH_Pos) /*!< 0x000003E0 */
#define OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_HIGH OPAMP_OTR_AO1_OPT_OFFSET_TRIM_LP_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP1 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_LOW_Pos (10U)                         
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_LOW_Msk (0x1FU << OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_LOW_Pos) /*!< 0x00007C00 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_LOW  OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP2 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_HIGH_Pos (15U)                        
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_HIGH_Msk (0x1FU << OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_HIGH_Pos) /*!< 0x000F8000 */
#define OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_HIGH OPAMP_OTR_AO2_OPT_OFFSET_TRIM_LP_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP2 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_LOW_Pos (20U)                         
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_LOW_Msk (0x1FU << OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_LOW_Pos) /*!< 0x01F00000 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_LOW  OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_LOW_Msk /*!< Offset trim for transistors differential pair PMOS of OPAMP3 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_HIGH_Pos (25U)                        
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_HIGH_Msk (0x1FU << OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_HIGH_Pos) /*!< 0x3E000000 */
#define OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_HIGH OPAMP_OTR_AO3_OPT_OFFSET_TRIM_LP_HIGH_Msk /*!< Offset trim for transistors differential pair NMOS of OPAMP3 */

/******************************************************************************/
/*                                                                            */
/*                       CRC calculation unit (CRC)                           */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for CRC_DR register  *********************/
#define CRC_DR_DR_Pos                       (0U)                               
#define CRC_DR_DR_Msk                       (0xFFFFFFFFU << CRC_DR_DR_Pos)     /*!< 0xFFFFFFFF */
#define CRC_DR_DR                           CRC_DR_DR_Msk                      /*!< Data register bits */

/*******************  Bit definition for CRC_IDR register  ********************/
#define CRC_IDR_IDR_Pos                     (0U)                               
#define CRC_IDR_IDR_Msk                     (0xFFU << CRC_IDR_IDR_Pos)         /*!< 0x000000FF */
#define CRC_IDR_IDR                         CRC_IDR_IDR_Msk                    /*!< General-purpose 8-bit data register bits */

/********************  Bit definition for CRC_CR register  ********************/
#define CRC_CR_RESET_Pos                    (0U)                               
#define CRC_CR_RESET_Msk                    (0x1U << CRC_CR_RESET_Pos)         /*!< 0x00000001 */
#define CRC_CR_RESET                        CRC_CR_RESET_Msk                   /*!< RESET bit */

/******************************************************************************/
/*                                                                            */
/*                    Digital to Analog Converter (DAC)                       */
/*                                                                            */
/******************************************************************************/

/********************  Bit definition for DAC_CR register  ********************/
#define DAC_CR_EN1_Pos                      (0U)                               
#define DAC_CR_EN1_Msk                      (0x1U << DAC_CR_EN1_Pos)           /*!< 0x00000001 */
#define DAC_CR_EN1                          DAC_CR_EN1_Msk                     /*!<DAC channel1 enable */
#define DAC_CR_BOFF1_Pos                    (1U)                               
#define DAC_CR_BOFF1_Msk                    (0x1U << DAC_CR_BOFF1_Pos)         /*!< 0x00000002 */
#define DAC_CR_BOFF1                        DAC_CR_BOFF1_Msk                   /*!<DAC channel1 output buffer disable */
#define DAC_CR_TEN1_Pos                     (2U)                               
#define DAC_CR_TEN1_Msk                     (0x1U << DAC_CR_TEN1_Pos)          /*!< 0x00000004 */
#define DAC_CR_TEN1                         DAC_CR_TEN1_Msk                    /*!<DAC channel1 Trigger enable */

#define DAC_CR_TSEL1_Pos                    (3U)                               
#define DAC_CR_TSEL1_Msk                    (0x7U << DAC_CR_TSEL1_Pos)         /*!< 0x00000038 */
#define DAC_CR_TSEL1                        DAC_CR_TSEL1_Msk                   /*!<TSEL1[2:0] (DAC channel1 Trigger selection) */
#define DAC_CR_TSEL1_0                      (0x1U << DAC_CR_TSEL1_Pos)         /*!< 0x00000008 */
#define DAC_CR_TSEL1_1                      (0x2U << DAC_CR_TSEL1_Pos)         /*!< 0x00000010 */
#define DAC_CR_TSEL1_2                      (0x4U << DAC_CR_TSEL1_Pos)         /*!< 0x00000020 */

#define DAC_CR_WAVE1_Pos                    (6U)                               
#define DAC_CR_WAVE1_Msk                    (0x3U << DAC_CR_WAVE1_Pos)         /*!< 0x000000C0 */
#define DAC_CR_WAVE1                        DAC_CR_WAVE1_Msk                   /*!<WAVE1[1:0] (DAC channel1 noise/triangle wave generation enable) */
#define DAC_CR_WAVE1_0                      (0x1U << DAC_CR_WAVE1_Pos)         /*!< 0x00000040 */
#define DAC_CR_WAVE1_1                      (0x2U << DAC_CR_WAVE1_Pos)         /*!< 0x00000080 */

#define DAC_CR_MAMP1_Pos                    (8U)                               
#define DAC_CR_MAMP1_Msk                    (0xFU << DAC_CR_MAMP1_Pos)         /*!< 0x00000F00 */
#define DAC_CR_MAMP1                        DAC_CR_MAMP1_Msk                   /*!<MAMP1[3:0] (DAC channel1 Mask/Amplitude selector) */
#define DAC_CR_MAMP1_0                      (0x1U << DAC_CR_MAMP1_Pos)         /*!< 0x00000100 */
#define DAC_CR_MAMP1_1                      (0x2U << DAC_CR_MAMP1_Pos)         /*!< 0x00000200 */
#define DAC_CR_MAMP1_2                      (0x4U << DAC_CR_MAMP1_Pos)         /*!< 0x00000400 */
#define DAC_CR_MAMP1_3                      (0x8U << DAC_CR_MAMP1_Pos)         /*!< 0x00000800 */

#define DAC_CR_DMAEN1_Pos                   (12U)                              
#define DAC_CR_DMAEN1_Msk                   (0x1U << DAC_CR_DMAEN1_Pos)        /*!< 0x00001000 */
#define DAC_CR_DMAEN1                       DAC_CR_DMAEN1_Msk                  /*!<DAC channel1 DMA enable */
#define DAC_CR_DMAUDRIE1_Pos                (13U)                              
#define DAC_CR_DMAUDRIE1_Msk                (0x1U << DAC_CR_DMAUDRIE1_Pos)     /*!< 0x00002000 */
#define DAC_CR_DMAUDRIE1                    DAC_CR_DMAUDRIE1_Msk               /*!<DAC channel1 DMA Interrupt enable */
#define DAC_CR_EN2_Pos                      (16U)                              
#define DAC_CR_EN2_Msk                      (0x1U << DAC_CR_EN2_Pos)           /*!< 0x00010000 */
#define DAC_CR_EN2                          DAC_CR_EN2_Msk                     /*!<DAC channel2 enable */
#define DAC_CR_BOFF2_Pos                    (17U)                              
#define DAC_CR_BOFF2_Msk                    (0x1U << DAC_CR_BOFF2_Pos)         /*!< 0x00020000 */
#define DAC_CR_BOFF2                        DAC_CR_BOFF2_Msk                   /*!<DAC channel2 output buffer disable */
#define DAC_CR_TEN2_Pos                     (18U)                              
#define DAC_CR_TEN2_Msk                     (0x1U << DAC_CR_TEN2_Pos)          /*!< 0x00040000 */
#define DAC_CR_TEN2                         DAC_CR_TEN2_Msk                    /*!<DAC channel2 Trigger enable */

#define DAC_CR_TSEL2_Pos                    (19U)                              
#define DAC_CR_TSEL2_Msk                    (0x7U << DAC_CR_TSEL2_Pos)         /*!< 0x00380000 */
#define DAC_CR_TSEL2                        DAC_CR_TSEL2_Msk                   /*!<TSEL2[2:0] (DAC channel2 Trigger selection) */
#define DAC_CR_TSEL2_0                      (0x1U << DAC_CR_TSEL2_Pos)         /*!< 0x00080000 */
#define DAC_CR_TSEL2_1                      (0x2U << DAC_CR_TSEL2_Pos)         /*!< 0x00100000 */
#define DAC_CR_TSEL2_2                      (0x4U << DAC_CR_TSEL2_Pos)         /*!< 0x00200000 */

#define DAC_CR_WAVE2_Pos                    (22U)                              
#define DAC_CR_WAVE2_Msk                    (0x3U << DAC_CR_WAVE2_Pos)         /*!< 0x00C00000 */
#define DAC_CR_WAVE2                        DAC_CR_WAVE2_Msk                   /*!<WAVE2[1:0] (DAC channel2 noise/triangle wave generation enable) */
#define DAC_CR_WAVE2_0                      (0x1U << DAC_CR_WAVE2_Pos)         /*!< 0x00400000 */
#define DAC_CR_WAVE2_1                      (0x2U << DAC_CR_WAVE2_Pos)         /*!< 0x00800000 */

#define DAC_CR_MAMP2_Pos                    (24U)                              
#define DAC_CR_MAMP2_Msk                    (0xFU << DAC_CR_MAMP2_Pos)         /*!< 0x0F000000 */
#define DAC_CR_MAMP2                        DAC_CR_MAMP2_Msk                   /*!<MAMP2[3:0] (DAC channel2 Mask/Amplitude selector) */
#define DAC_CR_MAMP2_0                      (0x1U << DAC_CR_MAMP2_Pos)         /*!< 0x01000000 */
#define DAC_CR_MAMP2_1                      (0x2U << DAC_CR_MAMP2_Pos)         /*!< 0x02000000 */
#define DAC_CR_MAMP2_2                      (0x4U << DAC_CR_MAMP2_Pos)         /*!< 0x04000000 */
#define DAC_CR_MAMP2_3                      (0x8U << DAC_CR_MAMP2_Pos)         /*!< 0x08000000 */

#define DAC_CR_DMAEN2_Pos                   (28U)                              
#define DAC_CR_DMAEN2_Msk                   (0x1U << DAC_CR_DMAEN2_Pos)        /*!< 0x10000000 */
#define DAC_CR_DMAEN2                       DAC_CR_DMAEN2_Msk                  /*!<DAC channel2 DMA enabled */
#define DAC_CR_DMAUDRIE2_Pos                (29U)                              
#define DAC_CR_DMAUDRIE2_Msk                (0x1U << DAC_CR_DMAUDRIE2_Pos)     /*!< 0x20000000 */
#define DAC_CR_DMAUDRIE2                    DAC_CR_DMAUDRIE2_Msk               /*!<DAC channel2 DMA underrun interrupt enable */
/*****************  Bit definition for DAC_SWTRIGR register  ******************/
#define DAC_SWTRIGR_SWTRIG1_Pos             (0U)                               
#define DAC_SWTRIGR_SWTRIG1_Msk             (0x1U << DAC_SWTRIGR_SWTRIG1_Pos)  /*!< 0x00000001 */
#define DAC_SWTRIGR_SWTRIG1                 DAC_SWTRIGR_SWTRIG1_Msk            /*!<DAC channel1 software trigger */
#define DAC_SWTRIGR_SWTRIG2_Pos             (1U)                               
#define DAC_SWTRIGR_SWTRIG2_Msk             (0x1U << DAC_SWTRIGR_SWTRIG2_Pos)  /*!< 0x00000002 */
#define DAC_SWTRIGR_SWTRIG2                 DAC_SWTRIGR_SWTRIG2_Msk            /*!<DAC channel2 software trigger */

/*****************  Bit definition for DAC_DHR12R1 register  ******************/
#define DAC_DHR12R1_DACC1DHR_Pos            (0U)                               
#define DAC_DHR12R1_DACC1DHR_Msk            (0xFFFU << DAC_DHR12R1_DACC1DHR_Pos) /*!< 0x00000FFF */
#define DAC_DHR12R1_DACC1DHR                DAC_DHR12R1_DACC1DHR_Msk           /*!<DAC channel1 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12L1 register  ******************/
#define DAC_DHR12L1_DACC1DHR_Pos            (4U)                               
#define DAC_DHR12L1_DACC1DHR_Msk            (0xFFFU << DAC_DHR12L1_DACC1DHR_Pos) /*!< 0x0000FFF0 */
#define DAC_DHR12L1_DACC1DHR                DAC_DHR12L1_DACC1DHR_Msk           /*!<DAC channel1 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8R1 register  ******************/
#define DAC_DHR8R1_DACC1DHR_Pos             (0U)                               
#define DAC_DHR8R1_DACC1DHR_Msk             (0xFFU << DAC_DHR8R1_DACC1DHR_Pos) /*!< 0x000000FF */
#define DAC_DHR8R1_DACC1DHR                 DAC_DHR8R1_DACC1DHR_Msk            /*!<DAC channel1 8-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12R2 register  ******************/
#define DAC_DHR12R2_DACC2DHR_Pos            (0U)                               
#define DAC_DHR12R2_DACC2DHR_Msk            (0xFFFU << DAC_DHR12R2_DACC2DHR_Pos) /*!< 0x00000FFF */
#define DAC_DHR12R2_DACC2DHR                DAC_DHR12R2_DACC2DHR_Msk           /*!<DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12L2 register  ******************/
#define DAC_DHR12L2_DACC2DHR_Pos            (4U)                               
#define DAC_DHR12L2_DACC2DHR_Msk            (0xFFFU << DAC_DHR12L2_DACC2DHR_Pos) /*!< 0x0000FFF0 */
#define DAC_DHR12L2_DACC2DHR                DAC_DHR12L2_DACC2DHR_Msk           /*!<DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8R2 register  ******************/
#define DAC_DHR8R2_DACC2DHR_Pos             (0U)                               
#define DAC_DHR8R2_DACC2DHR_Msk             (0xFFU << DAC_DHR8R2_DACC2DHR_Pos) /*!< 0x000000FF */
#define DAC_DHR8R2_DACC2DHR                 DAC_DHR8R2_DACC2DHR_Msk            /*!<DAC channel2 8-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12RD register  ******************/
#define DAC_DHR12RD_DACC1DHR_Pos            (0U)                               
#define DAC_DHR12RD_DACC1DHR_Msk            (0xFFFU << DAC_DHR12RD_DACC1DHR_Pos) /*!< 0x00000FFF */
#define DAC_DHR12RD_DACC1DHR                DAC_DHR12RD_DACC1DHR_Msk           /*!<DAC channel1 12-bit Right aligned data */
#define DAC_DHR12RD_DACC2DHR_Pos            (16U)                              
#define DAC_DHR12RD_DACC2DHR_Msk            (0xFFFU << DAC_DHR12RD_DACC2DHR_Pos) /*!< 0x0FFF0000 */
#define DAC_DHR12RD_DACC2DHR                DAC_DHR12RD_DACC2DHR_Msk           /*!<DAC channel2 12-bit Right aligned data */

/*****************  Bit definition for DAC_DHR12LD register  ******************/
#define DAC_DHR12LD_DACC1DHR_Pos            (4U)                               
#define DAC_DHR12LD_DACC1DHR_Msk            (0xFFFU << DAC_DHR12LD_DACC1DHR_Pos) /*!< 0x0000FFF0 */
#define DAC_DHR12LD_DACC1DHR                DAC_DHR12LD_DACC1DHR_Msk           /*!<DAC channel1 12-bit Left aligned data */
#define DAC_DHR12LD_DACC2DHR_Pos            (20U)                              
#define DAC_DHR12LD_DACC2DHR_Msk            (0xFFFU << DAC_DHR12LD_DACC2DHR_Pos) /*!< 0xFFF00000 */
#define DAC_DHR12LD_DACC2DHR                DAC_DHR12LD_DACC2DHR_Msk           /*!<DAC channel2 12-bit Left aligned data */

/******************  Bit definition for DAC_DHR8RD register  ******************/
#define DAC_DHR8RD_DACC1DHR_Pos             (0U)                               
#define DAC_DHR8RD_DACC1DHR_Msk             (0xFFU << DAC_DHR8RD_DACC1DHR_Pos) /*!< 0x000000FF */
#define DAC_DHR8RD_DACC1DHR                 DAC_DHR8RD_DACC1DHR_Msk            /*!<DAC channel1 8-bit Right aligned data */
#define DAC_DHR8RD_DACC2DHR_Pos             (8U)                               
#define DAC_DHR8RD_DACC2DHR_Msk             (0xFFU << DAC_DHR8RD_DACC2DHR_Pos) /*!< 0x0000FF00 */
#define DAC_DHR8RD_DACC2DHR                 DAC_DHR8RD_DACC2DHR_Msk            /*!<DAC channel2 8-bit Right aligned data */

/*******************  Bit definition for DAC_DOR1 register  *******************/
#define DAC_DOR1_DACC1DOR_Pos               (0U)                               
#define DAC_DOR1_DACC1DOR_Msk               (0xFFFU << DAC_DOR1_DACC1DOR_Pos)  /*!< 0x00000FFF */
#define DAC_DOR1_DACC1DOR                   DAC_DOR1_DACC1DOR_Msk              /*!<DAC channel1 data output */

/*******************  Bit definition for DAC_DOR2 register  *******************/
#define DAC_DOR2_DACC2DOR_Pos               (0U)                               
#define DAC_DOR2_DACC2DOR_Msk               (0xFFFU << DAC_DOR2_DACC2DOR_Pos)  /*!< 0x00000FFF */
#define DAC_DOR2_DACC2DOR                   DAC_DOR2_DACC2DOR_Msk              /*!<DAC channel2 data output */

/********************  Bit definition for DAC_SR register  ********************/
#define DAC_SR_DMAUDR1_Pos                  (13U)                              
#define DAC_SR_DMAUDR1_Msk                  (0x1U << DAC_SR_DMAUDR1_Pos)       /*!< 0x00002000 */
#define DAC_SR_DMAUDR1                      DAC_SR_DMAUDR1_Msk                 /*!<DAC channel1 DMA underrun flag */
#define DAC_SR_DMAUDR2_Pos                  (29U)                              
#define DAC_SR_DMAUDR2_Msk                  (0x1U << DAC_SR_DMAUDR2_Pos)       /*!< 0x20000000 */
#define DAC_SR_DMAUDR2                      DAC_SR_DMAUDR2_Msk                 /*!<DAC channel2 DMA underrun flag */

/******************************************************************************/
/*                                                                            */
/*                           Debug MCU (DBGMCU)                               */
/*                                                                            */
/******************************************************************************/

/****************  Bit definition for DBGMCU_IDCODE register  *****************/
#define DBGMCU_IDCODE_DEV_ID_Pos                 (0U)                          
#define DBGMCU_IDCODE_DEV_ID_Msk                 (0xFFFU << DBGMCU_IDCODE_DEV_ID_Pos) /*!< 0x00000FFF */
#define DBGMCU_IDCODE_DEV_ID                     DBGMCU_IDCODE_DEV_ID_Msk      /*!< Device Identifier */

#define DBGMCU_IDCODE_REV_ID_Pos                 (16U)                         
#define DBGMCU_IDCODE_REV_ID_Msk                 (0xFFFFU << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0xFFFF0000 */
#define DBGMCU_IDCODE_REV_ID                     DBGMCU_IDCODE_REV_ID_Msk      /*!< REV_ID[15:0] bits (Revision Identifier) */
#define DBGMCU_IDCODE_REV_ID_0                   (0x0001U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00010000 */
#define DBGMCU_IDCODE_REV_ID_1                   (0x0002U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00020000 */
#define DBGMCU_IDCODE_REV_ID_2                   (0x0004U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00040000 */
#define DBGMCU_IDCODE_REV_ID_3                   (0x0008U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00080000 */
#define DBGMCU_IDCODE_REV_ID_4                   (0x0010U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00100000 */
#define DBGMCU_IDCODE_REV_ID_5                   (0x0020U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00200000 */
#define DBGMCU_IDCODE_REV_ID_6                   (0x0040U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00400000 */
#define DBGMCU_IDCODE_REV_ID_7                   (0x0080U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x00800000 */
#define DBGMCU_IDCODE_REV_ID_8                   (0x0100U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x01000000 */
#define DBGMCU_IDCODE_REV_ID_9                   (0x0200U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x02000000 */
#define DBGMCU_IDCODE_REV_ID_10                  (0x0400U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x04000000 */
#define DBGMCU_IDCODE_REV_ID_11                  (0x0800U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x08000000 */
#define DBGMCU_IDCODE_REV_ID_12                  (0x1000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x10000000 */
#define DBGMCU_IDCODE_REV_ID_13                  (0x2000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x20000000 */
#define DBGMCU_IDCODE_REV_ID_14                  (0x4000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x40000000 */
#define DBGMCU_IDCODE_REV_ID_15                  (0x8000U << DBGMCU_IDCODE_REV_ID_Pos) /*!< 0x80000000 */

/******************  Bit definition for DBGMCU_CR register  *******************/
#define DBGMCU_CR_DBG_SLEEP_Pos                  (0U)                          
#define DBGMCU_CR_DBG_SLEEP_Msk                  (0x1U << DBGMCU_CR_DBG_SLEEP_Pos) /*!< 0x00000001 */
#define DBGMCU_CR_DBG_SLEEP                      DBGMCU_CR_DBG_SLEEP_Msk       /*!< Debug Sleep Mode */
#define DBGMCU_CR_DBG_STOP_Pos                   (1U)                          
#define DBGMCU_CR_DBG_STOP_Msk                   (0x1U << DBGMCU_CR_DBG_STOP_Pos) /*!< 0x00000002 */
#define DBGMCU_CR_DBG_STOP                       DBGMCU_CR_DBG_STOP_Msk        /*!< Debug Stop Mode */
#define DBGMCU_CR_DBG_STANDBY_Pos                (2U)                          
#define DBGMCU_CR_DBG_STANDBY_Msk                (0x1U << DBGMCU_CR_DBG_STANDBY_Pos) /*!< 0x00000004 */
#define DBGMCU_CR_DBG_STANDBY                    DBGMCU_CR_DBG_STANDBY_Msk     /*!< Debug Standby mode */
#define DBGMCU_CR_TRACE_IOEN_Pos                 (5U)                          
#define DBGMCU_CR_TRACE_IOEN_Msk                 (0x1U << DBGMCU_CR_TRACE_IOEN_Pos) /*!< 0x00000020 */
#define DBGMCU_CR_TRACE_IOEN                     DBGMCU_CR_TRACE_IOEN_Msk      /*!< Trace Pin Assignment Control */

#define DBGMCU_CR_TRACE_MODE_Pos                 (6U)                          
#define DBGMCU_CR_TRACE_MODE_Msk                 (0x3U << DBGMCU_CR_TRACE_MODE_Pos) /*!< 0x000000C0 */
#define DBGMCU_CR_TRACE_MODE                     DBGMCU_CR_TRACE_MODE_Msk      /*!< TRACE_MODE[1:0] bits (Trace Pin Assignment Control) */
#define DBGMCU_CR_TRACE_MODE_0                   (0x1U << DBGMCU_CR_TRACE_MODE_Pos) /*!< 0x00000040 */
#define DBGMCU_CR_TRACE_MODE_1                   (0x2U << DBGMCU_CR_TRACE_MODE_Pos) /*!< 0x00000080 */

/******************  Bit definition for DBGMCU_APB1_FZ register  **************/

#define DBGMCU_APB1_FZ_DBG_TIM2_STOP_Pos         (0U)                          
#define DBGMCU_APB1_FZ_DBG_TIM2_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM2_STOP_Pos) /*!< 0x00000001 */
#define DBGMCU_APB1_FZ_DBG_TIM2_STOP             DBGMCU_APB1_FZ_DBG_TIM2_STOP_Msk /*!< TIM2 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM3_STOP_Pos         (1U)                          
#define DBGMCU_APB1_FZ_DBG_TIM3_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM3_STOP_Pos) /*!< 0x00000002 */
#define DBGMCU_APB1_FZ_DBG_TIM3_STOP             DBGMCU_APB1_FZ_DBG_TIM3_STOP_Msk /*!< TIM3 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM4_STOP_Pos         (2U)                          
#define DBGMCU_APB1_FZ_DBG_TIM4_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM4_STOP_Pos) /*!< 0x00000004 */
#define DBGMCU_APB1_FZ_DBG_TIM4_STOP             DBGMCU_APB1_FZ_DBG_TIM4_STOP_Msk /*!< TIM4 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM5_STOP_Pos         (3U)                          
#define DBGMCU_APB1_FZ_DBG_TIM5_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM5_STOP_Pos) /*!< 0x00000008 */
#define DBGMCU_APB1_FZ_DBG_TIM5_STOP             DBGMCU_APB1_FZ_DBG_TIM5_STOP_Msk /*!< TIM5 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP_Pos         (4U)                          
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM6_STOP_Pos) /*!< 0x00000010 */
#define DBGMCU_APB1_FZ_DBG_TIM6_STOP             DBGMCU_APB1_FZ_DBG_TIM6_STOP_Msk /*!< TIM6 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_TIM7_STOP_Pos         (5U)                          
#define DBGMCU_APB1_FZ_DBG_TIM7_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_TIM7_STOP_Pos) /*!< 0x00000020 */
#define DBGMCU_APB1_FZ_DBG_TIM7_STOP             DBGMCU_APB1_FZ_DBG_TIM7_STOP_Msk /*!< TIM7 counter stopped when core is halted */
#define DBGMCU_APB1_FZ_DBG_RTC_STOP_Pos          (10U)                         
#define DBGMCU_APB1_FZ_DBG_RTC_STOP_Msk          (0x1U << DBGMCU_APB1_FZ_DBG_RTC_STOP_Pos) /*!< 0x00000400 */
#define DBGMCU_APB1_FZ_DBG_RTC_STOP              DBGMCU_APB1_FZ_DBG_RTC_STOP_Msk /*!< RTC Counter stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos         (11U)                         
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_WWDG_STOP_Pos) /*!< 0x00000800 */
#define DBGMCU_APB1_FZ_DBG_WWDG_STOP             DBGMCU_APB1_FZ_DBG_WWDG_STOP_Msk /*!< Debug Window Watchdog stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos         (12U)                         
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP_Msk         (0x1U << DBGMCU_APB1_FZ_DBG_IWDG_STOP_Pos) /*!< 0x00001000 */
#define DBGMCU_APB1_FZ_DBG_IWDG_STOP             DBGMCU_APB1_FZ_DBG_IWDG_STOP_Msk /*!< Debug Independent Watchdog stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT_Pos (21U)                        
#define DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT_Msk (0x1U << DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT_Pos) /*!< 0x00200000 */
#define DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT    DBGMCU_APB1_FZ_DBG_I2C1_SMBUS_TIMEOUT_Msk /*!< SMBUS timeout mode stopped when Core is halted */
#define DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT_Pos (22U)                        
#define DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT_Msk (0x1U << DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT_Pos) /*!< 0x00400000 */
#define DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT    DBGMCU_APB1_FZ_DBG_I2C2_SMBUS_TIMEOUT_Msk /*!< SMBUS timeout mode stopped when Core is halted */

/******************  Bit definition for DBGMCU_APB2_FZ register  **************/

#define DBGMCU_APB2_FZ_DBG_TIM9_STOP_Pos         (2U)                          
#define DBGMCU_APB2_FZ_DBG_TIM9_STOP_Msk         (0x1U << DBGMCU_APB2_FZ_DBG_TIM9_STOP_Pos) /*!< 0x00000004 */
#define DBGMCU_APB2_FZ_DBG_TIM9_STOP             DBGMCU_APB2_FZ_DBG_TIM9_STOP_Msk /*!< TIM9 counter stopped when core is halted */
#define DBGMCU_APB2_FZ_DBG_TIM10_STOP_Pos        (3U)                          
#define DBGMCU_APB2_FZ_DBG_TIM10_STOP_Msk        (0x1U << DBGMCU_APB2_FZ_DBG_TIM10_STOP_Pos) /*!< 0x00000008 */
#define DBGMCU_APB2_FZ_DBG_TIM10_STOP            DBGMCU_APB2_FZ_DBG_TIM10_STOP_Msk /*!< TIM10 counter stopped when core is halted */
#define DBGMCU_APB2_FZ_DBG_TIM11_STOP_Pos        (4U)                          
#define DBGMCU_APB2_FZ_DBG_TIM11_STOP_Msk        (0x1U << DBGMCU_APB2_FZ_DBG_TIM11_STOP_Pos) /*!< 0x00000010 */
#define DBGMCU_APB2_FZ_DBG_TIM11_STOP            DBGMCU_APB2_FZ_DBG_TIM11_STOP_Msk /*!< TIM11 counter stopped when core is halted */

/******************************************************************************/
/*                                                                            */
/*                           DMA Controller (DMA)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for DMA_ISR register  ********************/
#define DMA_ISR_GIF1_Pos                    (0U)                               
#define DMA_ISR_GIF1_Msk                    (0x1U << DMA_ISR_GIF1_Pos)         /*!< 0x00000001 */
#define DMA_ISR_GIF1                        DMA_ISR_GIF1_Msk                   /*!< Channel 1 Global interrupt flag */
#define DMA_ISR_TCIF1_Pos                   (1U)                               
#define DMA_ISR_TCIF1_Msk                   (0x1U << DMA_ISR_TCIF1_Pos)        /*!< 0x00000002 */
#define DMA_ISR_TCIF1                       DMA_ISR_TCIF1_Msk                  /*!< Channel 1 Transfer Complete flag */
#define DMA_ISR_HTIF1_Pos                   (2U)                               
#define DMA_ISR_HTIF1_Msk                   (0x1U << DMA_ISR_HTIF1_Pos)        /*!< 0x00000004 */
#define DMA_ISR_HTIF1                       DMA_ISR_HTIF1_Msk                  /*!< Channel 1 Half Transfer flag */
#define DMA_ISR_TEIF1_Pos                   (3U)                               
#define DMA_ISR_TEIF1_Msk                   (0x1U << DMA_ISR_TEIF1_Pos)        /*!< 0x00000008 */
#define DMA_ISR_TEIF1                       DMA_ISR_TEIF1_Msk                  /*!< Channel 1 Transfer Error flag */
#define DMA_ISR_GIF2_Pos                    (4U)                               
#define DMA_ISR_GIF2_Msk                    (0x1U << DMA_ISR_GIF2_Pos)         /*!< 0x00000010 */
#define DMA_ISR_GIF2                        DMA_ISR_GIF2_Msk                   /*!< Channel 2 Global interrupt flag */
#define DMA_ISR_TCIF2_Pos                   (5U)                               
#define DMA_ISR_TCIF2_Msk                   (0x1U << DMA_ISR_TCIF2_Pos)        /*!< 0x00000020 */
#define DMA_ISR_TCIF2                       DMA_ISR_TCIF2_Msk                  /*!< Channel 2 Transfer Complete flag */
#define DMA_ISR_HTIF2_Pos                   (6U)                               
#define DMA_ISR_HTIF2_Msk                   (0x1U << DMA_ISR_HTIF2_Pos)        /*!< 0x00000040 */
#define DMA_ISR_HTIF2                       DMA_ISR_HTIF2_Msk                  /*!< Channel 2 Half Transfer flag */
#define DMA_ISR_TEIF2_Pos                   (7U)                               
#define DMA_ISR_TEIF2_Msk                   (0x1U << DMA_ISR_TEIF2_Pos)        /*!< 0x00000080 */
#define DMA_ISR_TEIF2                       DMA_ISR_TEIF2_Msk                  /*!< Channel 2 Transfer Error flag */
#define DMA_ISR_GIF3_Pos                    (8U)                               
#define DMA_ISR_GIF3_Msk                    (0x1U << DMA_ISR_GIF3_Pos)         /*!< 0x00000100 */
#define DMA_ISR_GIF3                        DMA_ISR_GIF3_Msk                   /*!< Channel 3 Global interrupt flag */
#define DMA_ISR_TCIF3_Pos                   (9U)                               
#define DMA_ISR_TCIF3_Msk                   (0x1U << DMA_ISR_TCIF3_Pos)        /*!< 0x00000200 */
#define DMA_ISR_TCIF3                       DMA_ISR_TCIF3_Msk                  /*!< Channel 3 Transfer Complete flag */
#define DMA_ISR_HTIF3_Pos                   (10U)                              
#define DMA_ISR_HTIF3_Msk                   (0x1U << DMA_ISR_HTIF3_Pos)        /*!< 0x00000400 */
#define DMA_ISR_HTIF3                       DMA_ISR_HTIF3_Msk                  /*!< Channel 3 Half Transfer flag */
#define DMA_ISR_TEIF3_Pos                   (11U)                              
#define DMA_ISR_TEIF3_Msk                   (0x1U << DMA_ISR_TEIF3_Pos)        /*!< 0x00000800 */
#define DMA_ISR_TEIF3                       DMA_ISR_TEIF3_Msk                  /*!< Channel 3 Transfer Error flag */
#define DMA_ISR_GIF4_Pos                    (12U)                              
#define DMA_ISR_GIF4_Msk                    (0x1U << DMA_ISR_GIF4_Pos)         /*!< 0x00001000 */
#define DMA_ISR_GIF4                        DMA_ISR_GIF4_Msk                   /*!< Channel 4 Global interrupt flag */
#define DMA_ISR_TCIF4_Pos                   (13U)                              
#define DMA_ISR_TCIF4_Msk                   (0x1U << DMA_ISR_TCIF4_Pos)        /*!< 0x00002000 */
#define DMA_ISR_TCIF4                       DMA_ISR_TCIF4_Msk                  /*!< Channel 4 Transfer Complete flag */
#define DMA_ISR_HTIF4_Pos                   (14U)                              
#define DMA_ISR_HTIF4_Msk                   (0x1U << DMA_ISR_HTIF4_Pos)        /*!< 0x00004000 */
#define DMA_ISR_HTIF4                       DMA_ISR_HTIF4_Msk                  /*!< Channel 4 Half Transfer flag */
#define DMA_ISR_TEIF4_Pos                   (15U)                              
#define DMA_ISR_TEIF4_Msk                   (0x1U << DMA_ISR_TEIF4_Pos)        /*!< 0x00008000 */
#define DMA_ISR_TEIF4                       DMA_ISR_TEIF4_Msk                  /*!< Channel 4 Transfer Error flag */
#define DMA_ISR_GIF5_Pos                    (16U)                              
#define DMA_ISR_GIF5_Msk                    (0x1U << DMA_ISR_GIF5_Pos)         /*!< 0x00010000 */
#define DMA_ISR_GIF5                        DMA_ISR_GIF5_Msk                   /*!< Channel 5 Global interrupt flag */
#define DMA_ISR_TCIF5_Pos                   (17U)                              
#define DMA_ISR_TCIF5_Msk                   (0x1U << DMA_ISR_TCIF5_Pos)        /*!< 0x00020000 */
#define DMA_ISR_TCIF5                       DMA_ISR_TCIF5_Msk                  /*!< Channel 5 Transfer Complete flag */
#define DMA_ISR_HTIF5_Pos                   (18U)                              
#define DMA_ISR_HTIF5_Msk                   (0x1U << DMA_ISR_HTIF5_Pos)        /*!< 0x00040000 */
#define DMA_ISR_HTIF5                       DMA_ISR_HTIF5_Msk                  /*!< Channel 5 Half Transfer flag */
#define DMA_ISR_TEIF5_Pos                   (19U)                              
#define DMA_ISR_TEIF5_Msk                   (0x1U << DMA_ISR_TEIF5_Pos)        /*!< 0x00080000 */
#define DMA_ISR_TEIF5                       DMA_ISR_TEIF5_Msk                  /*!< Channel 5 Transfer Error flag */
#define DMA_ISR_GIF6_Pos                    (20U)                              
#define DMA_ISR_GIF6_Msk                    (0x1U << DMA_ISR_GIF6_Pos)         /*!< 0x00100000 */
#define DMA_ISR_GIF6                        DMA_ISR_GIF6_Msk                   /*!< Channel 6 Global interrupt flag */
#define DMA_ISR_TCIF6_Pos                   (21U)                              
#define DMA_ISR_TCIF6_Msk                   (0x1U << DMA_ISR_TCIF6_Pos)        /*!< 0x00200000 */
#define DMA_ISR_TCIF6                       DMA_ISR_TCIF6_Msk                  /*!< Channel 6 Transfer Complete flag */
#define DMA_ISR_HTIF6_Pos                   (22U)                              
#define DMA_ISR_HTIF6_Msk                   (0x1U << DMA_ISR_HTIF6_Pos)        /*!< 0x00400000 */
#define DMA_ISR_HTIF6                       DMA_ISR_HTIF6_Msk                  /*!< Channel 6 Half Transfer flag */
#define DMA_ISR_TEIF6_Pos                   (23U)                              
#define DMA_ISR_TEIF6_Msk                   (0x1U << DMA_ISR_TEIF6_Pos)        /*!< 0x00800000 */
#define DMA_ISR_TEIF6                       DMA_ISR_TEIF6_Msk                  /*!< Channel 6 Transfer Error flag */
#define DMA_ISR_GIF7_Pos                    (24U)                              
#define DMA_ISR_GIF7_Msk                    (0x1U << DMA_ISR_GIF7_Pos)         /*!< 0x01000000 */
#define DMA_ISR_GIF7                        DMA_ISR_GIF7_Msk                   /*!< Channel 7 Global interrupt flag */
#define DMA_ISR_TCIF7_Pos                   (25U)                              
#define DMA_ISR_TCIF7_Msk                   (0x1U << DMA_ISR_TCIF7_Pos)        /*!< 0x02000000 */
#define DMA_ISR_TCIF7                       DMA_ISR_TCIF7_Msk                  /*!< Channel 7 Transfer Complete flag */
#define DMA_ISR_HTIF7_Pos                   (26U)                              
#define DMA_ISR_HTIF7_Msk                   (0x1U << DMA_ISR_HTIF7_Pos)        /*!< 0x04000000 */
#define DMA_ISR_HTIF7                       DMA_ISR_HTIF7_Msk                  /*!< Channel 7 Half Transfer flag */
#define DMA_ISR_TEIF7_Pos                   (27U)                              
#define DMA_ISR_TEIF7_Msk                   (0x1U << DMA_ISR_TEIF7_Pos)        /*!< 0x08000000 */
#define DMA_ISR_TEIF7                       DMA_ISR_TEIF7_Msk                  /*!< Channel 7 Transfer Error flag */

/*******************  Bit definition for DMA_IFCR register  *******************/
#define DMA_IFCR_CGIF1_Pos                  (0U)                               
#define DMA_IFCR_CGIF1_Msk                  (0x1U << DMA_IFCR_CGIF1_Pos)       /*!< 0x00000001 */
#define DMA_IFCR_CGIF1                      DMA_IFCR_CGIF1_Msk                 /*!< Channel 1 Global interrupt clear */
#define DMA_IFCR_CTCIF1_Pos                 (1U)                               
#define DMA_IFCR_CTCIF1_Msk                 (0x1U << DMA_IFCR_CTCIF1_Pos)      /*!< 0x00000002 */
#define DMA_IFCR_CTCIF1                     DMA_IFCR_CTCIF1_Msk                /*!< Channel 1 Transfer Complete clear */
#define DMA_IFCR_CHTIF1_Pos                 (2U)                               
#define DMA_IFCR_CHTIF1_Msk                 (0x1U << DMA_IFCR_CHTIF1_Pos)      /*!< 0x00000004 */
#define DMA_IFCR_CHTIF1                     DMA_IFCR_CHTIF1_Msk                /*!< Channel 1 Half Transfer clear */
#define DMA_IFCR_CTEIF1_Pos                 (3U)                               
#define DMA_IFCR_CTEIF1_Msk                 (0x1U << DMA_IFCR_CTEIF1_Pos)      /*!< 0x00000008 */
#define DMA_IFCR_CTEIF1                     DMA_IFCR_CTEIF1_Msk                /*!< Channel 1 Transfer Error clear */
#define DMA_IFCR_CGIF2_Pos                  (4U)                               
#define DMA_IFCR_CGIF2_Msk                  (0x1U << DMA_IFCR_CGIF2_Pos)       /*!< 0x00000010 */
#define DMA_IFCR_CGIF2                      DMA_IFCR_CGIF2_Msk                 /*!< Channel 2 Global interrupt clear */
#define DMA_IFCR_CTCIF2_Pos                 (5U)                               
#define DMA_IFCR_CTCIF2_Msk                 (0x1U << DMA_IFCR_CTCIF2_Pos)      /*!< 0x00000020 */
#define DMA_IFCR_CTCIF2                     DMA_IFCR_CTCIF2_Msk                /*!< Channel 2 Transfer Complete clear */
#define DMA_IFCR_CHTIF2_Pos                 (6U)                               
#define DMA_IFCR_CHTIF2_Msk                 (0x1U << DMA_IFCR_CHTIF2_Pos)      /*!< 0x00000040 */
#define DMA_IFCR_CHTIF2                     DMA_IFCR_CHTIF2_Msk                /*!< Channel 2 Half Transfer clear */
#define DMA_IFCR_CTEIF2_Pos                 (7U)                               
#define DMA_IFCR_CTEIF2_Msk                 (0x1U << DMA_IFCR_CTEIF2_Pos)      /*!< 0x00000080 */
#define DMA_IFCR_CTEIF2                     DMA_IFCR_CTEIF2_Msk                /*!< Channel 2 Transfer Error clear */
#define DMA_IFCR_CGIF3_Pos                  (8U)                               
#define DMA_IFCR_CGIF3_Msk                  (0x1U << DMA_IFCR_CGIF3_Pos)       /*!< 0x00000100 */
#define DMA_IFCR_CGIF3                      DMA_IFCR_CGIF3_Msk                 /*!< Channel 3 Global interrupt clear */
#define DMA_IFCR_CTCIF3_Pos                 (9U)                               
#define DMA_IFCR_CTCIF3_Msk                 (0x1U << DMA_IFCR_CTCIF3_Pos)      /*!< 0x00000200 */
#define DMA_IFCR_CTCIF3                     DMA_IFCR_CTCIF3_Msk                /*!< Channel 3 Transfer Complete clear */
#define DMA_IFCR_CHTIF3_Pos                 (10U)                              
#define DMA_IFCR_CHTIF3_Msk                 (0x1U << DMA_IFCR_CHTIF3_Pos)      /*!< 0x00000400 */
#define DMA_IFCR_CHTIF3                     DMA_IFCR_CHTIF3_Msk                /*!< Channel 3 Half Transfer clear */
#define DMA_IFCR_CTEIF3_Pos                 (11U)                              
#define DMA_IFCR_CTEIF3_Msk                 (0x1U << DMA_IFCR_CTEIF3_Pos)      /*!< 0x00000800 */
#define DMA_IFCR_CTEIF3                     DMA_IFCR_CTEIF3_Msk                /*!< Channel 3 Transfer Error clear */
#define DMA_IFCR_CGIF4_Pos                  (12U)                              
#define DMA_IFCR_CGIF4_Msk                  (0x1U << DMA_IFCR_CGIF4_Pos)       /*!< 0x00001000 */
#define DMA_IFCR_CGIF4                      DMA_IFCR_CGIF4_Msk                 /*!< Channel 4 Global interrupt clear */
#define DMA_IFCR_CTCIF4_Pos                 (13U)                              
#define DMA_IFCR_CTCIF4_Msk                 (0x1U << DMA_IFCR_CTCIF4_Pos)      /*!< 0x00002000 */
#define DMA_IFCR_CTCIF4                     DMA_IFCR_CTCIF4_Msk                /*!< Channel 4 Transfer Complete clear */
#define DMA_IFCR_CHTIF4_Pos                 (14U)                              
#define DMA_IFCR_CHTIF4_Msk                 (0x1U << DMA_IFCR_CHTIF4_Pos)      /*!< 0x00004000 */
#define DMA_IFCR_CHTIF4                     DMA_IFCR_CHTIF4_Msk                /*!< Channel 4 Half Transfer clear */
#define DMA_IFCR_CTEIF4_Pos                 (15U)                              
#define DMA_IFCR_CTEIF4_Msk                 (0x1U << DMA_IFCR_CTEIF4_Pos)      /*!< 0x00008000 */
#define DMA_IFCR_CTEIF4                     DMA_IFCR_CTEIF4_Msk                /*!< Channel 4 Transfer Error clear */
#define DMA_IFCR_CGIF5_Pos                  (16U)                              
#define DMA_IFCR_CGIF5_Msk                  (0x1U << DMA_IFCR_CGIF5_Pos)       /*!< 0x00010000 */
#define DMA_IFCR_CGIF5                      DMA_IFCR_CGIF5_Msk                 /*!< Channel 5 Global interrupt clear */
#define DMA_IFCR_CTCIF5_Pos                 (17U)                              
#define DMA_IFCR_CTCIF5_Msk                 (0x1U << DMA_IFCR_CTCIF5_Pos)      /*!< 0x00020000 */
#define DMA_IFCR_CTCIF5                     DMA_IFCR_CTCIF5_Msk                /*!< Channel 5 Transfer Complete clear */
#define DMA_IFCR_CHTIF5_Pos                 (18U)                              
#define DMA_IFCR_CHTIF5_Msk                 (0x1U << DMA_IFCR_CHTIF5_Pos)      /*!< 0x00040000 */
#define DMA_IFCR_CHTIF5                     DMA_IFCR_CHTIF5_Msk                /*!< Channel 5 Half Transfer clear */
#define DMA_IFCR_CTEIF5_Pos                 (19U)                              
#define DMA_IFCR_CTEIF5_Msk                 (0x1U << DMA_IFCR_CTEIF5_Pos)      /*!< 0x00080000 */
#define DMA_IFCR_CTEIF5                     DMA_IFCR_CTEIF5_Msk                /*!< Channel 5 Transfer Error clear */
#define DMA_IFCR_CGIF6_Pos                  (20U)                              
#define DMA_IFCR_CGIF6_Msk                  (0x1U << DMA_IFCR_CGIF6_Pos)       /*!< 0x00100000 */
#define DMA_IFCR_CGIF6                      DMA_IFCR_CGIF6_Msk                 /*!< Channel 6 Global interrupt clear */
#define DMA_IFCR_CTCIF6_Pos                 (21U)                              
#define DMA_IFCR_CTCIF6_Msk                 (0x1U << DMA_IFCR_CTCIF6_Pos)      /*!< 0x00200000 */
#define DMA_IFCR_CTCIF6                     DMA_IFCR_CTCIF6_Msk                /*!< Channel 6 Transfer Complete clear */
#define DMA_IFCR_CHTIF6_Pos                 (22U)                              
#define DMA_IFCR_CHTIF6_Msk                 (0x1U << DMA_IFCR_CHTIF6_Pos)      /*!< 0x00400000 */
#define DMA_IFCR_CHTIF6                     DMA_IFCR_CHTIF6_Msk                /*!< Channel 6 Half Transfer clear */
#define DMA_IFCR_CTEIF6_Pos                 (23U)                              
#define DMA_IFCR_CTEIF6_Msk                 (0x1U << DMA_IFCR_CTEIF6_Pos)      /*!< 0x00800000 */
#define DMA_IFCR_CTEIF6                     DMA_IFCR_CTEIF6_Msk                /*!< Channel 6 Transfer Error clear */
#define DMA_IFCR_CGIF7_Pos                  (24U)                              
#define DMA_IFCR_CGIF7_Msk                  (0x1U << DMA_IFCR_CGIF7_Pos)       /*!< 0x01000000 */
#define DMA_IFCR_CGIF7                      DMA_IFCR_CGIF7_Msk                 /*!< Channel 7 Global interrupt clear */
#define DMA_IFCR_CTCIF7_Pos                 (25U)                              
#define DMA_IFCR_CTCIF7_Msk                 (0x1U << DMA_IFCR_CTCIF7_Pos)      /*!< 0x02000000 */
#define DMA_IFCR_CTCIF7                     DMA_IFCR_CTCIF7_Msk                /*!< Channel 7 Transfer Complete clear */
#define DMA_IFCR_CHTIF7_Pos                 (26U)                              
#define DMA_IFCR_CHTIF7_Msk                 (0x1U << DMA_IFCR_CHTIF7_Pos)      /*!< 0x04000000 */
#define DMA_IFCR_CHTIF7                     DMA_IFCR_CHTIF7_Msk                /*!< Channel 7 Half Transfer clear */
#define DMA_IFCR_CTEIF7_Pos                 (27U)                              
#define DMA_IFCR_CTEIF7_Msk                 (0x1U << DMA_IFCR_CTEIF7_Pos)      /*!< 0x08000000 */
#define DMA_IFCR_CTEIF7                     DMA_IFCR_CTEIF7_Msk                /*!< Channel 7 Transfer Error clear */

/*******************  Bit definition for DMA_CCR register  *******************/
#define DMA_CCR_EN_Pos                      (0U)                               
#define DMA_CCR_EN_Msk                      (0x1U << DMA_CCR_EN_Pos)           /*!< 0x00000001 */
#define DMA_CCR_EN                          DMA_CCR_EN_Msk                     /*!< Channel enable*/
#define DMA_CCR_TCIE_Pos                    (1U)                               
#define DMA_CCR_TCIE_Msk                    (0x1U << DMA_CCR_TCIE_Pos)         /*!< 0x00000002 */
#define DMA_CCR_TCIE                        DMA_CCR_TCIE_Msk                   /*!< Transfer complete interrupt enable */
#define DMA_CCR_HTIE_Pos                    (2U)                               
#define DMA_CCR_HTIE_Msk                    (0x1U << DMA_CCR_HTIE_Pos)         /*!< 0x00000004 */
#define DMA_CCR_HTIE                        DMA_CCR_HTIE_Msk                   /*!< Half Transfer interrupt enable */
#define DMA_CCR_TEIE_Pos                    (3U)                               
#define DMA_CCR_TEIE_Msk                    (0x1U << DMA_CCR_TEIE_Pos)         /*!< 0x00000008 */
#define DMA_CCR_TEIE                        DMA_CCR_TEIE_Msk                   /*!< Transfer error interrupt enable */
#define DMA_CCR_DIR_Pos                     (4U)                               
#define DMA_CCR_DIR_Msk                     (0x1U << DMA_CCR_DIR_Pos)          /*!< 0x00000010 */
#define DMA_CCR_DIR                         DMA_CCR_DIR_Msk                    /*!< Data transfer direction */
#define DMA_CCR_CIRC_Pos                    (5U)                               
#define DMA_CCR_CIRC_Msk                    (0x1U << DMA_CCR_CIRC_Pos)         /*!< 0x00000020 */
#define DMA_CCR_CIRC                        DMA_CCR_CIRC_Msk                   /*!< Circular mode */
#define DMA_CCR_PINC_Pos                    (6U)                               
#define DMA_CCR_PINC_Msk                    (0x1U << DMA_CCR_PINC_Pos)         /*!< 0x00000040 */
#define DMA_CCR_PINC                        DMA_CCR_PINC_Msk                   /*!< Peripheral increment mode */
#define DMA_CCR_MINC_Pos                    (7U)                               
#define DMA_CCR_MINC_Msk                    (0x1U << DMA_CCR_MINC_Pos)         /*!< 0x00000080 */
#define DMA_CCR_MINC                        DMA_CCR_MINC_Msk                   /*!< Memory increment mode */

#define DMA_CCR_PSIZE_Pos                   (8U)                               
#define DMA_CCR_PSIZE_Msk                   (0x3U << DMA_CCR_PSIZE_Pos)        /*!< 0x00000300 */
#define DMA_CCR_PSIZE                       DMA_CCR_PSIZE_Msk                  /*!< PSIZE[1:0] bits (Peripheral size) */
#define DMA_CCR_PSIZE_0                     (0x1U << DMA_CCR_PSIZE_Pos)        /*!< 0x00000100 */
#define DMA_CCR_PSIZE_1                     (0x2U << DMA_CCR_PSIZE_Pos)        /*!< 0x00000200 */

#define DMA_CCR_MSIZE_Pos                   (10U)                              
#define DMA_CCR_MSIZE_Msk                   (0x3U << DMA_CCR_MSIZE_Pos)        /*!< 0x00000C00 */
#define DMA_CCR_MSIZE                       DMA_CCR_MSIZE_Msk                  /*!< MSIZE[1:0] bits (Memory size) */
#define DMA_CCR_MSIZE_0                     (0x1U << DMA_CCR_MSIZE_Pos)        /*!< 0x00000400 */
#define DMA_CCR_MSIZE_1                     (0x2U << DMA_CCR_MSIZE_Pos)        /*!< 0x00000800 */

#define DMA_CCR_PL_Pos                      (12U)                              
#define DMA_CCR_PL_Msk                      (0x3U << DMA_CCR_PL_Pos)           /*!< 0x00003000 */
#define DMA_CCR_PL                          DMA_CCR_PL_Msk                     /*!< PL[1:0] bits(Channel Priority level) */
#define DMA_CCR_PL_0                        (0x1U << DMA_CCR_PL_Pos)           /*!< 0x00001000 */
#define DMA_CCR_PL_1                        (0x2U << DMA_CCR_PL_Pos)           /*!< 0x00002000 */

#define DMA_CCR_MEM2MEM_Pos                 (14U)                              
#define DMA_CCR_MEM2MEM_Msk                 (0x1U << DMA_CCR_MEM2MEM_Pos)      /*!< 0x00004000 */
#define DMA_CCR_MEM2MEM                     DMA_CCR_MEM2MEM_Msk                /*!< Memory to memory mode */

/******************  Bit definition generic for DMA_CNDTR register  *******************/
#define DMA_CNDTR_NDT_Pos                   (0U)                               
#define DMA_CNDTR_NDT_Msk                   (0xFFFFU << DMA_CNDTR_NDT_Pos)     /*!< 0x0000FFFF */
#define DMA_CNDTR_NDT                       DMA_CNDTR_NDT_Msk                  /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR1 register  ******************/
#define DMA_CNDTR1_NDT_Pos                  (0U)                               
#define DMA_CNDTR1_NDT_Msk                  (0xFFFFU << DMA_CNDTR1_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR1_NDT                      DMA_CNDTR1_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR2 register  ******************/
#define DMA_CNDTR2_NDT_Pos                  (0U)                               
#define DMA_CNDTR2_NDT_Msk                  (0xFFFFU << DMA_CNDTR2_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR2_NDT                      DMA_CNDTR2_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR3 register  ******************/
#define DMA_CNDTR3_NDT_Pos                  (0U)                               
#define DMA_CNDTR3_NDT_Msk                  (0xFFFFU << DMA_CNDTR3_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR3_NDT                      DMA_CNDTR3_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR4 register  ******************/
#define DMA_CNDTR4_NDT_Pos                  (0U)                               
#define DMA_CNDTR4_NDT_Msk                  (0xFFFFU << DMA_CNDTR4_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR4_NDT                      DMA_CNDTR4_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR5 register  ******************/
#define DMA_CNDTR5_NDT_Pos                  (0U)                               
#define DMA_CNDTR5_NDT_Msk                  (0xFFFFU << DMA_CNDTR5_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR5_NDT                      DMA_CNDTR5_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR6 register  ******************/
#define DMA_CNDTR6_NDT_Pos                  (0U)                               
#define DMA_CNDTR6_NDT_Msk                  (0xFFFFU << DMA_CNDTR6_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR6_NDT                      DMA_CNDTR6_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition for DMA_CNDTR7 register  ******************/
#define DMA_CNDTR7_NDT_Pos                  (0U)                               
#define DMA_CNDTR7_NDT_Msk                  (0xFFFFU << DMA_CNDTR7_NDT_Pos)    /*!< 0x0000FFFF */
#define DMA_CNDTR7_NDT                      DMA_CNDTR7_NDT_Msk                 /*!< Number of data to Transfer */

/******************  Bit definition generic for DMA_CPAR register  ********************/
#define DMA_CPAR_PA_Pos                     (0U)                               
#define DMA_CPAR_PA_Msk                     (0xFFFFFFFFU << DMA_CPAR_PA_Pos)   /*!< 0xFFFFFFFF */
#define DMA_CPAR_PA                         DMA_CPAR_PA_Msk                    /*!< Peripheral Address */

/******************  Bit definition for DMA_CPAR1 register  *******************/
#define DMA_CPAR1_PA_Pos                    (0U)                               
#define DMA_CPAR1_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR1_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR1_PA                        DMA_CPAR1_PA_Msk                   /*!< Peripheral Address */

/******************  Bit definition for DMA_CPAR2 register  *******************/
#define DMA_CPAR2_PA_Pos                    (0U)                               
#define DMA_CPAR2_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR2_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR2_PA                        DMA_CPAR2_PA_Msk                   /*!< Peripheral Address */

/******************  Bit definition for DMA_CPAR3 register  *******************/
#define DMA_CPAR3_PA_Pos                    (0U)                               
#define DMA_CPAR3_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR3_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR3_PA                        DMA_CPAR3_PA_Msk                   /*!< Peripheral Address */


/******************  Bit definition for DMA_CPAR4 register  *******************/
#define DMA_CPAR4_PA_Pos                    (0U)                               
#define DMA_CPAR4_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR4_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR4_PA                        DMA_CPAR4_PA_Msk                   /*!< Peripheral Address */

/******************  Bit definition for DMA_CPAR5 register  *******************/
#define DMA_CPAR5_PA_Pos                    (0U)                               
#define DMA_CPAR5_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR5_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR5_PA                        DMA_CPAR5_PA_Msk                   /*!< Peripheral Address */

/******************  Bit definition for DMA_CPAR6 register  *******************/
#define DMA_CPAR6_PA_Pos                    (0U)                               
#define DMA_CPAR6_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR6_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR6_PA                        DMA_CPAR6_PA_Msk                   /*!< Peripheral Address */


/******************  Bit definition for DMA_CPAR7 register  *******************/
#define DMA_CPAR7_PA_Pos                    (0U)                               
#define DMA_CPAR7_PA_Msk                    (0xFFFFFFFFU << DMA_CPAR7_PA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CPAR7_PA                        DMA_CPAR7_PA_Msk                   /*!< Peripheral Address */

/******************  Bit definition generic for DMA_CMAR register  ********************/
#define DMA_CMAR_MA_Pos                     (0U)                               
#define DMA_CMAR_MA_Msk                     (0xFFFFFFFFU << DMA_CMAR_MA_Pos)   /*!< 0xFFFFFFFF */
#define DMA_CMAR_MA                         DMA_CMAR_MA_Msk                    /*!< Memory Address */

/******************  Bit definition for DMA_CMAR1 register  *******************/
#define DMA_CMAR1_MA_Pos                    (0U)                               
#define DMA_CMAR1_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR1_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR1_MA                        DMA_CMAR1_MA_Msk                   /*!< Memory Address */

/******************  Bit definition for DMA_CMAR2 register  *******************/
#define DMA_CMAR2_MA_Pos                    (0U)                               
#define DMA_CMAR2_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR2_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR2_MA                        DMA_CMAR2_MA_Msk                   /*!< Memory Address */

/******************  Bit definition for DMA_CMAR3 register  *******************/
#define DMA_CMAR3_MA_Pos                    (0U)                               
#define DMA_CMAR3_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR3_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR3_MA                        DMA_CMAR3_MA_Msk                   /*!< Memory Address */


/******************  Bit definition for DMA_CMAR4 register  *******************/
#define DMA_CMAR4_MA_Pos                    (0U)                               
#define DMA_CMAR4_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR4_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR4_MA                        DMA_CMAR4_MA_Msk                   /*!< Memory Address */

/******************  Bit definition for DMA_CMAR5 register  *******************/
#define DMA_CMAR5_MA_Pos                    (0U)                               
#define DMA_CMAR5_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR5_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR5_MA                        DMA_CMAR5_MA_Msk                   /*!< Memory Address */

/******************  Bit definition for DMA_CMAR6 register  *******************/
#define DMA_CMAR6_MA_Pos                    (0U)                               
#define DMA_CMAR6_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR6_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR6_MA                        DMA_CMAR6_MA_Msk                   /*!< Memory Address */

/******************  Bit definition for DMA_CMAR7 register  *******************/
#define DMA_CMAR7_MA_Pos                    (0U)                               
#define DMA_CMAR7_MA_Msk                    (0xFFFFFFFFU << DMA_CMAR7_MA_Pos)  /*!< 0xFFFFFFFF */
#define DMA_CMAR7_MA                        DMA_CMAR7_MA_Msk                   /*!< Memory Address */

/******************************************************************************/
/*                                                                            */
/*                  External Interrupt/Event Controller (EXTI)                */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for EXTI_IMR register  *******************/
#define EXTI_IMR_MR0_Pos                    (0U)                               
#define EXTI_IMR_MR0_Msk                    (0x1U << EXTI_IMR_MR0_Pos)         /*!< 0x00000001 */
#define EXTI_IMR_MR0                        EXTI_IMR_MR0_Msk                   /*!< Interrupt Mask on line 0 */
#define EXTI_IMR_MR1_Pos                    (1U)                               
#define EXTI_IMR_MR1_Msk                    (0x1U << EXTI_IMR_MR1_Pos)         /*!< 0x00000002 */
#define EXTI_IMR_MR1                        EXTI_IMR_MR1_Msk                   /*!< Interrupt Mask on line 1 */
#define EXTI_IMR_MR2_Pos                    (2U)                               
#define EXTI_IMR_MR2_Msk                    (0x1U << EXTI_IMR_MR2_Pos)         /*!< 0x00000004 */
#define EXTI_IMR_MR2                        EXTI_IMR_MR2_Msk                   /*!< Interrupt Mask on line 2 */
#define EXTI_IMR_MR3_Pos                    (3U)                               
#define EXTI_IMR_MR3_Msk                    (0x1U << EXTI_IMR_MR3_Pos)         /*!< 0x00000008 */
#define EXTI_IMR_MR3                        EXTI_IMR_MR3_Msk                   /*!< Interrupt Mask on line 3 */
#define EXTI_IMR_MR4_Pos                    (4U)                               
#define EXTI_IMR_MR4_Msk                    (0x1U << EXTI_IMR_MR4_Pos)         /*!< 0x00000010 */
#define EXTI_IMR_MR4                        EXTI_IMR_MR4_Msk                   /*!< Interrupt Mask on line 4 */
#define EXTI_IMR_MR5_Pos                    (5U)                               
#define EXTI_IMR_MR5_Msk                    (0x1U << EXTI_IMR_MR5_Pos)         /*!< 0x00000020 */
#define EXTI_IMR_MR5                        EXTI_IMR_MR5_Msk                   /*!< Interrupt Mask on line 5 */
#define EXTI_IMR_MR6_Pos                    (6U)                               
#define EXTI_IMR_MR6_Msk                    (0x1U << EXTI_IMR_MR6_Pos)         /*!< 0x00000040 */
#define EXTI_IMR_MR6                        EXTI_IMR_MR6_Msk                   /*!< Interrupt Mask on line 6 */
#define EXTI_IMR_MR7_Pos                    (7U)                               
#define EXTI_IMR_MR7_Msk                    (0x1U << EXTI_IMR_MR7_Pos)         /*!< 0x00000080 */
#define EXTI_IMR_MR7                        EXTI_IMR_MR7_Msk                   /*!< Interrupt Mask on line 7 */
#define EXTI_IMR_MR8_Pos                    (8U)                               
#define EXTI_IMR_MR8_Msk                    (0x1U << EXTI_IMR_MR8_Pos)         /*!< 0x00000100 */
#define EXTI_IMR_MR8                        EXTI_IMR_MR8_Msk                   /*!< Interrupt Mask on line 8 */
#define EXTI_IMR_MR9_Pos                    (9U)                               
#define EXTI_IMR_MR9_Msk                    (0x1U << EXTI_IMR_MR9_Pos)         /*!< 0x00000200 */
#define EXTI_IMR_MR9                        EXTI_IMR_MR9_Msk                   /*!< Interrupt Mask on line 9 */
#define EXTI_IMR_MR10_Pos                   (10U)                              
#define EXTI_IMR_MR10_Msk                   (0x1U << EXTI_IMR_MR10_Pos)        /*!< 0x00000400 */
#define EXTI_IMR_MR10                       EXTI_IMR_MR10_Msk                  /*!< Interrupt Mask on line 10 */
#define EXTI_IMR_MR11_Pos                   (11U)                              
#define EXTI_IMR_MR11_Msk                   (0x1U << EXTI_IMR_MR11_Pos)        /*!< 0x00000800 */
#define EXTI_IMR_MR11                       EXTI_IMR_MR11_Msk                  /*!< Interrupt Mask on line 11 */
#define EXTI_IMR_MR12_Pos                   (12U)                              
#define EXTI_IMR_MR12_Msk                   (0x1U << EXTI_IMR_MR12_Pos)        /*!< 0x00001000 */
#define EXTI_IMR_MR12                       EXTI_IMR_MR12_Msk                  /*!< Interrupt Mask on line 12 */
#define EXTI_IMR_MR13_Pos                   (13U)                              
#define EXTI_IMR_MR13_Msk                   (0x1U << EXTI_IMR_MR13_Pos)        /*!< 0x00002000 */
#define EXTI_IMR_MR13                       EXTI_IMR_MR13_Msk                  /*!< Interrupt Mask on line 13 */
#define EXTI_IMR_MR14_Pos                   (14U)                              
#define EXTI_IMR_MR14_Msk                   (0x1U << EXTI_IMR_MR14_Pos)        /*!< 0x00004000 */
#define EXTI_IMR_MR14                       EXTI_IMR_MR14_Msk                  /*!< Interrupt Mask on line 14 */
#define EXTI_IMR_MR15_Pos                   (15U)                              
#define EXTI_IMR_MR15_Msk                   (0x1U << EXTI_IMR_MR15_Pos)        /*!< 0x00008000 */
#define EXTI_IMR_MR15                       EXTI_IMR_MR15_Msk                  /*!< Interrupt Mask on line 15 */
#define EXTI_IMR_MR16_Pos                   (16U)                              
#define EXTI_IMR_MR16_Msk                   (0x1U << EXTI_IMR_MR16_Pos)        /*!< 0x00010000 */
#define EXTI_IMR_MR16                       EXTI_IMR_MR16_Msk                  /*!< Interrupt Mask on line 16 */
#define EXTI_IMR_MR17_Pos                   (17U)                              
#define EXTI_IMR_MR17_Msk                   (0x1U << EXTI_IMR_MR17_Pos)        /*!< 0x00020000 */
#define EXTI_IMR_MR17                       EXTI_IMR_MR17_Msk                  /*!< Interrupt Mask on line 17 */
#define EXTI_IMR_MR18_Pos                   (18U)                              
#define EXTI_IMR_MR18_Msk                   (0x1U << EXTI_IMR_MR18_Pos)        /*!< 0x00040000 */
#define EXTI_IMR_MR18                       EXTI_IMR_MR18_Msk                  /*!< Interrupt Mask on line 18 */
#define EXTI_IMR_MR19_Pos                   (19U)                              
#define EXTI_IMR_MR19_Msk                   (0x1U << EXTI_IMR_MR19_Pos)        /*!< 0x00080000 */
#define EXTI_IMR_MR19                       EXTI_IMR_MR19_Msk                  /*!< Interrupt Mask on line 19 */
#define EXTI_IMR_MR20_Pos                   (20U)                              
#define EXTI_IMR_MR20_Msk                   (0x1U << EXTI_IMR_MR20_Pos)        /*!< 0x00100000 */
#define EXTI_IMR_MR20                       EXTI_IMR_MR20_Msk                  /*!< Interrupt Mask on line 20 */
#define EXTI_IMR_MR21_Pos                   (21U)                              
#define EXTI_IMR_MR21_Msk                   (0x1U << EXTI_IMR_MR21_Pos)        /*!< 0x00200000 */
#define EXTI_IMR_MR21                       EXTI_IMR_MR21_Msk                  /*!< Interrupt Mask on line 21 */
#define EXTI_IMR_MR22_Pos                   (22U)                              
#define EXTI_IMR_MR22_Msk                   (0x1U << EXTI_IMR_MR22_Pos)        /*!< 0x00400000 */
#define EXTI_IMR_MR22                       EXTI_IMR_MR22_Msk                  /*!< Interrupt Mask on line 22 */
#define EXTI_IMR_MR23_Pos                   (23U)                              
#define EXTI_IMR_MR23_Msk                   (0x1U << EXTI_IMR_MR23_Pos)        /*!< 0x00800000 */
#define EXTI_IMR_MR23                       EXTI_IMR_MR23_Msk                  /*!< Interrupt Mask on line 23 */

/* References Defines */
#define  EXTI_IMR_IM0 EXTI_IMR_MR0
#define  EXTI_IMR_IM1 EXTI_IMR_MR1
#define  EXTI_IMR_IM2 EXTI_IMR_MR2
#define  EXTI_IMR_IM3 EXTI_IMR_MR3
#define  EXTI_IMR_IM4 EXTI_IMR_MR4
#define  EXTI_IMR_IM5 EXTI_IMR_MR5
#define  EXTI_IMR_IM6 EXTI_IMR_MR6
#define  EXTI_IMR_IM7 EXTI_IMR_MR7
#define  EXTI_IMR_IM8 EXTI_IMR_MR8
#define  EXTI_IMR_IM9 EXTI_IMR_MR9
#define  EXTI_IMR_IM10 EXTI_IMR_MR10
#define  EXTI_IMR_IM11 EXTI_IMR_MR11
#define  EXTI_IMR_IM12 EXTI_IMR_MR12
#define  EXTI_IMR_IM13 EXTI_IMR_MR13
#define  EXTI_IMR_IM14 EXTI_IMR_MR14
#define  EXTI_IMR_IM15 EXTI_IMR_MR15
#define  EXTI_IMR_IM16 EXTI_IMR_MR16
#define  EXTI_IMR_IM17 EXTI_IMR_MR17
#define  EXTI_IMR_IM18 EXTI_IMR_MR18
#define  EXTI_IMR_IM19 EXTI_IMR_MR19
#define  EXTI_IMR_IM20 EXTI_IMR_MR20
#define  EXTI_IMR_IM21 EXTI_IMR_MR21
#define  EXTI_IMR_IM22 EXTI_IMR_MR22
/* Category 3, 4 & 5 */
#define  EXTI_IMR_IM23 EXTI_IMR_MR23
#define EXTI_IMR_IM_Pos                     (0U)                               
#define EXTI_IMR_IM_Msk                     (0xFFFFFFU << EXTI_IMR_IM_Pos)     /*!< 0x00FFFFFF */
#define EXTI_IMR_IM                         EXTI_IMR_IM_Msk                    /*!< Interrupt Mask All */

/*******************  Bit definition for EXTI_EMR register  *******************/
#define EXTI_EMR_MR0_Pos                    (0U)                               
#define EXTI_EMR_MR0_Msk                    (0x1U << EXTI_EMR_MR0_Pos)         /*!< 0x00000001 */
#define EXTI_EMR_MR0                        EXTI_EMR_MR0_Msk                   /*!< Event Mask on line 0 */
#define EXTI_EMR_MR1_Pos                    (1U)                               
#define EXTI_EMR_MR1_Msk                    (0x1U << EXTI_EMR_MR1_Pos)         /*!< 0x00000002 */
#define EXTI_EMR_MR1                        EXTI_EMR_MR1_Msk                   /*!< Event Mask on line 1 */
#define EXTI_EMR_MR2_Pos                    (2U)                               
#define EXTI_EMR_MR2_Msk                    (0x1U << EXTI_EMR_MR2_Pos)         /*!< 0x00000004 */
#define EXTI_EMR_MR2                        EXTI_EMR_MR2_Msk                   /*!< Event Mask on line 2 */
#define EXTI_EMR_MR3_Pos                    (3U)                               
#define EXTI_EMR_MR3_Msk                    (0x1U << EXTI_EMR_MR3_Pos)         /*!< 0x00000008 */
#define EXTI_EMR_MR3                        EXTI_EMR_MR3_Msk                   /*!< Event Mask on line 3 */
#define EXTI_EMR_MR4_Pos                    (4U)                               
#define EXTI_EMR_MR4_Msk                    (0x1U << EXTI_EMR_MR4_Pos)         /*!< 0x00000010 */
#define EXTI_EMR_MR4                        EXTI_EMR_MR4_Msk                   /*!< Event Mask on line 4 */
#define EXTI_EMR_MR5_Pos                    (5U)                               
#define EXTI_EMR_MR5_Msk                    (0x1U << EXTI_EMR_MR5_Pos)         /*!< 0x00000020 */
#define EXTI_EMR_MR5                        EXTI_EMR_MR5_Msk                   /*!< Event Mask on line 5 */
#define EXTI_EMR_MR6_Pos                    (6U)                               
#define EXTI_EMR_MR6_Msk                    (0x1U << EXTI_EMR_MR6_Pos)         /*!< 0x00000040 */
#define EXTI_EMR_MR6                        EXTI_EMR_MR6_Msk                   /*!< Event Mask on line 6 */
#define EXTI_EMR_MR7_Pos                    (7U)                               
#define EXTI_EMR_MR7_Msk                    (0x1U << EXTI_EMR_MR7_Pos)         /*!< 0x00000080 */
#define EXTI_EMR_MR7                        EXTI_EMR_MR7_Msk                   /*!< Event Mask on line 7 */
#define EXTI_EMR_MR8_Pos                    (8U)                               
#define EXTI_EMR_MR8_Msk                    (0x1U << EXTI_EMR_MR8_Pos)         /*!< 0x00000100 */
#define EXTI_EMR_MR8                        EXTI_EMR_MR8_Msk                   /*!< Event Mask on line 8 */
#define EXTI_EMR_MR9_Pos                    (9U)                               
#define EXTI_EMR_MR9_Msk                    (0x1U << EXTI_EMR_MR9_Pos)         /*!< 0x00000200 */
#define EXTI_EMR_MR9                        EXTI_EMR_MR9_Msk                   /*!< Event Mask on line 9 */
#define EXTI_EMR_MR10_Pos                   (10U)                              
#define EXTI_EMR_MR10_Msk                   (0x1U << EXTI_EMR_MR10_Pos)        /*!< 0x00000400 */
#define EXTI_EMR_MR10                       EXTI_EMR_MR10_Msk                  /*!< Event Mask on line 10 */
#define EXTI_EMR_MR11_Pos                   (11U)                              
#define EXTI_EMR_MR11_Msk                   (0x1U << EXTI_EMR_MR11_Pos)        /*!< 0x00000800 */
#define EXTI_EMR_MR11                       EXTI_EMR_MR11_Msk                  /*!< Event Mask on line 11 */
#define EXTI_EMR_MR12_Pos                   (12U)                              
#define EXTI_EMR_MR12_Msk                   (0x1U << EXTI_EMR_MR12_Pos)        /*!< 0x00001000 */
#define EXTI_EMR_MR12                       EXTI_EMR_MR12_Msk                  /*!< Event Mask on line 12 */
#define EXTI_EMR_MR13_Pos                   (13U)                              
#define EXTI_EMR_MR13_Msk                   (0x1U << EXTI_EMR_MR13_Pos)        /*!< 0x00002000 */
#define EXTI_EMR_MR13                       EXTI_EMR_MR13_Msk                  /*!< Event Mask on line 13 */
#define EXTI_EMR_MR14_Pos                   (14U)                              
#define EXTI_EMR_MR14_Msk                   (0x1U << EXTI_EMR_MR14_Pos)        /*!< 0x00004000 */
#define EXTI_EMR_MR14                       EXTI_EMR_MR14_Msk                  /*!< Event Mask on line 14 */
#define EXTI_EMR_MR15_Pos                   (15U)                              
#define EXTI_EMR_MR15_Msk                   (0x1U << EXTI_EMR_MR15_Pos)        /*!< 0x00008000 */
#define EXTI_EMR_MR15                       EXTI_EMR_MR15_Msk                  /*!< Event Mask on line 15 */
#define EXTI_EMR_MR16_Pos                   (16U)                              
#define EXTI_EMR_MR16_Msk                   (0x1U << EXTI_EMR_MR16_Pos)        /*!< 0x00010000 */
#define EXTI_EMR_MR16                       EXTI_EMR_MR16_Msk                  /*!< Event Mask on line 16 */
#define EXTI_EMR_MR17_Pos                   (17U)                              
#define EXTI_EMR_MR17_Msk                   (0x1U << EXTI_EMR_MR17_Pos)        /*!< 0x00020000 */
#define EXTI_EMR_MR17                       EXTI_EMR_MR17_Msk                  /*!< Event Mask on line 17 */
#define EXTI_EMR_MR18_Pos                   (18U)                              
#define EXTI_EMR_MR18_Msk                   (0x1U << EXTI_EMR_MR18_Pos)        /*!< 0x00040000 */
#define EXTI_EMR_MR18                       EXTI_EMR_MR18_Msk                  /*!< Event Mask on line 18 */
#define EXTI_EMR_MR19_Pos                   (19U)                              
#define EXTI_EMR_MR19_Msk                   (0x1U << EXTI_EMR_MR19_Pos)        /*!< 0x00080000 */
#define EXTI_EMR_MR19                       EXTI_EMR_MR19_Msk                  /*!< Event Mask on line 19 */
#define EXTI_EMR_MR20_Pos                   (20U)                              
#define EXTI_EMR_MR20_Msk                   (0x1U << EXTI_EMR_MR20_Pos)        /*!< 0x00100000 */
#define EXTI_EMR_MR20                       EXTI_EMR_MR20_Msk                  /*!< Event Mask on line 20 */
#define EXTI_EMR_MR21_Pos                   (21U)                              
#define EXTI_EMR_MR21_Msk                   (0x1U << EXTI_EMR_MR21_Pos)        /*!< 0x00200000 */
#define EXTI_EMR_MR21                       EXTI_EMR_MR21_Msk                  /*!< Event Mask on line 21 */
#define EXTI_EMR_MR22_Pos                   (22U)                              
#define EXTI_EMR_MR22_Msk                   (0x1U << EXTI_EMR_MR22_Pos)        /*!< 0x00400000 */
#define EXTI_EMR_MR22                       EXTI_EMR_MR22_Msk                  /*!< Event Mask on line 22 */
#define EXTI_EMR_MR23_Pos                   (23U)                              
#define EXTI_EMR_MR23_Msk                   (0x1U << EXTI_EMR_MR23_Pos)        /*!< 0x00800000 */
#define EXTI_EMR_MR23                       EXTI_EMR_MR23_Msk                  /*!< Event Mask on line 23 */

/* References Defines */
#define  EXTI_EMR_EM0 EXTI_EMR_MR0
#define  EXTI_EMR_EM1 EXTI_EMR_MR1
#define  EXTI_EMR_EM2 EXTI_EMR_MR2
#define  EXTI_EMR_EM3 EXTI_EMR_MR3
#define  EXTI_EMR_EM4 EXTI_EMR_MR4
#define  EXTI_EMR_EM5 EXTI_EMR_MR5
#define  EXTI_EMR_EM6 EXTI_EMR_MR6
#define  EXTI_EMR_EM7 EXTI_EMR_MR7
#define  EXTI_EMR_EM8 EXTI_EMR_MR8
#define  EXTI_EMR_EM9 EXTI_EMR_MR9
#define  EXTI_EMR_EM10 EXTI_EMR_MR10
#define  EXTI_EMR_EM11 EXTI_EMR_MR11
#define  EXTI_EMR_EM12 EXTI_EMR_MR12
#define  EXTI_EMR_EM13 EXTI_EMR_MR13
#define  EXTI_EMR_EM14 EXTI_EMR_MR14
#define  EXTI_EMR_EM15 EXTI_EMR_MR15
#define  EXTI_EMR_EM16 EXTI_EMR_MR16
#define  EXTI_EMR_EM17 EXTI_EMR_MR17
#define  EXTI_EMR_EM18 EXTI_EMR_MR18
#define  EXTI_EMR_EM19 EXTI_EMR_MR19
#define  EXTI_EMR_EM20 EXTI_EMR_MR20
#define  EXTI_EMR_EM21 EXTI_EMR_MR21
#define  EXTI_EMR_EM22 EXTI_EMR_MR22
#define  EXTI_EMR_EM23 EXTI_EMR_MR23

/******************  Bit definition for EXTI_RTSR register  *******************/
#define EXTI_RTSR_TR0_Pos                   (0U)                               
#define EXTI_RTSR_TR0_Msk                   (0x1U << EXTI_RTSR_TR0_Pos)        /*!< 0x00000001 */
#define EXTI_RTSR_TR0                       EXTI_RTSR_TR0_Msk                  /*!< Rising trigger event configuration bit of line 0 */
#define EXTI_RTSR_TR1_Pos                   (1U)                               
#define EXTI_RTSR_TR1_Msk                   (0x1U << EXTI_RTSR_TR1_Pos)        /*!< 0x00000002 */
#define EXTI_RTSR_TR1                       EXTI_RTSR_TR1_Msk                  /*!< Rising trigger event configuration bit of line 1 */
#define EXTI_RTSR_TR2_Pos                   (2U)                               
#define EXTI_RTSR_TR2_Msk                   (0x1U << EXTI_RTSR_TR2_Pos)        /*!< 0x00000004 */
#define EXTI_RTSR_TR2                       EXTI_RTSR_TR2_Msk                  /*!< Rising trigger event configuration bit of line 2 */
#define EXTI_RTSR_TR3_Pos                   (3U)                               
#define EXTI_RTSR_TR3_Msk                   (0x1U << EXTI_RTSR_TR3_Pos)        /*!< 0x00000008 */
#define EXTI_RTSR_TR3                       EXTI_RTSR_TR3_Msk                  /*!< Rising trigger event configuration bit of line 3 */
#define EXTI_RTSR_TR4_Pos                   (4U)                               
#define EXTI_RTSR_TR4_Msk                   (0x1U << EXTI_RTSR_TR4_Pos)        /*!< 0x00000010 */
#define EXTI_RTSR_TR4                       EXTI_RTSR_TR4_Msk                  /*!< Rising trigger event configuration bit of line 4 */
#define EXTI_RTSR_TR5_Pos                   (5U)                               
#define EXTI_RTSR_TR5_Msk                   (0x1U << EXTI_RTSR_TR5_Pos)        /*!< 0x00000020 */
#define EXTI_RTSR_TR5                       EXTI_RTSR_TR5_Msk                  /*!< Rising trigger event configuration bit of line 5 */
#define EXTI_RTSR_TR6_Pos                   (6U)                               
#define EXTI_RTSR_TR6_Msk                   (0x1U << EXTI_RTSR_TR6_Pos)        /*!< 0x00000040 */
#define EXTI_RTSR_TR6                       EXTI_RTSR_TR6_Msk                  /*!< Rising trigger event configuration bit of line 6 */
#define EXTI_RTSR_TR7_Pos                   (7U)                               
#define EXTI_RTSR_TR7_Msk                   (0x1U << EXTI_RTSR_TR7_Pos)        /*!< 0x00000080 */
#define EXTI_RTSR_TR7                       EXTI_RTSR_TR7_Msk                  /*!< Rising trigger event configuration bit of line 7 */
#define EXTI_RTSR_TR8_Pos                   (8U)                               
#define EXTI_RTSR_TR8_Msk                   (0x1U << EXTI_RTSR_TR8_Pos)        /*!< 0x00000100 */
#define EXTI_RTSR_TR8                       EXTI_RTSR_TR8_Msk                  /*!< Rising trigger event configuration bit of line 8 */
#define EXTI_RTSR_TR9_Pos                   (9U)                               
#define EXTI_RTSR_TR9_Msk                   (0x1U << EXTI_RTSR_TR9_Pos)        /*!< 0x00000200 */
#define EXTI_RTSR_TR9                       EXTI_RTSR_TR9_Msk                  /*!< Rising trigger event configuration bit of line 9 */
#define EXTI_RTSR_TR10_Pos                  (10U)                              
#define EXTI_RTSR_TR10_Msk                  (0x1U << EXTI_RTSR_TR10_Pos)       /*!< 0x00000400 */
#define EXTI_RTSR_TR10                      EXTI_RTSR_TR10_Msk                 /*!< Rising trigger event configuration bit of line 10 */
#define EXTI_RTSR_TR11_Pos                  (11U)                              
#define EXTI_RTSR_TR11_Msk                  (0x1U << EXTI_RTSR_TR11_Pos)       /*!< 0x00000800 */
#define EXTI_RTSR_TR11                      EXTI_RTSR_TR11_Msk                 /*!< Rising trigger event configuration bit of line 11 */
#define EXTI_RTSR_TR12_Pos                  (12U)                              
#define EXTI_RTSR_TR12_Msk                  (0x1U << EXTI_RTSR_TR12_Pos)       /*!< 0x00001000 */
#define EXTI_RTSR_TR12                      EXTI_RTSR_TR12_Msk                 /*!< Rising trigger event configuration bit of line 12 */
#define EXTI_RTSR_TR13_Pos                  (13U)                              
#define EXTI_RTSR_TR13_Msk                  (0x1U << EXTI_RTSR_TR13_Pos)       /*!< 0x00002000 */
#define EXTI_RTSR_TR13                      EXTI_RTSR_TR13_Msk                 /*!< Rising trigger event configuration bit of line 13 */
#define EXTI_RTSR_TR14_Pos                  (14U)                              
#define EXTI_RTSR_TR14_Msk                  (0x1U << EXTI_RTSR_TR14_Pos)       /*!< 0x00004000 */
#define EXTI_RTSR_TR14                      EXTI_RTSR_TR14_Msk                 /*!< Rising trigger event configuration bit of line 14 */
#define EXTI_RTSR_TR15_Pos                  (15U)                              
#define EXTI_RTSR_TR15_Msk                  (0x1U << EXTI_RTSR_TR15_Pos)       /*!< 0x00008000 */
#define EXTI_RTSR_TR15                      EXTI_RTSR_TR15_Msk                 /*!< Rising trigger event configuration bit of line 15 */
#define EXTI_RTSR_TR16_Pos                  (16U)                              
#define EXTI_RTSR_TR16_Msk                  (0x1U << EXTI_RTSR_TR16_Pos)       /*!< 0x00010000 */
#define EXTI_RTSR_TR16                      EXTI_RTSR_TR16_Msk                 /*!< Rising trigger event configuration bit of line 16 */
#define EXTI_RTSR_TR17_Pos                  (17U)                              
#define EXTI_RTSR_TR17_Msk                  (0x1U << EXTI_RTSR_TR17_Pos)       /*!< 0x00020000 */
#define EXTI_RTSR_TR17                      EXTI_RTSR_TR17_Msk                 /*!< Rising trigger event configuration bit of line 17 */
#define EXTI_RTSR_TR18_Pos                  (18U)                              
#define EXTI_RTSR_TR18_Msk                  (0x1U << EXTI_RTSR_TR18_Pos)       /*!< 0x00040000 */
#define EXTI_RTSR_TR18                      EXTI_RTSR_TR18_Msk                 /*!< Rising trigger event configuration bit of line 18 */
#define EXTI_RTSR_TR19_Pos                  (19U)                              
#define EXTI_RTSR_TR19_Msk                  (0x1U << EXTI_RTSR_TR19_Pos)       /*!< 0x00080000 */
#define EXTI_RTSR_TR19                      EXTI_RTSR_TR19_Msk                 /*!< Rising trigger event configuration bit of line 19 */
#define EXTI_RTSR_TR20_Pos                  (20U)                              
#define EXTI_RTSR_TR20_Msk                  (0x1U << EXTI_RTSR_TR20_Pos)       /*!< 0x00100000 */
#define EXTI_RTSR_TR20                      EXTI_RTSR_TR20_Msk                 /*!< Rising trigger event configuration bit of line 20 */
#define EXTI_RTSR_TR21_Pos                  (21U)                              
#define EXTI_RTSR_TR21_Msk                  (0x1U << EXTI_RTSR_TR21_Pos)       /*!< 0x00200000 */
#define EXTI_RTSR_TR21                      EXTI_RTSR_TR21_Msk                 /*!< Rising trigger event configuration bit of line 21 */
#define EXTI_RTSR_TR22_Pos                  (22U)                              
#define EXTI_RTSR_TR22_Msk                  (0x1U << EXTI_RTSR_TR22_Pos)       /*!< 0x00400000 */
#define EXTI_RTSR_TR22                      EXTI_RTSR_TR22_Msk                 /*!< Rising trigger event configuration bit of line 22 */
#define EXTI_RTSR_TR23_Pos                  (23U)                              
#define EXTI_RTSR_TR23_Msk                  (0x1U << EXTI_RTSR_TR23_Pos)       /*!< 0x00800000 */
#define EXTI_RTSR_TR23                      EXTI_RTSR_TR23_Msk                 /*!< Rising trigger event configuration bit of line 23 */

/* References Defines */
#define  EXTI_RTSR_RT0 EXTI_RTSR_TR0
#define  EXTI_RTSR_RT1 EXTI_RTSR_TR1
#define  EXTI_RTSR_RT2 EXTI_RTSR_TR2
#define  EXTI_RTSR_RT3 EXTI_RTSR_TR3
#define  EXTI_RTSR_RT4 EXTI_RTSR_TR4
#define  EXTI_RTSR_RT5 EXTI_RTSR_TR5
#define  EXTI_RTSR_RT6 EXTI_RTSR_TR6
#define  EXTI_RTSR_RT7 EXTI_RTSR_TR7
#define  EXTI_RTSR_RT8 EXTI_RTSR_TR8
#define  EXTI_RTSR_RT9 EXTI_RTSR_TR9
#define  EXTI_RTSR_RT10 EXTI_RTSR_TR10
#define  EXTI_RTSR_RT11 EXTI_RTSR_TR11
#define  EXTI_RTSR_RT12 EXTI_RTSR_TR12
#define  EXTI_RTSR_RT13 EXTI_RTSR_TR13
#define  EXTI_RTSR_RT14 EXTI_RTSR_TR14
#define  EXTI_RTSR_RT15 EXTI_RTSR_TR15
#define  EXTI_RTSR_RT16 EXTI_RTSR_TR16
#define  EXTI_RTSR_RT17 EXTI_RTSR_TR17
#define  EXTI_RTSR_RT18 EXTI_RTSR_TR18
#define  EXTI_RTSR_RT19 EXTI_RTSR_TR19
#define  EXTI_RTSR_RT20 EXTI_RTSR_TR20
#define  EXTI_RTSR_RT21 EXTI_RTSR_TR21
#define  EXTI_RTSR_RT22 EXTI_RTSR_TR22
#define  EXTI_RTSR_RT23 EXTI_RTSR_TR23

/******************  Bit definition for EXTI_FTSR register  *******************/
#define EXTI_FTSR_TR0_Pos                   (0U)                               
#define EXTI_FTSR_TR0_Msk                   (0x1U << EXTI_FTSR_TR0_Pos)        /*!< 0x00000001 */
#define EXTI_FTSR_TR0                       EXTI_FTSR_TR0_Msk                  /*!< Falling trigger event configuration bit of line 0 */
#define EXTI_FTSR_TR1_Pos                   (1U)                               
#define EXTI_FTSR_TR1_Msk                   (0x1U << EXTI_FTSR_TR1_Pos)        /*!< 0x00000002 */
#define EXTI_FTSR_TR1                       EXTI_FTSR_TR1_Msk                  /*!< Falling trigger event configuration bit of line 1 */
#define EXTI_FTSR_TR2_Pos                   (2U)                               
#define EXTI_FTSR_TR2_Msk                   (0x1U << EXTI_FTSR_TR2_Pos)        /*!< 0x00000004 */
#define EXTI_FTSR_TR2                       EXTI_FTSR_TR2_Msk                  /*!< Falling trigger event configuration bit of line 2 */
#define EXTI_FTSR_TR3_Pos                   (3U)                               
#define EXTI_FTSR_TR3_Msk                   (0x1U << EXTI_FTSR_TR3_Pos)        /*!< 0x00000008 */
#define EXTI_FTSR_TR3                       EXTI_FTSR_TR3_Msk                  /*!< Falling trigger event configuration bit of line 3 */
#define EXTI_FTSR_TR4_Pos                   (4U)                               
#define EXTI_FTSR_TR4_Msk                   (0x1U << EXTI_FTSR_TR4_Pos)        /*!< 0x00000010 */
#define EXTI_FTSR_TR4                       EXTI_FTSR_TR4_Msk                  /*!< Falling trigger event configuration bit of line 4 */
#define EXTI_FTSR_TR5_Pos                   (5U)                               
#define EXTI_FTSR_TR5_Msk                   (0x1U << EXTI_FTSR_TR5_Pos)        /*!< 0x00000020 */
#define EXTI_FTSR_TR5                       EXTI_FTSR_TR5_Msk                  /*!< Falling trigger event configuration bit of line 5 */
#define EXTI_FTSR_TR6_Pos                   (6U)                               
#define EXTI_FTSR_TR6_Msk                   (0x1U << EXTI_FTSR_TR6_Pos)        /*!< 0x00000040 */
#define EXTI_FTSR_TR6                       EXTI_FTSR_TR6_Msk                  /*!< Falling trigger event configuration bit of line 6 */
#define EXTI_FTSR_TR7_Pos                   (7U)                               
#define EXTI_FTSR_TR7_Msk                   (0x1U << EXTI_FTSR_TR7_Pos)        /*!< 0x00000080 */
#define EXTI_FTSR_TR7                       EXTI_FTSR_TR7_Msk                  /*!< Falling trigger event configuration bit of line 7 */
#define EXTI_FTSR_TR8_Pos                   (8U)                               
#define EXTI_FTSR_TR8_Msk                   (0x1U << EXTI_FTSR_TR8_Pos)        /*!< 0x00000100 */
#define EXTI_FTSR_TR8                       EXTI_FTSR_TR8_Msk                  /*!< Falling trigger event configuration bit of line 8 */
#define EXTI_FTSR_TR9_Pos                   (9U)                               
#define EXTI_FTSR_TR9_Msk                   (0x1U << EXTI_FTSR_TR9_Pos)        /*!< 0x00000200 */
#define EXTI_FTSR_TR9                       EXTI_FTSR_TR9_Msk                  /*!< Falling trigger event configuration bit of line 9 */
#define EXTI_FTSR_TR10_Pos                  (10U)                              
#define EXTI_FTSR_TR10_Msk                  (0x1U << EXTI_FTSR_TR10_Pos)       /*!< 0x00000400 */
#define EXTI_FTSR_TR10                      EXTI_FTSR_TR10_Msk                 /*!< Falling trigger event configuration bit of line 10 */
#define EXTI_FTSR_TR11_Pos                  (11U)                              
#define EXTI_FTSR_TR11_Msk                  (0x1U << EXTI_FTSR_TR11_Pos)       /*!< 0x00000800 */
#define EXTI_FTSR_TR11                      EXTI_FTSR_TR11_Msk                 /*!< Falling trigger event configuration bit of line 11 */
#define EXTI_FTSR_TR12_Pos                  (12U)                              
#define EXTI_FTSR_TR12_Msk                  (0x1U << EXTI_FTSR_TR12_Pos)       /*!< 0x00001000 */
#define EXTI_FTSR_TR12                      EXTI_FTSR_TR12_Msk                 /*!< Falling trigger event configuration bit of line 12 */
#define EXTI_FTSR_TR13_Pos                  (13U)                              
#define EXTI_FTSR_TR13_Msk                  (0x1U << EXTI_FTSR_TR13_Pos)       /*!< 0x00002000 */
#define EXTI_FTSR_TR13                      EXTI_FTSR_TR13_Msk                 /*!< Falling trigger event configuration bit of line 13 */
#define EXTI_FTSR_TR14_Pos                  (14U)                              
#define EXTI_FTSR_TR14_Msk                  (0x1U << EXTI_FTSR_TR14_Pos)       /*!< 0x00004000 */
#define EXTI_FTSR_TR14                      EXTI_FTSR_TR14_Msk                 /*!< Falling trigger event configuration bit of line 14 */
#define EXTI_FTSR_TR15_Pos                  (15U)                              
#define EXTI_FTSR_TR15_Msk                  (0x1U << EXTI_FTSR_TR15_Pos)       /*!< 0x00008000 */
#define EXTI_FTSR_TR15                      EXTI_FTSR_TR15_Msk                 /*!< Falling trigger event configuration bit of line 15 */
#define EXTI_FTSR_TR16_Pos                  (16U)                              
#define EXTI_FTSR_TR16_Msk                  (0x1U << EXTI_FTSR_TR16_Pos)       /*!< 0x00010000 */
#define EXTI_FTSR_TR16                      EXTI_FTSR_TR16_Msk                 /*!< Falling trigger event configuration bit of line 16 */
#define EXTI_FTSR_TR17_Pos                  (17U)                              
#define EXTI_FTSR_TR17_Msk                  (0x1U << EXTI_FTSR_TR17_Pos)       /*!< 0x00020000 */
#define EXTI_FTSR_TR17                      EXTI_FTSR_TR17_Msk                 /*!< Falling trigger event configuration bit of line 17 */
#define EXTI_FTSR_TR18_Pos                  (18U)                              
#define EXTI_FTSR_TR18_Msk                  (0x1U << EXTI_FTSR_TR18_Pos)       /*!< 0x00040000 */
#define EXTI_FTSR_TR18                      EXTI_FTSR_TR18_Msk                 /*!< Falling trigger event configuration bit of line 18 */
#define EXTI_FTSR_TR19_Pos                  (19U)                              
#define EXTI_FTSR_TR19_Msk                  (0x1U << EXTI_FTSR_TR19_Pos)       /*!< 0x00080000 */
#define EXTI_FTSR_TR19                      EXTI_FTSR_TR19_Msk                 /*!< Falling trigger event configuration bit of line 19 */
#define EXTI_FTSR_TR20_Pos                  (20U)                              
#define EXTI_FTSR_TR20_Msk                  (0x1U << EXTI_FTSR_TR20_Pos)       /*!< 0x00100000 */
#define EXTI_FTSR_TR20                      EXTI_FTSR_TR20_Msk                 /*!< Falling trigger event configuration bit of line 20 */
#define EXTI_FTSR_TR21_Pos                  (21U)                              
#define EXTI_FTSR_TR21_Msk                  (0x1U << EXTI_FTSR_TR21_Pos)       /*!< 0x00200000 */
#define EXTI_FTSR_TR21                      EXTI_FTSR_TR21_Msk                 /*!< Falling trigger event configuration bit of line 21 */
#define EXTI_FTSR_TR22_Pos                  (22U)                              
#define EXTI_FTSR_TR22_Msk                  (0x1U << EXTI_FTSR_TR22_Pos)       /*!< 0x00400000 */
#define EXTI_FTSR_TR22                      EXTI_FTSR_TR22_Msk                 /*!< Falling trigger event configuration bit of line 22 */
#define EXTI_FTSR_TR23_Pos                  (23U)                              
#define EXTI_FTSR_TR23_Msk                  (0x1U << EXTI_FTSR_TR23_Pos)       /*!< 0x00800000 */
#define EXTI_FTSR_TR23                      EXTI_FTSR_TR23_Msk                 /*!< Falling trigger event configuration bit of line 23 */

/* References Defines */
#define  EXTI_FTSR_FT0 EXTI_FTSR_TR0
#define  EXTI_FTSR_FT1 EXTI_FTSR_TR1
#define  EXTI_FTSR_FT2 EXTI_FTSR_TR2
#define  EXTI_FTSR_FT3 EXTI_FTSR_TR3
#define  EXTI_FTSR_FT4 EXTI_FTSR_TR4
#define  EXTI_FTSR_FT5 EXTI_FTSR_TR5
#define  EXTI_FTSR_FT6 EXTI_FTSR_TR6
#define  EXTI_FTSR_FT7 EXTI_FTSR_TR7
#define  EXTI_FTSR_FT8 EXTI_FTSR_TR8
#define  EXTI_FTSR_FT9 EXTI_FTSR_TR9
#define  EXTI_FTSR_FT10 EXTI_FTSR_TR10
#define  EXTI_FTSR_FT11 EXTI_FTSR_TR11
#define  EXTI_FTSR_FT12 EXTI_FTSR_TR12
#define  EXTI_FTSR_FT13 EXTI_FTSR_TR13
#define  EXTI_FTSR_FT14 EXTI_FTSR_TR14
#define  EXTI_FTSR_FT15 EXTI_FTSR_TR15
#define  EXTI_FTSR_FT16 EXTI_FTSR_TR16
#define  EXTI_FTSR_FT17 EXTI_FTSR_TR17
#define  EXTI_FTSR_FT18 EXTI_FTSR_TR18
#define  EXTI_FTSR_FT19 EXTI_FTSR_TR19
#define  EXTI_FTSR_FT20 EXTI_FTSR_TR20
#define  EXTI_FTSR_FT21 EXTI_FTSR_TR21
#define  EXTI_FTSR_FT22 EXTI_FTSR_TR22
#define  EXTI_FTSR_FT23 EXTI_FTSR_TR23

/******************  Bit definition for EXTI_SWIER register  ******************/
#define EXTI_SWIER_SWIER0_Pos               (0U)                               
#define EXTI_SWIER_SWIER0_Msk               (0x1U << EXTI_SWIER_SWIER0_Pos)    /*!< 0x00000001 */
#define EXTI_SWIER_SWIER0                   EXTI_SWIER_SWIER0_Msk              /*!< Software Interrupt on line 0 */
#define EXTI_SWIER_SWIER1_Pos               (1U)                               
#define EXTI_SWIER_SWIER1_Msk               (0x1U << EXTI_SWIER_SWIER1_Pos)    /*!< 0x00000002 */
#define EXTI_SWIER_SWIER1                   EXTI_SWIER_SWIER1_Msk              /*!< Software Interrupt on line 1 */
#define EXTI_SWIER_SWIER2_Pos               (2U)                               
#define EXTI_SWIER_SWIER2_Msk               (0x1U << EXTI_SWIER_SWIER2_Pos)    /*!< 0x00000004 */
#define EXTI_SWIER_SWIER2                   EXTI_SWIER_SWIER2_Msk              /*!< Software Interrupt on line 2 */
#define EXTI_SWIER_SWIER3_Pos               (3U)                               
#define EXTI_SWIER_SWIER3_Msk               (0x1U << EXTI_SWIER_SWIER3_Pos)    /*!< 0x00000008 */
#define EXTI_SWIER_SWIER3                   EXTI_SWIER_SWIER3_Msk              /*!< Software Interrupt on line 3 */
#define EXTI_SWIER_SWIER4_Pos               (4U)                               
#define EXTI_SWIER_SWIER4_Msk               (0x1U << EXTI_SWIER_SWIER4_Pos)    /*!< 0x00000010 */
#define EXTI_SWIER_SWIER4                   EXTI_SWIER_SWIER4_Msk              /*!< Software Interrupt on line 4 */
#define EXTI_SWIER_SWIER5_Pos               (5U)                               
#define EXTI_SWIER_SWIER5_Msk               (0x1U << EXTI_SWIER_SWIER5_Pos)    /*!< 0x00000020 */
#define EXTI_SWIER_SWIER5                   EXTI_SWIER_SWIER5_Msk              /*!< Software Interrupt on line 5 */
#define EXTI_SWIER_SWIER6_Pos               (6U)                               
#define EXTI_SWIER_SWIER6_Msk               (0x1U << EXTI_SWIER_SWIER6_Pos)    /*!< 0x00000040 */
#define EXTI_SWIER_SWIER6                   EXTI_SWIER_SWIER6_Msk              /*!< Software Interrupt on line 6 */
#define EXTI_SWIER_SWIER7_Pos               (7U)                               
#define EXTI_SWIER_SWIER7_Msk               (0x1U << EXTI_SWIER_SWIER7_Pos)    /*!< 0x00000080 */
#define EXTI_SWIER_SWIER7                   EXTI_SWIER_SWIER7_Msk              /*!< Software Interrupt on line 7 */
#define EXTI_SWIER_SWIER8_Pos               (8U)                               
#define EXTI_SWIER_SWIER8_Msk               (0x1U << EXTI_SWIER_SWIER8_Pos)    /*!< 0x00000100 */
#define EXTI_SWIER_SWIER8                   EXTI_SWIER_SWIER8_Msk              /*!< Software Interrupt on line 8 */
#define EXTI_SWIER_SWIER9_Pos               (9U)                               
#define EXTI_SWIER_SWIER9_Msk               (0x1U << EXTI_SWIER_SWIER9_Pos)    /*!< 0x00000200 */
#define EXTI_SWIER_SWIER9                   EXTI_SWIER_SWIER9_Msk              /*!< Software Interrupt on line 9 */
#define EXTI_SWIER_SWIER10_Pos              (10U)                              
#define EXTI_SWIER_SWIER10_Msk              (0x1U << EXTI_SWIER_SWIER10_Pos)   /*!< 0x00000400 */
#define EXTI_SWIER_SWIER10                  EXTI_SWIER_SWIER10_Msk             /*!< Software Interrupt on line 10 */
#define EXTI_SWIER_SWIER11_Pos              (11U)                              
#define EXTI_SWIER_SWIER11_Msk              (0x1U << EXTI_SWIER_SWIER11_Pos)   /*!< 0x00000800 */
#define EXTI_SWIER_SWIER11                  EXTI_SWIER_SWIER11_Msk             /*!< Software Interrupt on line 11 */
#define EXTI_SWIER_SWIER12_Pos              (12U)                              
#define EXTI_SWIER_SWIER12_Msk              (0x1U << EXTI_SWIER_SWIER12_Pos)   /*!< 0x00001000 */
#define EXTI_SWIER_SWIER12                  EXTI_SWIER_SWIER12_Msk             /*!< Software Interrupt on line 12 */
#define EXTI_SWIER_SWIER13_Pos              (13U)                              
#define EXTI_SWIER_SWIER13_Msk              (0x1U << EXTI_SWIER_SWIER13_Pos)   /*!< 0x00002000 */
#define EXTI_SWIER_SWIER13                  EXTI_SWIER_SWIER13_Msk             /*!< Software Interrupt on line 13 */
#define EXTI_SWIER_SWIER14_Pos              (14U)                              
#define EXTI_SWIER_SWIER14_Msk              (0x1U << EXTI_SWIER_SWIER14_Pos)   /*!< 0x00004000 */
#define EXTI_SWIER_SWIER14                  EXTI_SWIER_SWIER14_Msk             /*!< Software Interrupt on line 14 */
#define EXTI_SWIER_SWIER15_Pos              (15U)                              
#define EXTI_SWIER_SWIER15_Msk              (0x1U << EXTI_SWIER_SWIER15_Pos)   /*!< 0x00008000 */
#define EXTI_SWIER_SWIER15                  EXTI_SWIER_SWIER15_Msk             /*!< Software Interrupt on line 15 */
#define EXTI_SWIER_SWIER16_Pos              (16U)                              
#define EXTI_SWIER_SWIER16_Msk              (0x1U << EXTI_SWIER_SWIER16_Pos)   /*!< 0x00010000 */
#define EXTI_SWIER_SWIER16                  EXTI_SWIER_SWIER16_Msk             /*!< Software Interrupt on line 16 */
#define EXTI_SWIER_SWIER17_Pos              (17U)                              
#define EXTI_SWIER_SWIER17_Msk              (0x1U << EXTI_SWIER_SWIER17_Pos)   /*!< 0x00020000 */
#define EXTI_SWIER_SWIER17                  EXTI_SWIER_SWIER17_Msk             /*!< Software Interrupt on line 17 */
#define EXTI_SWIER_SWIER18_Pos              (18U)                              
#define EXTI_SWIER_SWIER18_Msk              (0x1U << EXTI_SWIER_SWIER18_Pos)   /*!< 0x00040000 */
#define EXTI_SWIER_SWIER18                  EXTI_SWIER_SWIER18_Msk             /*!< Software Interrupt on line 18 */
#define EXTI_SWIER_SWIER19_Pos              (19U)                              
#define EXTI_SWIER_SWIER19_Msk              (0x1U << EXTI_SWIER_SWIER19_Pos)   /*!< 0x00080000 */
#define EXTI_SWIER_SWIER19                  EXTI_SWIER_SWIER19_Msk             /*!< Software Interrupt on line 19 */
#define EXTI_SWIER_SWIER20_Pos              (20U)                              
#define EXTI_SWIER_SWIER20_Msk              (0x1U << EXTI_SWIER_SWIER20_Pos)   /*!< 0x00100000 */
#define EXTI_SWIER_SWIER20                  EXTI_SWIER_SWIER20_Msk             /*!< Software Interrupt on line 20 */
#define EXTI_SWIER_SWIER21_Pos              (21U)                              
#define EXTI_SWIER_SWIER21_Msk              (0x1U << EXTI_SWIER_SWIER21_Pos)   /*!< 0x00200000 */
#define EXTI_SWIER_SWIER21                  EXTI_SWIER_SWIER21_Msk             /*!< Software Interrupt on line 21 */
#define EXTI_SWIER_SWIER22_Pos              (22U)                              
#define EXTI_SWIER_SWIER22_Msk              (0x1U << EXTI_SWIER_SWIER22_Pos)   /*!< 0x00400000 */
#define EXTI_SWIER_SWIER22                  EXTI_SWIER_SWIER22_Msk             /*!< Software Interrupt on line 22 */
#define EXTI_SWIER_SWIER23_Pos              (23U)                              
#define EXTI_SWIER_SWIER23_Msk              (0x1U << EXTI_SWIER_SWIER23_Pos)   /*!< 0x00800000 */
#define EXTI_SWIER_SWIER23                  EXTI_SWIER_SWIER23_Msk             /*!< Software Interrupt on line 23 */

/* References Defines */
#define  EXTI_SWIER_SWI0 EXTI_SWIER_SWIER0
#define  EXTI_SWIER_SWI1 EXTI_SWIER_SWIER1
#define  EXTI_SWIER_SWI2 EXTI_SWIER_SWIER2
#define  EXTI_SWIER_SWI3 EXTI_SWIER_SWIER3
#define  EXTI_SWIER_SWI4 EXTI_SWIER_SWIER4
#define  EXTI_SWIER_SWI5 EXTI_SWIER_SWIER5
#define  EXTI_SWIER_SWI6 EXTI_SWIER_SWIER6
#define  EXTI_SWIER_SWI7 EXTI_SWIER_SWIER7
#define  EXTI_SWIER_SWI8 EXTI_SWIER_SWIER8
#define  EXTI_SWIER_SWI9 EXTI_SWIER_SWIER9
#define  EXTI_SWIER_SWI10 EXTI_SWIER_SWIER10
#define  EXTI_SWIER_SWI11 EXTI_SWIER_SWIER11
#define  EXTI_SWIER_SWI12 EXTI_SWIER_SWIER12
#define  EXTI_SWIER_SWI13 EXTI_SWIER_SWIER13
#define  EXTI_SWIER_SWI14 EXTI_SWIER_SWIER14
#define  EXTI_SWIER_SWI15 EXTI_SWIER_SWIER15
#define  EXTI_SWIER_SWI16 EXTI_SWIER_SWIER16
#define  EXTI_SWIER_SWI17 EXTI_SWIER_SWIER17
#define  EXTI_SWIER_SWI18 EXTI_SWIER_SWIER18
#define  EXTI_SWIER_SWI19 EXTI_SWIER_SWIER19
#define  EXTI_SWIER_SWI20 EXTI_SWIER_SWIER20
#define  EXTI_SWIER_SWI21 EXTI_SWIER_SWIER21
#define  EXTI_SWIER_SWI22 EXTI_SWIER_SWIER22
#define  EXTI_SWIER_SWI23 EXTI_SWIER_SWIER23

/*******************  Bit definition for EXTI_PR register  ********************/
#define EXTI_PR_PR0_Pos                     (0U)                               
#define EXTI_PR_PR0_Msk                     (0x1U << EXTI_PR_PR0_Pos)          /*!< 0x00000001 */
#define EXTI_PR_PR0                         EXTI_PR_PR0_Msk                    /*!< Pending bit for line 0 */
#define EXTI_PR_PR1_Pos                     (1U)                               
#define EXTI_PR_PR1_Msk                     (0x1U << EXTI_PR_PR1_Pos)          /*!< 0x00000002 */
#define EXTI_PR_PR1                         EXTI_PR_PR1_Msk                    /*!< Pending bit for line 1 */
#define EXTI_PR_PR2_Pos                     (2U)                               
#define EXTI_PR_PR2_Msk                     (0x1U << EXTI_PR_PR2_Pos)          /*!< 0x00000004 */
#define EXTI_PR_PR2                         EXTI_PR_PR2_Msk                    /*!< Pending bit for line 2 */
#define EXTI_PR_PR3_Pos                     (3U)                               
#define EXTI_PR_PR3_Msk                     (0x1U << EXTI_PR_PR3_Pos)          /*!< 0x00000008 */
#define EXTI_PR_PR3                         EXTI_PR_PR3_Msk                    /*!< Pending bit for line 3 */
#define EXTI_PR_PR4_Pos                     (4U)                               
#define EXTI_PR_PR4_Msk                     (0x1U << EXTI_PR_PR4_Pos)          /*!< 0x00000010 */
#define EXTI_PR_PR4                         EXTI_PR_PR4_Msk                    /*!< Pending bit for line 4 */
#define EXTI_PR_PR5_Pos                     (5U)                               
#define EXTI_PR_PR5_Msk                     (0x1U << EXTI_PR_PR5_Pos)          /*!< 0x00000020 */
#define EXTI_PR_PR5                         EXTI_PR_PR5_Msk                    /*!< Pending bit for line 5 */
#define EXTI_PR_PR6_Pos                     (6U)                               
#define EXTI_PR_PR6_Msk                     (0x1U << EXTI_PR_PR6_Pos)          /*!< 0x00000040 */
#define EXTI_PR_PR6                         EXTI_PR_PR6_Msk                    /*!< Pending bit for line 6 */
#define EXTI_PR_PR7_Pos                     (7U)                               
#define EXTI_PR_PR7_Msk                     (0x1U << EXTI_PR_PR7_Pos)          /*!< 0x00000080 */
#define EXTI_PR_PR7                         EXTI_PR_PR7_Msk                    /*!< Pending bit for line 7 */
#define EXTI_PR_PR8_Pos                     (8U)                               
#define EXTI_PR_PR8_Msk                     (0x1U << EXTI_PR_PR8_Pos)          /*!< 0x00000100 */
#define EXTI_PR_PR8                         EXTI_PR_PR8_Msk                    /*!< Pending bit for line 8 */
#define EXTI_PR_PR9_Pos                     (9U)                               
#define EXTI_PR_PR9_Msk                     (0x1U << EXTI_PR_PR9_Pos)          /*!< 0x00000200 */
#define EXTI_PR_PR9                         EXTI_PR_PR9_Msk                    /*!< Pending bit for line 9 */
#define EXTI_PR_PR10_Pos                    (10U)                              
#define EXTI_PR_PR10_Msk                    (0x1U << EXTI_PR_PR10_Pos)         /*!< 0x00000400 */
#define EXTI_PR_PR10                        EXTI_PR_PR10_Msk                   /*!< Pending bit for line 10 */
#define EXTI_PR_PR11_Pos                    (11U)                              
#define EXTI_PR_PR11_Msk                    (0x1U << EXTI_PR_PR11_Pos)         /*!< 0x00000800 */
#define EXTI_PR_PR11                        EXTI_PR_PR11_Msk                   /*!< Pending bit for line 11 */
#define EXTI_PR_PR12_Pos                    (12U)                              
#define EXTI_PR_PR12_Msk                    (0x1U << EXTI_PR_PR12_Pos)         /*!< 0x00001000 */
#define EXTI_PR_PR12                        EXTI_PR_PR12_Msk                   /*!< Pending bit for line 12 */
#define EXTI_PR_PR13_Pos                    (13U)                              
#define EXTI_PR_PR13_Msk                    (0x1U << EXTI_PR_PR13_Pos)         /*!< 0x00002000 */
#define EXTI_PR_PR13                        EXTI_PR_PR13_Msk                   /*!< Pending bit for line 13 */
#define EXTI_PR_PR14_Pos                    (14U)                              
#define EXTI_PR_PR14_Msk                    (0x1U << EXTI_PR_PR14_Pos)         /*!< 0x00004000 */
#define EXTI_PR_PR14                        EXTI_PR_PR14_Msk                   /*!< Pending bit for line 14 */
#define EXTI_PR_PR15_Pos                    (15U)                              
#define EXTI_PR_PR15_Msk                    (0x1U << EXTI_PR_PR15_Pos)         /*!< 0x00008000 */
#define EXTI_PR_PR15                        EXTI_PR_PR15_Msk                   /*!< Pending bit for line 15 */
#define EXTI_PR_PR16_Pos                    (16U)                              
#define EXTI_PR_PR16_Msk                    (0x1U << EXTI_PR_PR16_Pos)         /*!< 0x00010000 */
#define EXTI_PR_PR16                        EXTI_PR_PR16_Msk                   /*!< Pending bit for line 16 */
#define EXTI_PR_PR17_Pos                    (17U)                              
#define EXTI_PR_PR17_Msk                    (0x1U << EXTI_PR_PR17_Pos)         /*!< 0x00020000 */
#define EXTI_PR_PR17                        EXTI_PR_PR17_Msk                   /*!< Pending bit for line 17 */
#define EXTI_PR_PR18_Pos                    (18U)                              
#define EXTI_PR_PR18_Msk                    (0x1U << EXTI_PR_PR18_Pos)         /*!< 0x00040000 */
#define EXTI_PR_PR18                        EXTI_PR_PR18_Msk                   /*!< Pending bit for line 18 */
#define EXTI_PR_PR19_Pos                    (19U)                              
#define EXTI_PR_PR19_Msk                    (0x1U << EXTI_PR_PR19_Pos)         /*!< 0x00080000 */
#define EXTI_PR_PR19                        EXTI_PR_PR19_Msk                   /*!< Pending bit for line 19 */
#define EXTI_PR_PR20_Pos                    (20U)                              
#define EXTI_PR_PR20_Msk                    (0x1U << EXTI_PR_PR20_Pos)         /*!< 0x00100000 */
#define EXTI_PR_PR20                        EXTI_PR_PR20_Msk                   /*!< Pending bit for line 20 */
#define EXTI_PR_PR21_Pos                    (21U)                              
#define EXTI_PR_PR21_Msk                    (0x1U << EXTI_PR_PR21_Pos)         /*!< 0x00200000 */
#define EXTI_PR_PR21                        EXTI_PR_PR21_Msk                   /*!< Pending bit for line 21 */
#define EXTI_PR_PR22_Pos                    (22U)                              
#define EXTI_PR_PR22_Msk                    (0x1U << EXTI_PR_PR22_Pos)         /*!< 0x00400000 */
#define EXTI_PR_PR22                        EXTI_PR_PR22_Msk                   /*!< Pending bit for line 22 */
#define EXTI_PR_PR23_Pos                    (23U)                              
#define EXTI_PR_PR23_Msk                    (0x1U << EXTI_PR_PR23_Pos)         /*!< 0x00800000 */
#define EXTI_PR_PR23                        EXTI_PR_PR23_Msk                   /*!< Pending bit for line 23 */

/* References Defines */
#define  EXTI_PR_PIF0 EXTI_PR_PR0
#define  EXTI_PR_PIF1 EXTI_PR_PR1
#define  EXTI_PR_PIF2 EXTI_PR_PR2
#define  EXTI_PR_PIF3 EXTI_PR_PR3
#define  EXTI_PR_PIF4 EXTI_PR_PR4
#define  EXTI_PR_PIF5 EXTI_PR_PR5
#define  EXTI_PR_PIF6 EXTI_PR_PR6
#define  EXTI_PR_PIF7 EXTI_PR_PR7
#define  EXTI_PR_PIF8 EXTI_PR_PR8
#define  EXTI_PR_PIF9 EXTI_PR_PR9
#define  EXTI_PR_PIF10 EXTI_PR_PR10
#define  EXTI_PR_PIF11 EXTI_PR_PR11
#define  EXTI_PR_PIF12 EXTI_PR_PR12
#define  EXTI_PR_PIF13 EXTI_PR_PR13
#define  EXTI_PR_PIF14 EXTI_PR_PR14
#define  EXTI_PR_PIF15 EXTI_PR_PR15
#define  EXTI_PR_PIF16 EXTI_PR_PR16
#define  EXTI_PR_PIF17 EXTI_PR_PR17
#define  EXTI_PR_PIF18 EXTI_PR_PR18
#define  EXTI_PR_PIF19 EXTI_PR_PR19
#define  EXTI_PR_PIF20 EXTI_PR_PR20
#define  EXTI_PR_PIF21 EXTI_PR_PR21
#define  EXTI_PR_PIF22 EXTI_PR_PR22
#define  EXTI_PR_PIF23 EXTI_PR_PR23

/******************************************************************************/
/*                                                                            */
/*                FLASH, DATA EEPROM and Option Bytes Registers               */
/*                        (FLASH, DATA_EEPROM, OB)                            */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for FLASH_ACR register  ******************/
#define FLASH_ACR_LATENCY_Pos                (0U)                              
#define FLASH_ACR_LATENCY_Msk                (0x1U << FLASH_ACR_LATENCY_Pos)   /*!< 0x00000001 */
#define FLASH_ACR_LATENCY                    FLASH_ACR_LATENCY_Msk             /*!< Latency */
#define FLASH_ACR_PRFTEN_Pos                 (1U)                              
#define FLASH_ACR_PRFTEN_Msk                 (0x1U << FLASH_ACR_PRFTEN_Pos)    /*!< 0x00000002 */
#define FLASH_ACR_PRFTEN                     FLASH_ACR_PRFTEN_Msk              /*!< Prefetch Buffer Enable */
#define FLASH_ACR_ACC64_Pos                  (2U)                              
#define FLASH_ACR_ACC64_Msk                  (0x1U << FLASH_ACR_ACC64_Pos)     /*!< 0x00000004 */
#define FLASH_ACR_ACC64                      FLASH_ACR_ACC64_Msk               /*!< Access 64 bits */
#define FLASH_ACR_SLEEP_PD_Pos               (3U)                              
#define FLASH_ACR_SLEEP_PD_Msk               (0x1U << FLASH_ACR_SLEEP_PD_Pos)  /*!< 0x00000008 */
#define FLASH_ACR_SLEEP_PD                   FLASH_ACR_SLEEP_PD_Msk            /*!< Flash mode during sleep mode */
#define FLASH_ACR_RUN_PD_Pos                 (4U)                              
#define FLASH_ACR_RUN_PD_Msk                 (0x1U << FLASH_ACR_RUN_PD_Pos)    /*!< 0x00000010 */
#define FLASH_ACR_RUN_PD                     FLASH_ACR_RUN_PD_Msk              /*!< Flash mode during RUN mode */

/*******************  Bit definition for FLASH_PECR register  ******************/
#define FLASH_PECR_PELOCK_Pos                (0U)                              
#define FLASH_PECR_PELOCK_Msk                (0x1U << FLASH_PECR_PELOCK_Pos)   /*!< 0x00000001 */
#define FLASH_PECR_PELOCK                    FLASH_PECR_PELOCK_Msk             /*!< FLASH_PECR and Flash data Lock */
#define FLASH_PECR_PRGLOCK_Pos               (1U)                              
#define FLASH_PECR_PRGLOCK_Msk               (0x1U << FLASH_PECR_PRGLOCK_Pos)  /*!< 0x00000002 */
#define FLASH_PECR_PRGLOCK                   FLASH_PECR_PRGLOCK_Msk            /*!< Program matrix Lock */
#define FLASH_PECR_OPTLOCK_Pos               (2U)                              
#define FLASH_PECR_OPTLOCK_Msk               (0x1U << FLASH_PECR_OPTLOCK_Pos)  /*!< 0x00000004 */
#define FLASH_PECR_OPTLOCK                   FLASH_PECR_OPTLOCK_Msk            /*!< Option byte matrix Lock */
#define FLASH_PECR_PROG_Pos                  (3U)                              
#define FLASH_PECR_PROG_Msk                  (0x1U << FLASH_PECR_PROG_Pos)     /*!< 0x00000008 */
#define FLASH_PECR_PROG                      FLASH_PECR_PROG_Msk               /*!< Program matrix selection */
#define FLASH_PECR_DATA_Pos                  (4U)                              
#define FLASH_PECR_DATA_Msk                  (0x1U << FLASH_PECR_DATA_Pos)     /*!< 0x00000010 */
#define FLASH_PECR_DATA                      FLASH_PECR_DATA_Msk               /*!< Data matrix selection */
#define FLASH_PECR_FTDW_Pos                  (8U)                              
#define FLASH_PECR_FTDW_Msk                  (0x1U << FLASH_PECR_FTDW_Pos)     /*!< 0x00000100 */
#define FLASH_PECR_FTDW                      FLASH_PECR_FTDW_Msk               /*!< Fixed Time Data write for Word/Half Word/Byte programming */
#define FLASH_PECR_ERASE_Pos                 (9U)                              
#define FLASH_PECR_ERASE_Msk                 (0x1U << FLASH_PECR_ERASE_Pos)    /*!< 0x00000200 */
#define FLASH_PECR_ERASE                     FLASH_PECR_ERASE_Msk              /*!< Page erasing mode */
#define FLASH_PECR_FPRG_Pos                  (10U)                             
#define FLASH_PECR_FPRG_Msk                  (0x1U << FLASH_PECR_FPRG_Pos)     /*!< 0x00000400 */
#define FLASH_PECR_FPRG                      FLASH_PECR_FPRG_Msk               /*!< Fast Page/Half Page programming mode */
#define FLASH_PECR_PARALLBANK_Pos            (15U)                             
#define FLASH_PECR_PARALLBANK_Msk            (0x1U << FLASH_PECR_PARALLBANK_Pos) /*!< 0x00008000 */
#define FLASH_PECR_PARALLBANK                FLASH_PECR_PARALLBANK_Msk         /*!< Parallel Bank mode */
#define FLASH_PECR_EOPIE_Pos                 (16U)                             
#define FLASH_PECR_EOPIE_Msk                 (0x1U << FLASH_PECR_EOPIE_Pos)    /*!< 0x00010000 */
#define FLASH_PECR_EOPIE                     FLASH_PECR_EOPIE_Msk              /*!< End of programming interrupt */ 
#define FLASH_PECR_ERRIE_Pos                 (17U)                             
#define FLASH_PECR_ERRIE_Msk                 (0x1U << FLASH_PECR_ERRIE_Pos)    /*!< 0x00020000 */
#define FLASH_PECR_ERRIE                     FLASH_PECR_ERRIE_Msk              /*!< Error interrupt */ 
#define FLASH_PECR_OBL_LAUNCH_Pos            (18U)                             
#define FLASH_PECR_OBL_LAUNCH_Msk            (0x1U << FLASH_PECR_OBL_LAUNCH_Pos) /*!< 0x00040000 */
#define FLASH_PECR_OBL_LAUNCH                FLASH_PECR_OBL_LAUNCH_Msk         /*!< Launch the option byte loading */ 

/******************  Bit definition for FLASH_PDKEYR register  ******************/
#define FLASH_PDKEYR_PDKEYR_Pos              (0U)                              
#define FLASH_PDKEYR_PDKEYR_Msk              (0xFFFFFFFFU << FLASH_PDKEYR_PDKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_PDKEYR_PDKEYR                  FLASH_PDKEYR_PDKEYR_Msk           /*!< FLASH_PEC and data matrix Key */

/******************  Bit definition for FLASH_PEKEYR register  ******************/
#define FLASH_PEKEYR_PEKEYR_Pos              (0U)                              
#define FLASH_PEKEYR_PEKEYR_Msk              (0xFFFFFFFFU << FLASH_PEKEYR_PEKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_PEKEYR_PEKEYR                  FLASH_PEKEYR_PEKEYR_Msk           /*!< FLASH_PEC and data matrix Key */

/******************  Bit definition for FLASH_PRGKEYR register  ******************/
#define FLASH_PRGKEYR_PRGKEYR_Pos            (0U)                              
#define FLASH_PRGKEYR_PRGKEYR_Msk            (0xFFFFFFFFU << FLASH_PRGKEYR_PRGKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_PRGKEYR_PRGKEYR                FLASH_PRGKEYR_PRGKEYR_Msk         /*!< Program matrix Key */

/******************  Bit definition for FLASH_OPTKEYR register  ******************/
#define FLASH_OPTKEYR_OPTKEYR_Pos            (0U)                              
#define FLASH_OPTKEYR_OPTKEYR_Msk            (0xFFFFFFFFU << FLASH_OPTKEYR_OPTKEYR_Pos) /*!< 0xFFFFFFFF */
#define FLASH_OPTKEYR_OPTKEYR                FLASH_OPTKEYR_OPTKEYR_Msk         /*!< Option bytes matrix Key */

/******************  Bit definition for FLASH_SR register  *******************/
#define FLASH_SR_BSY_Pos                     (0U)                              
#define FLASH_SR_BSY_Msk                     (0x1U << FLASH_SR_BSY_Pos)        /*!< 0x00000001 */
#define FLASH_SR_BSY                         FLASH_SR_BSY_Msk                  /*!< Busy */
#define FLASH_SR_EOP_Pos                     (1U)                              
#define FLASH_SR_EOP_Msk                     (0x1U << FLASH_SR_EOP_Pos)        /*!< 0x00000002 */
#define FLASH_SR_EOP                         FLASH_SR_EOP_Msk                  /*!< End Of Programming*/
#define FLASH_SR_ENDHV_Pos                   (2U)                              
#define FLASH_SR_ENDHV_Msk                   (0x1U << FLASH_SR_ENDHV_Pos)      /*!< 0x00000004 */
#define FLASH_SR_ENDHV                       FLASH_SR_ENDHV_Msk                /*!< End of high voltage */
#define FLASH_SR_READY_Pos                   (3U)                              
#define FLASH_SR_READY_Msk                   (0x1U << FLASH_SR_READY_Pos)      /*!< 0x00000008 */
#define FLASH_SR_READY                       FLASH_SR_READY_Msk                /*!< Flash ready after low power mode */

#define FLASH_SR_WRPERR_Pos                  (8U)                              
#define FLASH_SR_WRPERR_Msk                  (0x1U << FLASH_SR_WRPERR_Pos)     /*!< 0x00000100 */
#define FLASH_SR_WRPERR                      FLASH_SR_WRPERR_Msk               /*!< Write protected error */
#define FLASH_SR_PGAERR_Pos                  (9U)                              
#define FLASH_SR_PGAERR_Msk                  (0x1U << FLASH_SR_PGAERR_Pos)     /*!< 0x00000200 */
#define FLASH_SR_PGAERR                      FLASH_SR_PGAERR_Msk               /*!< Programming Alignment Error */
#define FLASH_SR_SIZERR_Pos                  (10U)                             
#define FLASH_SR_SIZERR_Msk                  (0x1U << FLASH_SR_SIZERR_Pos)     /*!< 0x00000400 */
#define FLASH_SR_SIZERR                      FLASH_SR_SIZERR_Msk               /*!< Size error */
#define FLASH_SR_OPTVERR_Pos                 (11U)                             
#define FLASH_SR_OPTVERR_Msk                 (0x1U << FLASH_SR_OPTVERR_Pos)    /*!< 0x00000800 */
#define FLASH_SR_OPTVERR                     FLASH_SR_OPTVERR_Msk              /*!< Option validity error */
#define FLASH_SR_OPTVERRUSR_Pos              (12U)                             
#define FLASH_SR_OPTVERRUSR_Msk              (0x1U << FLASH_SR_OPTVERRUSR_Pos) /*!< 0x00001000 */
#define FLASH_SR_OPTVERRUSR                  FLASH_SR_OPTVERRUSR_Msk           /*!< Option User validity error */

/******************  Bit definition for FLASH_OBR register  *******************/
#define FLASH_OBR_RDPRT_Pos                  (0U)                              
#define FLASH_OBR_RDPRT_Msk                  (0xFFU << FLASH_OBR_RDPRT_Pos)    /*!< 0x000000FF */
#define FLASH_OBR_RDPRT                      FLASH_OBR_RDPRT_Msk               /*!< Read Protection */
#define FLASH_OBR_BOR_LEV_Pos                (16U)                             
#define FLASH_OBR_BOR_LEV_Msk                (0xFU << FLASH_OBR_BOR_LEV_Pos)   /*!< 0x000F0000 */
#define FLASH_OBR_BOR_LEV                    FLASH_OBR_BOR_LEV_Msk             /*!< BOR_LEV[3:0] Brown Out Reset Threshold Level*/
#define FLASH_OBR_USER_Pos                   (20U)                             
#define FLASH_OBR_USER_Msk                   (0xFU << FLASH_OBR_USER_Pos)      /*!< 0x00F00000 */
#define FLASH_OBR_USER                       FLASH_OBR_USER_Msk                /*!< User Option Bytes */
#define FLASH_OBR_IWDG_SW_Pos                (20U)                             
#define FLASH_OBR_IWDG_SW_Msk                (0x1U << FLASH_OBR_IWDG_SW_Pos)   /*!< 0x00100000 */
#define FLASH_OBR_IWDG_SW                    FLASH_OBR_IWDG_SW_Msk             /*!< IWDG_SW */
#define FLASH_OBR_nRST_STOP_Pos              (21U)                             
#define FLASH_OBR_nRST_STOP_Msk              (0x1U << FLASH_OBR_nRST_STOP_Pos) /*!< 0x00200000 */
#define FLASH_OBR_nRST_STOP                  FLASH_OBR_nRST_STOP_Msk           /*!< nRST_STOP */
#define FLASH_OBR_nRST_STDBY_Pos             (22U)                             
#define FLASH_OBR_nRST_STDBY_Msk             (0x1U << FLASH_OBR_nRST_STDBY_Pos) /*!< 0x00400000 */
#define FLASH_OBR_nRST_STDBY                 FLASH_OBR_nRST_STDBY_Msk          /*!< nRST_STDBY */
#define FLASH_OBR_nRST_BFB2_Pos              (23U)                             
#define FLASH_OBR_nRST_BFB2_Msk              (0x1U << FLASH_OBR_nRST_BFB2_Pos) /*!< 0x00800000 */
#define FLASH_OBR_nRST_BFB2                  FLASH_OBR_nRST_BFB2_Msk           /*!< BFB2 */

/******************  Bit definition for FLASH_WRPR register  ******************/
#define FLASH_WRPR1_WRP_Pos                  (0U)                              
#define FLASH_WRPR1_WRP_Msk                  (0xFFFFFFFFU << FLASH_WRPR1_WRP_Pos) /*!< 0xFFFFFFFF */
#define FLASH_WRPR1_WRP                      FLASH_WRPR1_WRP_Msk               /*!< Write Protect sectors 0  to 31  */
#define FLASH_WRPR2_WRP_Pos                  (0U)                              
#define FLASH_WRPR2_WRP_Msk                  (0xFFFFFFFFU << FLASH_WRPR2_WRP_Pos) /*!< 0xFFFFFFFF */
#define FLASH_WRPR2_WRP                      FLASH_WRPR2_WRP_Msk               /*!< Write Protect sectors 32 to 63  */             
#define FLASH_WRPR3_WRP_Pos                  (0U)                              
#define FLASH_WRPR3_WRP_Msk                  (0xFFFFFFFFU << FLASH_WRPR3_WRP_Pos) /*!< 0xFFFFFFFF */
#define FLASH_WRPR3_WRP                      FLASH_WRPR3_WRP_Msk               /*!< Write Protect sectors 64 to 95  */ 

/******************************************************************************/
/*                                                                            */
/*                       Flexible Static Memory Controller                    */
/*                                                                            */
/******************************************************************************/
/******************  Bit definition for FSMC_BCRx register (x=1..4) *******************/
#define FSMC_BCRx_MBKEN_Pos                 (0U)                               
#define FSMC_BCRx_MBKEN_Msk                 (0x1U << FSMC_BCRx_MBKEN_Pos)      /*!< 0x00000001 */
#define FSMC_BCRx_MBKEN                     FSMC_BCRx_MBKEN_Msk                /*!< Memory bank enable bit */
#define FSMC_BCRx_MUXEN_Pos                 (1U)                               
#define FSMC_BCRx_MUXEN_Msk                 (0x1U << FSMC_BCRx_MUXEN_Pos)      /*!< 0x00000002 */
#define FSMC_BCRx_MUXEN                     FSMC_BCRx_MUXEN_Msk                /*!< Address/data multiplexing enable bit */

#define FSMC_BCRx_MTYP_Pos                  (2U)                               
#define FSMC_BCRx_MTYP_Msk                  (0x3U << FSMC_BCRx_MTYP_Pos)       /*!< 0x0000000C */
#define FSMC_BCRx_MTYP                      FSMC_BCRx_MTYP_Msk                 /*!< MTYP[1:0] bits (Memory type) */
#define FSMC_BCRx_MTYP_0                    (0x1U << FSMC_BCRx_MTYP_Pos)       /*!< 0x00000004 */
#define FSMC_BCRx_MTYP_1                    (0x2U << FSMC_BCRx_MTYP_Pos)       /*!< 0x00000008 */

#define FSMC_BCRx_MWID_Pos                  (4U)                               
#define FSMC_BCRx_MWID_Msk                  (0x3U << FSMC_BCRx_MWID_Pos)       /*!< 0x00000030 */
#define FSMC_BCRx_MWID                      FSMC_BCRx_MWID_Msk                 /*!< MWID[1:0] bits (Memory data bus width) */
#define FSMC_BCRx_MWID_0                    (0x1U << FSMC_BCRx_MWID_Pos)       /*!< 0x00000010 */
#define FSMC_BCRx_MWID_1                    (0x2U << FSMC_BCRx_MWID_Pos)       /*!< 0x00000020 */

#define FSMC_BCRx_FACCEN_Pos                (6U)                               
#define FSMC_BCRx_FACCEN_Msk                (0x1U << FSMC_BCRx_FACCEN_Pos)     /*!< 0x00000040 */
#define FSMC_BCRx_FACCEN                    FSMC_BCRx_FACCEN_Msk               /*!< Flash access enable */
#define FSMC_BCRx_BURSTEN_Pos               (8U)                               
#define FSMC_BCRx_BURSTEN_Msk               (0x1U << FSMC_BCRx_BURSTEN_Pos)    /*!< 0x00000100 */
#define FSMC_BCRx_BURSTEN                   FSMC_BCRx_BURSTEN_Msk              /*!< Burst enable bit */
#define FSMC_BCRx_WAITPOL_Pos               (9U)                               
#define FSMC_BCRx_WAITPOL_Msk               (0x1U << FSMC_BCRx_WAITPOL_Pos)    /*!< 0x00000200 */
#define FSMC_BCRx_WAITPOL                   FSMC_BCRx_WAITPOL_Msk              /*!< Wait signal polarity bit */
#define FSMC_BCRx_WRAPMOD_Pos               (10U)                              
#define FSMC_BCRx_WRAPMOD_Msk               (0x1U << FSMC_BCRx_WRAPMOD_Pos)    /*!< 0x00000400 */
#define FSMC_BCRx_WRAPMOD                   FSMC_BCRx_WRAPMOD_Msk              /*!< Wrapped burst mode support */
#define FSMC_BCRx_WAITCFG_Pos               (11U)                              
#define FSMC_BCRx_WAITCFG_Msk               (0x1U << FSMC_BCRx_WAITCFG_Pos)    /*!< 0x00000800 */
#define FSMC_BCRx_WAITCFG                   FSMC_BCRx_WAITCFG_Msk              /*!< Wait timing configuration */
#define FSMC_BCRx_WREN_Pos                  (12U)                              
#define FSMC_BCRx_WREN_Msk                  (0x1U << FSMC_BCRx_WREN_Pos)       /*!< 0x00001000 */
#define FSMC_BCRx_WREN                      FSMC_BCRx_WREN_Msk                 /*!< Write enable bit */
#define FSMC_BCRx_WAITEN_Pos                (13U)                              
#define FSMC_BCRx_WAITEN_Msk                (0x1U << FSMC_BCRx_WAITEN_Pos)     /*!< 0x00002000 */
#define FSMC_BCRx_WAITEN                    FSMC_BCRx_WAITEN_Msk               /*!< Wait enable bit */
#define FSMC_BCRx_EXTMOD_Pos                (14U)                              
#define FSMC_BCRx_EXTMOD_Msk                (0x1U << FSMC_BCRx_EXTMOD_Pos)     /*!< 0x00004000 */
#define FSMC_BCRx_EXTMOD                    FSMC_BCRx_EXTMOD_Msk               /*!< Extended mode enable */
#define FSMC_BCRx_ASYNCWAIT_Pos             (15U)                              
#define FSMC_BCRx_ASYNCWAIT_Msk             (0x1U << FSMC_BCRx_ASYNCWAIT_Pos)  /*!< 0x00008000 */
#define FSMC_BCRx_ASYNCWAIT                 FSMC_BCRx_ASYNCWAIT_Msk            /*!< Asynchronous wait */
#define FSMC_BCRx_CBURSTRW_Pos              (19U)                              
#define FSMC_BCRx_CBURSTRW_Msk              (0x1U << FSMC_BCRx_CBURSTRW_Pos)   /*!< 0x00080000 */
#define FSMC_BCRx_CBURSTRW                  FSMC_BCRx_CBURSTRW_Msk             /*!< Write burst enable */

/******************  Bit definition for FSMC_BTRx register (x=1..4)  ******************/
#define FSMC_BTRx_ADDSET_Pos                (0U)                               
#define FSMC_BTRx_ADDSET_Msk                (0xFU << FSMC_BTRx_ADDSET_Pos)     /*!< 0x0000000F */
#define FSMC_BTRx_ADDSET                    FSMC_BTRx_ADDSET_Msk               /*!< ADDSET[3:0] bits (Address setup phase duration) */
#define FSMC_BTRx_ADDSET_0                  (0x1U << FSMC_BTRx_ADDSET_Pos)     /*!< 0x00000001 */
#define FSMC_BTRx_ADDSET_1                  (0x2U << FSMC_BTRx_ADDSET_Pos)     /*!< 0x00000002 */
#define FSMC_BTRx_ADDSET_2                  (0x4U << FSMC_BTRx_ADDSET_Pos)     /*!< 0x00000004 */
#define FSMC_BTRx_ADDSET_3                  (0x8U << FSMC_BTRx_ADDSET_Pos)     /*!< 0x00000008 */

#define FSMC_BTRx_ADDHLD_Pos                (4U)                               
#define FSMC_BTRx_ADDHLD_Msk                (0xFU << FSMC_BTRx_ADDHLD_Pos)     /*!< 0x000000F0 */
#define FSMC_BTRx_ADDHLD                    FSMC_BTRx_ADDHLD_Msk               /*!< ADDHLD[3:0] bits (Address-hold phase duration) */
#define FSMC_BTRx_ADDHLD_0                  (0x1U << FSMC_BTRx_ADDHLD_Pos)     /*!< 0x00000010 */
#define FSMC_BTRx_ADDHLD_1                  (0x2U << FSMC_BTRx_ADDHLD_Pos)     /*!< 0x00000020 */
#define FSMC_BTRx_ADDHLD_2                  (0x4U << FSMC_BTRx_ADDHLD_Pos)     /*!< 0x00000040 */
#define FSMC_BTRx_ADDHLD_3                  (0x8U << FSMC_BTRx_ADDHLD_Pos)     /*!< 0x00000080 */

#define FSMC_BTRx_DATAST_Pos                (8U)                               
#define FSMC_BTRx_DATAST_Msk                (0xFFU << FSMC_BTRx_DATAST_Pos)    /*!< 0x0000FF00 */
#define FSMC_BTRx_DATAST                    FSMC_BTRx_DATAST_Msk               /*!< DATAST [7:0] bits (Data-phase duration) */
#define FSMC_BTRx_DATAST_0                  (0x01U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00000100 */
#define FSMC_BTRx_DATAST_1                  (0x02U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00000200 */
#define FSMC_BTRx_DATAST_2                  (0x04U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00000400 */
#define FSMC_BTRx_DATAST_3                  (0x08U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00000800 */
#define FSMC_BTRx_DATAST_4                  (0x10U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00001000 */
#define FSMC_BTRx_DATAST_5                  (0x20U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00002000 */
#define FSMC_BTRx_DATAST_6                  (0x40U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00004000 */
#define FSMC_BTRx_DATAST_7                  (0x80U << FSMC_BTRx_DATAST_Pos)    /*!< 0x00008000 */

#define FSMC_BTRx_BUSTURN_Pos               (16U)                              
#define FSMC_BTRx_BUSTURN_Msk               (0xFU << FSMC_BTRx_BUSTURN_Pos)    /*!< 0x000F0000 */
#define FSMC_BTRx_BUSTURN                   FSMC_BTRx_BUSTURN_Msk              /*!< BUSTURN[3:0] bits (Bus turnaround phase duration) */
#define FSMC_BTRx_BUSTURN_0                 (0x1U << FSMC_BTRx_BUSTURN_Pos)    /*!< 0x00010000 */
#define FSMC_BTRx_BUSTURN_1                 (0x2U << FSMC_BTRx_BUSTURN_Pos)    /*!< 0x00020000 */
#define FSMC_BTRx_BUSTURN_2                 (0x4U << FSMC_BTRx_BUSTURN_Pos)    /*!< 0x00040000 */
#define FSMC_BTRx_BUSTURN_3                 (0x8U << FSMC_BTRx_BUSTURN_Pos)    /*!< 0x00080000 */

#define FSMC_BTRx_CLKDIV_Pos                (20U)                              
#define FSMC_BTRx_CLKDIV_Msk                (0xFU << FSMC_BTRx_CLKDIV_Pos)     /*!< 0x00F00000 */
#define FSMC_BTRx_CLKDIV                    FSMC_BTRx_CLKDIV_Msk               /*!< CLKDIV[3:0] bits (Clock divide ratio) */
#define FSMC_BTRx_CLKDIV_0                  (0x1U << FSMC_BTRx_CLKDIV_Pos)     /*!< 0x00100000 */
#define FSMC_BTRx_CLKDIV_1                  (0x2U << FSMC_BTRx_CLKDIV_Pos)     /*!< 0x00200000 */
#define FSMC_BTRx_CLKDIV_2                  (0x4U << FSMC_BTRx_CLKDIV_Pos)     /*!< 0x00400000 */
#define FSMC_BTRx_CLKDIV_3                  (0x8U << FSMC_BTRx_CLKDIV_Pos)     /*!< 0x00800000 */

#define FSMC_BTRx_DATLAT_Pos                (24U)                              
#define FSMC_BTRx_DATLAT_Msk                (0xFU << FSMC_BTRx_DATLAT_Pos)     /*!< 0x0F000000 */
#define FSMC_BTRx_DATLAT                    FSMC_BTRx_DATLAT_Msk               /*!< DATLA[3:0] bits (Data latency) */
#define FSMC_BTRx_DATLAT_0                  (0x1U << FSMC_BTRx_DATLAT_Pos)     /*!< 0x01000000 */
#define FSMC_BTRx_DATLAT_1                  (0x2U << FSMC_BTRx_DATLAT_Pos)     /*!< 0x02000000 */
#define FSMC_BTRx_DATLAT_2                  (0x4U << FSMC_BTRx_DATLAT_Pos)     /*!< 0x04000000 */
#define FSMC_BTRx_DATLAT_3                  (0x8U << FSMC_BTRx_DATLAT_Pos)     /*!< 0x08000000 */

#define FSMC_BTRx_ACCMOD_Pos                (28U)                              
#define FSMC_BTRx_ACCMOD_Msk                (0x3U << FSMC_BTRx_ACCMOD_Pos)     /*!< 0x30000000 */
#define FSMC_BTRx_ACCMOD                    FSMC_BTRx_ACCMOD_Msk               /*!< ACCMOD[1:0] bits (Access mode) */
#define FSMC_BTRx_ACCMOD_0                  (0x1U << FSMC_BTRx_ACCMOD_Pos)     /*!< 0x10000000 */
#define FSMC_BTRx_ACCMOD_1                  (0x2U << FSMC_BTRx_ACCMOD_Pos)     /*!< 0x20000000 */

/******************  Bit definition for FSMC_BWTRx register (x=1..4) ******************/
#define FSMC_BWTRx_ADDSET_Pos               (0U)                               
#define FSMC_BWTRx_ADDSET_Msk               (0xFU << FSMC_BWTRx_ADDSET_Pos)    /*!< 0x0000000F */
#define FSMC_BWTRx_ADDSET                   FSMC_BWTRx_ADDSET_Msk              /*!< ADDSET[3:0] bits (Address setup phase duration) */
#define FSMC_BWTRx_ADDSET_0                 (0x1U << FSMC_BWTRx_ADDSET_Pos)    /*!< 0x00000001 */
#define FSMC_BWTRx_ADDSET_1                 (0x2U << FSMC_BWTRx_ADDSET_Pos)    /*!< 0x00000002 */
#define FSMC_BWTRx_ADDSET_2                 (0x4U << FSMC_BWTRx_ADDSET_Pos)    /*!< 0x00000004 */
#define FSMC_BWTRx_ADDSET_3                 (0x8U << FSMC_BWTRx_ADDSET_Pos)    /*!< 0x00000008 */

#define FSMC_BWTRx_ADDHLD_Pos               (4U)                               
#define FSMC_BWTRx_ADDHLD_Msk               (0xFU << FSMC_BWTRx_ADDHLD_Pos)    /*!< 0x000000F0 */
#define FSMC_BWTRx_ADDHLD                   FSMC_BWTRx_ADDHLD_Msk              /*!< ADDHLD[3:0] bits (Address-hold phase duration) */
#define FSMC_BWTRx_ADDHLD_0                 (0x1U << FSMC_BWTRx_ADDHLD_Pos)    /*!< 0x00000010 */
#define FSMC_BWTRx_ADDHLD_1                 (0x2U << FSMC_BWTRx_ADDHLD_Pos)    /*!< 0x00000020 */
#define FSMC_BWTRx_ADDHLD_2                 (0x4U << FSMC_BWTRx_ADDHLD_Pos)    /*!< 0x00000040 */
#define FSMC_BWTRx_ADDHLD_3                 (0x8U << FSMC_BWTRx_ADDHLD_Pos)    /*!< 0x00000080 */

#define FSMC_BWTRx_DATAST_Pos               (8U)                               
#define FSMC_BWTRx_DATAST_Msk               (0xFFU << FSMC_BWTRx_DATAST_Pos)   /*!< 0x0000FF00 */
#define FSMC_BWTRx_DATAST                   FSMC_BWTRx_DATAST_Msk              /*!< DATAST [7:0] bits (Data-phase duration) */
#define FSMC_BWTRx_DATAST_0                 (0x01U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00000100 */
#define FSMC_BWTRx_DATAST_1                 (0x02U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00000200 */
#define FSMC_BWTRx_DATAST_2                 (0x04U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00000400 */
#define FSMC_BWTRx_DATAST_3                 (0x08U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00000800 */
#define FSMC_BWTRx_DATAST_4                 (0x10U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00001000 */
#define FSMC_BWTRx_DATAST_5                 (0x20U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00002000 */
#define FSMC_BWTRx_DATAST_6                 (0x40U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00004000 */
#define FSMC_BWTRx_DATAST_7                 (0x80U << FSMC_BWTRx_DATAST_Pos)   /*!< 0x00008000 */

#define FSMC_BWTRx_BUSTURN_Pos              (16U)                              
#define FSMC_BWTRx_BUSTURN_Msk              (0xFU << FSMC_BWTRx_BUSTURN_Pos)   /*!< 0x000F0000 */
#define FSMC_BWTRx_BUSTURN                  FSMC_BWTRx_BUSTURN_Msk             /*!< BUSTURN[3:0] bits (Bus turnaround phase duration) */
#define FSMC_BWTRx_BUSTURN_0                (0x1U << FSMC_BWTRx_BUSTURN_Pos)   /*!< 0x00010000 */
#define FSMC_BWTRx_BUSTURN_1                (0x2U << FSMC_BWTRx_BUSTURN_Pos)   /*!< 0x00020000 */
#define FSMC_BWTRx_BUSTURN_2                (0x4U << FSMC_BWTRx_BUSTURN_Pos)   /*!< 0x00040000 */
#define FSMC_BWTRx_BUSTURN_3                (0x8U << FSMC_BWTRx_BUSTURN_Pos)   /*!< 0x00080000 */

#define FSMC_BWTRx_ACCMOD_Pos               (28U)                              
#define FSMC_BWTRx_ACCMOD_Msk               (0x3U << FSMC_BWTRx_ACCMOD_Pos)    /*!< 0x30000000 */
#define FSMC_BWTRx_ACCMOD                   FSMC_BWTRx_ACCMOD_Msk              /*!< ACCMOD[1:0] bits (Access mode) */
#define FSMC_BWTRx_ACCMOD_0                 (0x1U << FSMC_BWTRx_ACCMOD_Pos)    /*!< 0x10000000 */
#define FSMC_BWTRx_ACCMOD_1                 (0x2U << FSMC_BWTRx_ACCMOD_Pos)    /*!< 0x20000000 */

/******************************************************************************/
/*                                                                            */
/*                            General Purpose I/O                             */
/*                                                                            */
/******************************************************************************/
/******************  Bits definition for GPIO_MODER register  *****************/
#define GPIO_MODER_MODER0_Pos                (0U)                              
#define GPIO_MODER_MODER0_Msk                (0x3U << GPIO_MODER_MODER0_Pos)   /*!< 0x00000003 */
#define GPIO_MODER_MODER0                    GPIO_MODER_MODER0_Msk             
#define GPIO_MODER_MODER0_0                  (0x1U << GPIO_MODER_MODER0_Pos)   /*!< 0x00000001 */
#define GPIO_MODER_MODER0_1                  (0x2U << GPIO_MODER_MODER0_Pos)   /*!< 0x00000002 */

#define GPIO_MODER_MODER1_Pos                (2U)                              
#define GPIO_MODER_MODER1_Msk                (0x3U << GPIO_MODER_MODER1_Pos)   /*!< 0x0000000C */
#define GPIO_MODER_MODER1                    GPIO_MODER_MODER1_Msk             
#define GPIO_MODER_MODER1_0                  (0x1U << GPIO_MODER_MODER1_Pos)   /*!< 0x00000004 */
#define GPIO_MODER_MODER1_1                  (0x2U << GPIO_MODER_MODER1_Pos)   /*!< 0x00000008 */

#define GPIO_MODER_MODER2_Pos                (4U)                              
#define GPIO_MODER_MODER2_Msk                (0x3U << GPIO_MODER_MODER2_Pos)   /*!< 0x00000030 */
#define GPIO_MODER_MODER2                    GPIO_MODER_MODER2_Msk             
#define GPIO_MODER_MODER2_0                  (0x1U << GPIO_MODER_MODER2_Pos)   /*!< 0x00000010 */
#define GPIO_MODER_MODER2_1                  (0x2U << GPIO_MODER_MODER2_Pos)   /*!< 0x00000020 */

#define GPIO_MODER_MODER3_Pos                (6U)                              
#define GPIO_MODER_MODER3_Msk                (0x3U << GPIO_MODER_MODER3_Pos)   /*!< 0x000000C0 */
#define GPIO_MODER_MODER3                    GPIO_MODER_MODER3_Msk             
#define GPIO_MODER_MODER3_0                  (0x1U << GPIO_MODER_MODER3_Pos)   /*!< 0x00000040 */
#define GPIO_MODER_MODER3_1                  (0x2U << GPIO_MODER_MODER3_Pos)   /*!< 0x00000080 */

#define GPIO_MODER_MODER4_Pos                (8U)                              
#define GPIO_MODER_MODER4_Msk                (0x3U << GPIO_MODER_MODER4_Pos)   /*!< 0x00000300 */
#define GPIO_MODER_MODER4                    GPIO_MODER_MODER4_Msk             
#define GPIO_MODER_MODER4_0                  (0x1U << GPIO_MODER_MODER4_Pos)   /*!< 0x00000100 */
#define GPIO_MODER_MODER4_1                  (0x2U << GPIO_MODER_MODER4_Pos)   /*!< 0x00000200 */

#define GPIO_MODER_MODER5_Pos                (10U)                             
#define GPIO_MODER_MODER5_Msk                (0x3U << GPIO_MODER_MODER5_Pos)   /*!< 0x00000C00 */
#define GPIO_MODER_MODER5                    GPIO_MODER_MODER5_Msk             
#define GPIO_MODER_MODER5_0                  (0x1U << GPIO_MODER_MODER5_Pos)   /*!< 0x00000400 */
#define GPIO_MODER_MODER5_1                  (0x2U << GPIO_MODER_MODER5_Pos)   /*!< 0x00000800 */

#define GPIO_MODER_MODER6_Pos                (12U)                             
#define GPIO_MODER_MODER6_Msk                (0x3U << GPIO_MODER_MODER6_Pos)   /*!< 0x00003000 */
#define GPIO_MODER_MODER6                    GPIO_MODER_MODER6_Msk             
#define GPIO_MODER_MODER6_0                  (0x1U << GPIO_MODER_MODER6_Pos)   /*!< 0x00001000 */
#define GPIO_MODER_MODER6_1                  (0x2U << GPIO_MODER_MODER6_Pos)   /*!< 0x00002000 */

#define GPIO_MODER_MODER7_Pos                (14U)                             
#define GPIO_MODER_MODER7_Msk                (0x3U << GPIO_MODER_MODER7_Pos)   /*!< 0x0000C000 */
#define GPIO_MODER_MODER7                    GPIO_MODER_MODER7_Msk             
#define GPIO_MODER_MODER7_0                  (0x1U << GPIO_MODER_MODER7_Pos)   /*!< 0x00004000 */
#define GPIO_MODER_MODER7_1                  (0x2U << GPIO_MODER_MODER7_Pos)   /*!< 0x00008000 */

#define GPIO_MODER_MODER8_Pos                (16U)                             
#define GPIO_MODER_MODER8_Msk                (0x3U << GPIO_MODER_MODER8_Pos)   /*!< 0x00030000 */
#define GPIO_MODER_MODER8                    GPIO_MODER_MODER8_Msk             
#define GPIO_MODER_MODER8_0                  (0x1U << GPIO_MODER_MODER8_Pos)   /*!< 0x00010000 */
#define GPIO_MODER_MODER8_1                  (0x2U << GPIO_MODER_MODER8_Pos)   /*!< 0x00020000 */

#define GPIO_MODER_MODER9_Pos                (18U)                             
#define GPIO_MODER_MODER9_Msk                (0x3U << GPIO_MODER_MODER9_Pos)   /*!< 0x000C0000 */
#define GPIO_MODER_MODER9                    GPIO_MODER_MODER9_Msk             
#define GPIO_MODER_MODER9_0                  (0x1U << GPIO_MODER_MODER9_Pos)   /*!< 0x00040000 */
#define GPIO_MODER_MODER9_1                  (0x2U << GPIO_MODER_MODER9_Pos)   /*!< 0x00080000 */

#define GPIO_MODER_MODER10_Pos               (20U)                             
#define GPIO_MODER_MODER10_Msk               (0x3U << GPIO_MODER_MODER10_Pos)  /*!< 0x00300000 */
#define GPIO_MODER_MODER10                   GPIO_MODER_MODER10_Msk            
#define GPIO_MODER_MODER10_0                 (0x1U << GPIO_MODER_MODER10_Pos)  /*!< 0x00100000 */
#define GPIO_MODER_MODER10_1                 (0x2U << GPIO_MODER_MODER10_Pos)  /*!< 0x00200000 */

#define GPIO_MODER_MODER11_Pos               (22U)                             
#define GPIO_MODER_MODER11_Msk               (0x3U << GPIO_MODER_MODER11_Pos)  /*!< 0x00C00000 */
#define GPIO_MODER_MODER11                   GPIO_MODER_MODER11_Msk            
#define GPIO_MODER_MODER11_0                 (0x1U << GPIO_MODER_MODER11_Pos)  /*!< 0x00400000 */
#define GPIO_MODER_MODER11_1                 (0x2U << GPIO_MODER_MODER11_Pos)  /*!< 0x00800000 */

#define GPIO_MODER_MODER12_Pos               (24U)                             
#define GPIO_MODER_MODER12_Msk               (0x3U << GPIO_MODER_MODER12_Pos)  /*!< 0x03000000 */
#define GPIO_MODER_MODER12                   GPIO_MODER_MODER12_Msk            
#define GPIO_MODER_MODER12_0                 (0x1U << GPIO_MODER_MODER12_Pos)  /*!< 0x01000000 */
#define GPIO_MODER_MODER12_1                 (0x2U << GPIO_MODER_MODER12_Pos)  /*!< 0x02000000 */

#define GPIO_MODER_MODER13_Pos               (26U)                             
#define GPIO_MODER_MODER13_Msk               (0x3U << GPIO_MODER_MODER13_Pos)  /*!< 0x0C000000 */
#define GPIO_MODER_MODER13                   GPIO_MODER_MODER13_Msk            
#define GPIO_MODER_MODER13_0                 (0x1U << GPIO_MODER_MODER13_Pos)  /*!< 0x04000000 */
#define GPIO_MODER_MODER13_1                 (0x2U << GPIO_MODER_MODER13_Pos)  /*!< 0x08000000 */

#define GPIO_MODER_MODER14_Pos               (28U)                             
#define GPIO_MODER_MODER14_Msk               (0x3U << GPIO_MODER_MODER14_Pos)  /*!< 0x30000000 */
#define GPIO_MODER_MODER14                   GPIO_MODER_MODER14_Msk            
#define GPIO_MODER_MODER14_0                 (0x1U << GPIO_MODER_MODER14_Pos)  /*!< 0x10000000 */
#define GPIO_MODER_MODER14_1                 (0x2U << GPIO_MODER_MODER14_Pos)  /*!< 0x20000000 */

#define GPIO_MODER_MODER15_Pos               (30U)                             
#define GPIO_MODER_MODER15_Msk               (0x3U << GPIO_MODER_MODER15_Pos)  /*!< 0xC0000000 */
#define GPIO_MODER_MODER15                   GPIO_MODER_MODER15_Msk            
#define GPIO_MODER_MODER15_0                 (0x1U << GPIO_MODER_MODER15_Pos)  /*!< 0x40000000 */
#define GPIO_MODER_MODER15_1                 (0x2U << GPIO_MODER_MODER15_Pos)  /*!< 0x80000000 */

/******************  Bits definition for GPIO_OTYPER register  ****************/
#define GPIO_OTYPER_OT_0                     (0x00000001U)                     
#define GPIO_OTYPER_OT_1                     (0x00000002U)                     
#define GPIO_OTYPER_OT_2                     (0x00000004U)                     
#define GPIO_OTYPER_OT_3                     (0x00000008U)                     
#define GPIO_OTYPER_OT_4                     (0x00000010U)                     
#define GPIO_OTYPER_OT_5                     (0x00000020U)                     
#define GPIO_OTYPER_OT_6                     (0x00000040U)                     
#define GPIO_OTYPER_OT_7                     (0x00000080U)                     
#define GPIO_OTYPER_OT_8                     (0x00000100U)                     
#define GPIO_OTYPER_OT_9                     (0x00000200U)                     
#define GPIO_OTYPER_OT_10                    (0x00000400U)                     
#define GPIO_OTYPER_OT_11                    (0x00000800U)                     
#define GPIO_OTYPER_OT_12                    (0x00001000U)                     
#define GPIO_OTYPER_OT_13                    (0x00002000U)                     
#define GPIO_OTYPER_OT_14                    (0x00004000U)                     
#define GPIO_OTYPER_OT_15                    (0x00008000U)                     

/******************  Bits definition for GPIO_OSPEEDR register  ***************/
#define GPIO_OSPEEDER_OSPEEDR0_Pos           (0U)                              
#define GPIO_OSPEEDER_OSPEEDR0_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR0_Pos) /*!< 0x00000003 */
#define GPIO_OSPEEDER_OSPEEDR0               GPIO_OSPEEDER_OSPEEDR0_Msk        
#define GPIO_OSPEEDER_OSPEEDR0_0             (0x1U << GPIO_OSPEEDER_OSPEEDR0_Pos) /*!< 0x00000001 */
#define GPIO_OSPEEDER_OSPEEDR0_1             (0x2U << GPIO_OSPEEDER_OSPEEDR0_Pos) /*!< 0x00000002 */

#define GPIO_OSPEEDER_OSPEEDR1_Pos           (2U)                              
#define GPIO_OSPEEDER_OSPEEDR1_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR1_Pos) /*!< 0x0000000C */
#define GPIO_OSPEEDER_OSPEEDR1               GPIO_OSPEEDER_OSPEEDR1_Msk        
#define GPIO_OSPEEDER_OSPEEDR1_0             (0x1U << GPIO_OSPEEDER_OSPEEDR1_Pos) /*!< 0x00000004 */
#define GPIO_OSPEEDER_OSPEEDR1_1             (0x2U << GPIO_OSPEEDER_OSPEEDR1_Pos) /*!< 0x00000008 */

#define GPIO_OSPEEDER_OSPEEDR2_Pos           (4U)                              
#define GPIO_OSPEEDER_OSPEEDR2_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR2_Pos) /*!< 0x00000030 */
#define GPIO_OSPEEDER_OSPEEDR2               GPIO_OSPEEDER_OSPEEDR2_Msk        
#define GPIO_OSPEEDER_OSPEEDR2_0             (0x1U << GPIO_OSPEEDER_OSPEEDR2_Pos) /*!< 0x00000010 */
#define GPIO_OSPEEDER_OSPEEDR2_1             (0x2U << GPIO_OSPEEDER_OSPEEDR2_Pos) /*!< 0x00000020 */

#define GPIO_OSPEEDER_OSPEEDR3_Pos           (6U)                              
#define GPIO_OSPEEDER_OSPEEDR3_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR3_Pos) /*!< 0x000000C0 */
#define GPIO_OSPEEDER_OSPEEDR3               GPIO_OSPEEDER_OSPEEDR3_Msk        
#define GPIO_OSPEEDER_OSPEEDR3_0             (0x1U << GPIO_OSPEEDER_OSPEEDR3_Pos) /*!< 0x00000040 */
#define GPIO_OSPEEDER_OSPEEDR3_1             (0x2U << GPIO_OSPEEDER_OSPEEDR3_Pos) /*!< 0x00000080 */

#define GPIO_OSPEEDER_OSPEEDR4_Pos           (8U)                              
#define GPIO_OSPEEDER_OSPEEDR4_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR4_Pos) /*!< 0x00000300 */
#define GPIO_OSPEEDER_OSPEEDR4               GPIO_OSPEEDER_OSPEEDR4_Msk        
#define GPIO_OSPEEDER_OSPEEDR4_0             (0x1U << GPIO_OSPEEDER_OSPEEDR4_Pos) /*!< 0x00000100 */
#define GPIO_OSPEEDER_OSPEEDR4_1             (0x2U << GPIO_OSPEEDER_OSPEEDR4_Pos) /*!< 0x00000200 */

#define GPIO_OSPEEDER_OSPEEDR5_Pos           (10U)                             
#define GPIO_OSPEEDER_OSPEEDR5_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR5_Pos) /*!< 0x00000C00 */
#define GPIO_OSPEEDER_OSPEEDR5               GPIO_OSPEEDER_OSPEEDR5_Msk        
#define GPIO_OSPEEDER_OSPEEDR5_0             (0x1U << GPIO_OSPEEDER_OSPEEDR5_Pos) /*!< 0x00000400 */
#define GPIO_OSPEEDER_OSPEEDR5_1             (0x2U << GPIO_OSPEEDER_OSPEEDR5_Pos) /*!< 0x00000800 */

#define GPIO_OSPEEDER_OSPEEDR6_Pos           (12U)                             
#define GPIO_OSPEEDER_OSPEEDR6_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR6_Pos) /*!< 0x00003000 */
#define GPIO_OSPEEDER_OSPEEDR6               GPIO_OSPEEDER_OSPEEDR6_Msk        
#define GPIO_OSPEEDER_OSPEEDR6_0             (0x1U << GPIO_OSPEEDER_OSPEEDR6_Pos) /*!< 0x00001000 */
#define GPIO_OSPEEDER_OSPEEDR6_1             (0x2U << GPIO_OSPEEDER_OSPEEDR6_Pos) /*!< 0x00002000 */

#define GPIO_OSPEEDER_OSPEEDR7_Pos           (14U)                             
#define GPIO_OSPEEDER_OSPEEDR7_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR7_Pos) /*!< 0x0000C000 */
#define GPIO_OSPEEDER_OSPEEDR7               GPIO_OSPEEDER_OSPEEDR7_Msk        
#define GPIO_OSPEEDER_OSPEEDR7_0             (0x1U << GPIO_OSPEEDER_OSPEEDR7_Pos) /*!< 0x00004000 */
#define GPIO_OSPEEDER_OSPEEDR7_1             (0x2U << GPIO_OSPEEDER_OSPEEDR7_Pos) /*!< 0x00008000 */

#define GPIO_OSPEEDER_OSPEEDR8_Pos           (16U)                             
#define GPIO_OSPEEDER_OSPEEDR8_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR8_Pos) /*!< 0x00030000 */
#define GPIO_OSPEEDER_OSPEEDR8               GPIO_OSPEEDER_OSPEEDR8_Msk        
#define GPIO_OSPEEDER_OSPEEDR8_0             (0x1U << GPIO_OSPEEDER_OSPEEDR8_Pos) /*!< 0x00010000 */
#define GPIO_OSPEEDER_OSPEEDR8_1             (0x2U << GPIO_OSPEEDER_OSPEEDR8_Pos) /*!< 0x00020000 */

#define GPIO_OSPEEDER_OSPEEDR9_Pos           (18U)                             
#define GPIO_OSPEEDER_OSPEEDR9_Msk           (0x3U << GPIO_OSPEEDER_OSPEEDR9_Pos) /*!< 0x000C0000 */
#define GPIO_OSPEEDER_OSPEEDR9               GPIO_OSPEEDER_OSPEEDR9_Msk        
#define GPIO_OSPEEDER_OSPEEDR9_0             (0x1U << GPIO_OSPEEDER_OSPEEDR9_Pos) /*!< 0x00040000 */
#define GPIO_OSPEEDER_OSPEEDR9_1             (0x2U << GPIO_OSPEEDER_OSPEEDR9_Pos) /*!< 0x00080000 */

#define GPIO_OSPEEDER_OSPEEDR10_Pos          (20U)                             
#define GPIO_OSPEEDER_OSPEEDR10_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR10_Pos) /*!< 0x00300000 */
#define GPIO_OSPEEDER_OSPEEDR10              GPIO_OSPEEDER_OSPEEDR10_Msk       
#define GPIO_OSPEEDER_OSPEEDR10_0            (0x1U << GPIO_OSPEEDER_OSPEEDR10_Pos) /*!< 0x00100000 */
#define GPIO_OSPEEDER_OSPEEDR10_1            (0x2U << GPIO_OSPEEDER_OSPEEDR10_Pos) /*!< 0x00200000 */

#define GPIO_OSPEEDER_OSPEEDR11_Pos          (22U)                             
#define GPIO_OSPEEDER_OSPEEDR11_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR11_Pos) /*!< 0x00C00000 */
#define GPIO_OSPEEDER_OSPEEDR11              GPIO_OSPEEDER_OSPEEDR11_Msk       
#define GPIO_OSPEEDER_OSPEEDR11_0            (0x1U << GPIO_OSPEEDER_OSPEEDR11_Pos) /*!< 0x00400000 */
#define GPIO_OSPEEDER_OSPEEDR11_1            (0x2U << GPIO_OSPEEDER_OSPEEDR11_Pos) /*!< 0x00800000 */

#define GPIO_OSPEEDER_OSPEEDR12_Pos          (24U)                             
#define GPIO_OSPEEDER_OSPEEDR12_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR12_Pos) /*!< 0x03000000 */
#define GPIO_OSPEEDER_OSPEEDR12              GPIO_OSPEEDER_OSPEEDR12_Msk       
#define GPIO_OSPEEDER_OSPEEDR12_0            (0x1U << GPIO_OSPEEDER_OSPEEDR12_Pos) /*!< 0x01000000 */
#define GPIO_OSPEEDER_OSPEEDR12_1            (0x2U << GPIO_OSPEEDER_OSPEEDR12_Pos) /*!< 0x02000000 */

#define GPIO_OSPEEDER_OSPEEDR13_Pos          (26U)                             
#define GPIO_OSPEEDER_OSPEEDR13_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR13_Pos) /*!< 0x0C000000 */
#define GPIO_OSPEEDER_OSPEEDR13              GPIO_OSPEEDER_OSPEEDR13_Msk       
#define GPIO_OSPEEDER_OSPEEDR13_0            (0x1U << GPIO_OSPEEDER_OSPEEDR13_Pos) /*!< 0x04000000 */
#define GPIO_OSPEEDER_OSPEEDR13_1            (0x2U << GPIO_OSPEEDER_OSPEEDR13_Pos) /*!< 0x08000000 */

#define GPIO_OSPEEDER_OSPEEDR14_Pos          (28U)                             
#define GPIO_OSPEEDER_OSPEEDR14_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR14_Pos) /*!< 0x30000000 */
#define GPIO_OSPEEDER_OSPEEDR14              GPIO_OSPEEDER_OSPEEDR14_Msk       
#define GPIO_OSPEEDER_OSPEEDR14_0            (0x1U << GPIO_OSPEEDER_OSPEEDR14_Pos) /*!< 0x10000000 */
#define GPIO_OSPEEDER_OSPEEDR14_1            (0x2U << GPIO_OSPEEDER_OSPEEDR14_Pos) /*!< 0x20000000 */

#define GPIO_OSPEEDER_OSPEEDR15_Pos          (30U)                             
#define GPIO_OSPEEDER_OSPEEDR15_Msk          (0x3U << GPIO_OSPEEDER_OSPEEDR15_Pos) /*!< 0xC0000000 */
#define GPIO_OSPEEDER_OSPEEDR15              GPIO_OSPEEDER_OSPEEDR15_Msk       
#define GPIO_OSPEEDER_OSPEEDR15_0            (0x1U << GPIO_OSPEEDER_OSPEEDR15_Pos) /*!< 0x40000000 */
#define GPIO_OSPEEDER_OSPEEDR15_1            (0x2U << GPIO_OSPEEDER_OSPEEDR15_Pos) /*!< 0x80000000 */

/******************  Bits definition for GPIO_PUPDR register  *****************/
#define GPIO_PUPDR_PUPDR0_Pos                (0U)                              
#define GPIO_PUPDR_PUPDR0_Msk                (0x3U << GPIO_PUPDR_PUPDR0_Pos)   /*!< 0x00000003 */
#define GPIO_PUPDR_PUPDR0                    GPIO_PUPDR_PUPDR0_Msk             
#define GPIO_PUPDR_PUPDR0_0                  (0x1U << GPIO_PUPDR_PUPDR0_Pos)   /*!< 0x00000001 */
#define GPIO_PUPDR_PUPDR0_1                  (0x2U << GPIO_PUPDR_PUPDR0_Pos)   /*!< 0x00000002 */

#define GPIO_PUPDR_PUPDR1_Pos                (2U)                              
#define GPIO_PUPDR_PUPDR1_Msk                (0x3U << GPIO_PUPDR_PUPDR1_Pos)   /*!< 0x0000000C */
#define GPIO_PUPDR_PUPDR1                    GPIO_PUPDR_PUPDR1_Msk             
#define GPIO_PUPDR_PUPDR1_0                  (0x1U << GPIO_PUPDR_PUPDR1_Pos)   /*!< 0x00000004 */
#define GPIO_PUPDR_PUPDR1_1                  (0x2U << GPIO_PUPDR_PUPDR1_Pos)   /*!< 0x00000008 */

#define GPIO_PUPDR_PUPDR2_Pos                (4U)                              
#define GPIO_PUPDR_PUPDR2_Msk                (0x3U << GPIO_PUPDR_PUPDR2_Pos)   /*!< 0x00000030 */
#define GPIO_PUPDR_PUPDR2                    GPIO_PUPDR_PUPDR2_Msk             
#define GPIO_PUPDR_PUPDR2_0                  (0x1U << GPIO_PUPDR_PUPDR2_Pos)   /*!< 0x00000010 */
#define GPIO_PUPDR_PUPDR2_1                  (0x2U << GPIO_PUPDR_PUPDR2_Pos)   /*!< 0x00000020 */

#define GPIO_PUPDR_PUPDR3_Pos                (6U)                              
#define GPIO_PUPDR_PUPDR3_Msk                (0x3U << GPIO_PUPDR_PUPDR3_Pos)   /*!< 0x000000C0 */
#define GPIO_PUPDR_PUPDR3                    GPIO_PUPDR_PUPDR3_Msk             
#define GPIO_PUPDR_PUPDR3_0                  (0x1U << GPIO_PUPDR_PUPDR3_Pos)   /*!< 0x00000040 */
#define GPIO_PUPDR_PUPDR3_1                  (0x2U << GPIO_PUPDR_PUPDR3_Pos)   /*!< 0x00000080 */

#define GPIO_PUPDR_PUPDR4_Pos                (8U)                              
#define GPIO_PUPDR_PUPDR4_Msk                (0x3U << GPIO_PUPDR_PUPDR4_Pos)   /*!< 0x00000300 */
#define GPIO_PUPDR_PUPDR4                    GPIO_PUPDR_PUPDR4_Msk             
#define GPIO_PUPDR_PUPDR4_0                  (0x1U << GPIO_PUPDR_PUPDR4_Pos)   /*!< 0x00000100 */
#define GPIO_PUPDR_PUPDR4_1                  (0x2U << GPIO_PUPDR_PUPDR4_Pos)   /*!< 0x00000200 */

#define GPIO_PUPDR_PUPDR5_Pos                (10U)                             
#define GPIO_PUPDR_PUPDR5_Msk                (0x3U << GPIO_PUPDR_PUPDR5_Pos)   /*!< 0x00000C00 */
#define GPIO_PUPDR_PUPDR5                    GPIO_PUPDR_PUPDR5_Msk             
#define GPIO_PUPDR_PUPDR5_0                  (0x1U << GPIO_PUPDR_PUPDR5_Pos)   /*!< 0x00000400 */
#define GPIO_PUPDR_PUPDR5_1                  (0x2U << GPIO_PUPDR_PUPDR5_Pos)   /*!< 0x00000800 */

#define GPIO_PUPDR_PUPDR6_Pos                (12U)                             
#define GPIO_PUPDR_PUPDR6_Msk                (0x3U << GPIO_PUPDR_PUPDR6_Pos)   /*!< 0x00003000 */
#define GPIO_PUPDR_PUPDR6                    GPIO_PUPDR_PUPDR6_Msk             
#define GPIO_PUPDR_PUPDR6_0                  (0x1U << GPIO_PUPDR_PUPDR6_Pos)   /*!< 0x00001000 */
#define GPIO_PUPDR_PUPDR6_1                  (0x2U << GPIO_PUPDR_PUPDR6_Pos)   /*!< 0x00002000 */

#define GPIO_PUPDR_PUPDR7_Pos                (14U)                             
#define GPIO_PUPDR_PUPDR7_Msk                (0x3U << GPIO_PUPDR_PUPDR7_Pos)   /*!< 0x0000C000 */
#define GPIO_PUPDR_PUPDR7                    GPIO_PUPDR_PUPDR7_Msk             
#define GPIO_PUPDR_PUPDR7_0                  (0x1U << GPIO_PUPDR_PUPDR7_Pos)   /*!< 0x00004000 */
#define GPIO_PUPDR_PUPDR7_1                  (0x2U << GPIO_PUPDR_PUPDR7_Pos)   /*!< 0x00008000 */

#define GPIO_PUPDR_PUPDR8_Pos                (16U)                             
#define GPIO_PUPDR_PUPDR8_Msk                (0x3U << GPIO_PUPDR_PUPDR8_Pos)   /*!< 0x00030000 */
#define GPIO_PUPDR_PUPDR8                    GPIO_PUPDR_PUPDR8_Msk             
#define GPIO_PUPDR_PUPDR8_0                  (0x1U << GPIO_PUPDR_PUPDR8_Pos)   /*!< 0x00010000 */
#define GPIO_PUPDR_PUPDR8_1                  (0x2U << GPIO_PUPDR_PUPDR8_Pos)   /*!< 0x00020000 */

#define GPIO_PUPDR_PUPDR9_Pos                (18U)                             
#define GPIO_PUPDR_PUPDR9_Msk                (0x3U << GPIO_PUPDR_PUPDR9_Pos)   /*!< 0x000C0000 */
#define GPIO_PUPDR_PUPDR9                    GPIO_PUPDR_PUPDR9_Msk             
#define GPIO_PUPDR_PUPDR9_0                  (0x1U << GPIO_PUPDR_PUPDR9_Pos)   /*!< 0x00040000 */
#define GPIO_PUPDR_PUPDR9_1                  (0x2U << GPIO_PUPDR_PUPDR9_Pos)   /*!< 0x00080000 */

#define GPIO_PUPDR_PUPDR10_Pos               (20U)                             
#define GPIO_PUPDR_PUPDR10_Msk               (0x3U << GPIO_PUPDR_PUPDR10_Pos)  /*!< 0x00300000 */
#define GPIO_PUPDR_PUPDR10                   GPIO_PUPDR_PUPDR10_Msk            
#define GPIO_PUPDR_PUPDR10_0                 (0x1U << GPIO_PUPDR_PUPDR10_Pos)  /*!< 0x00100000 */
#define GPIO_PUPDR_PUPDR10_1                 (0x2U << GPIO_PUPDR_PUPDR10_Pos)  /*!< 0x00200000 */

#define GPIO_PUPDR_PUPDR11_Pos               (22U)                             
#define GPIO_PUPDR_PUPDR11_Msk               (0x3U << GPIO_PUPDR_PUPDR11_Pos)  /*!< 0x00C00000 */
#define GPIO_PUPDR_PUPDR11                   GPIO_PUPDR_PUPDR11_Msk            
#define GPIO_PUPDR_PUPDR11_0                 (0x1U << GPIO_PUPDR_PUPDR11_Pos)  /*!< 0x00400000 */
#define GPIO_PUPDR_PUPDR11_1                 (0x2U << GPIO_PUPDR_PUPDR11_Pos)  /*!< 0x00800000 */

#define GPIO_PUPDR_PUPDR12_Pos               (24U)                             
#define GPIO_PUPDR_PUPDR12_Msk               (0x3U << GPIO_PUPDR_PUPDR12_Pos)  /*!< 0x03000000 */
#define GPIO_PUPDR_PUPDR12                   GPIO_PUPDR_PUPDR12_Msk            
#define GPIO_PUPDR_PUPDR12_0                 (0x1U << GPIO_PUPDR_PUPDR12_Pos)  /*!< 0x01000000 */
#define GPIO_PUPDR_PUPDR12_1                 (0x2U << GPIO_PUPDR_PUPDR12_Pos)  /*!< 0x02000000 */

#define GPIO_PUPDR_PUPDR13_Pos               (26U)                             
#define GPIO_PUPDR_PUPDR13_Msk               (0x3U << GPIO_PUPDR_PUPDR13_Pos)  /*!< 0x0C000000 */
#define GPIO_PUPDR_PUPDR13                   GPIO_PUPDR_PUPDR13_Msk            
#define GPIO_PUPDR_PUPDR13_0                 (0x1U << GPIO_PUPDR_PUPDR13_Pos)  /*!< 0x04000000 */
#define GPIO_PUPDR_PUPDR13_1                 (0x2U << GPIO_PUPDR_PUPDR13_Pos)  /*!< 0x08000000 */

#define GPIO_PUPDR_PUPDR14_Pos               (28U)                             
#define GPIO_PUPDR_PUPDR14_Msk               (0x3U << GPIO_PUPDR_PUPDR14_Pos)  /*!< 0x30000000 */
#define GPIO_PUPDR_PUPDR14                   GPIO_PUPDR_PUPDR14_Msk            
#define GPIO_PUPDR_PUPDR14_0                 (0x1U << GPIO_PUPDR_PUPDR14_Pos)  /*!< 0x10000000 */
#define GPIO_PUPDR_PUPDR14_1                 (0x2U << GPIO_PUPDR_PUPDR14_Pos)  /*!< 0x20000000 */
#define GPIO_PUPDR_PUPDR15_Pos               (30U)                             
#define GPIO_PUPDR_PUPDR15_Msk               (0x3U << GPIO_PUPDR_PUPDR15_Pos)  /*!< 0xC0000000 */
#define GPIO_PUPDR_PUPDR15                   GPIO_PUPDR_PUPDR15_Msk            
#define GPIO_PUPDR_PUPDR15_0                 (0x1U << GPIO_PUPDR_PUPDR15_Pos)  /*!< 0x40000000 */
#define GPIO_PUPDR_PUPDR15_1                 (0x2U << GPIO_PUPDR_PUPDR15_Pos)  /*!< 0x80000000 */

/******************  Bits definition for GPIO_IDR register  *******************/
#define GPIO_IDR_IDR_0                       (0x00000001U)                     
#define GPIO_IDR_IDR_1                       (0x00000002U)                     
#define GPIO_IDR_IDR_2                       (0x00000004U)                     
#define GPIO_IDR_IDR_3                       (0x00000008U)                     
#define GPIO_IDR_IDR_4                       (0x00000010U)                     
#define GPIO_IDR_IDR_5                       (0x00000020U)                     
#define GPIO_IDR_IDR_6                       (0x00000040U)                     
#define GPIO_IDR_IDR_7                       (0x00000080U)                     
#define GPIO_IDR_IDR_8                       (0x00000100U)                     
#define GPIO_IDR_IDR_9                       (0x00000200U)                     
#define GPIO_IDR_IDR_10                      (0x00000400U)                     
#define GPIO_IDR_IDR_11                      (0x00000800U)                     
#define GPIO_IDR_IDR_12                      (0x00001000U)                     
#define GPIO_IDR_IDR_13                      (0x00002000U)                     
#define GPIO_IDR_IDR_14                      (0x00004000U)                     
#define GPIO_IDR_IDR_15                      (0x00008000U)                     

/******************  Bits definition for GPIO_ODR register  *******************/
#define GPIO_ODR_ODR_0                       (0x00000001U)                     
#define GPIO_ODR_ODR_1                       (0x00000002U)                     
#define GPIO_ODR_ODR_2                       (0x00000004U)                     
#define GPIO_ODR_ODR_3                       (0x00000008U)                     
#define GPIO_ODR_ODR_4                       (0x00000010U)                     
#define GPIO_ODR_ODR_5                       (0x00000020U)                     
#define GPIO_ODR_ODR_6                       (0x00000040U)                     
#define GPIO_ODR_ODR_7                       (0x00000080U)                     
#define GPIO_ODR_ODR_8                       (0x00000100U)                     
#define GPIO_ODR_ODR_9                       (0x00000200U)                     
#define GPIO_ODR_ODR_10                      (0x00000400U)                     
#define GPIO_ODR_ODR_11                      (0x00000800U)                     
#define GPIO_ODR_ODR_12                      (0x00001000U)                     
#define GPIO_ODR_ODR_13                      (0x00002000U)                     
#define GPIO_ODR_ODR_14                      (0x00004000U)                     
#define GPIO_ODR_ODR_15                      (0x00008000U)                     

/******************  Bits definition for GPIO_BSRR register  ******************/
#define GPIO_BSRR_BS_0                       (0x00000001U)                     
#define GPIO_BSRR_BS_1                       (0x00000002U)                     
#define GPIO_BSRR_BS_2                       (0x00000004U)                     
#define GPIO_BSRR_BS_3                       (0x00000008U)                     
#define GPIO_BSRR_BS_4                       (0x00000010U)                     
#define GPIO_BSRR_BS_5                       (0x00000020U)                     
#define GPIO_BSRR_BS_6                       (0x00000040U)                     
#define GPIO_BSRR_BS_7                       (0x00000080U)                     
#define GPIO_BSRR_BS_8                       (0x00000100U)                     
#define GPIO_BSRR_BS_9                       (0x00000200U)                     
#define GPIO_BSRR_BS_10                      (0x00000400U)                     
#define GPIO_BSRR_BS_11                      (0x00000800U)                     
#define GPIO_BSRR_BS_12                      (0x00001000U)                     
#define GPIO_BSRR_BS_13                      (0x00002000U)                     
#define GPIO_BSRR_BS_14                      (0x00004000U)                     
#define GPIO_BSRR_BS_15                      (0x00008000U)                     
#define GPIO_BSRR_BR_0                       (0x00010000U)                     
#define GPIO_BSRR_BR_1                       (0x00020000U)                     
#define GPIO_BSRR_BR_2                       (0x00040000U)                     
#define GPIO_BSRR_BR_3                       (0x00080000U)                     
#define GPIO_BSRR_BR_4                       (0x00100000U)                     
#define GPIO_BSRR_BR_5                       (0x00200000U)                     
#define GPIO_BSRR_BR_6                       (0x00400000U)                     
#define GPIO_BSRR_BR_7                       (0x00800000U)                     
#define GPIO_BSRR_BR_8                       (0x01000000U)                     
#define GPIO_BSRR_BR_9                       (0x02000000U)                     
#define GPIO_BSRR_BR_10                      (0x04000000U)                     
#define GPIO_BSRR_BR_11                      (0x08000000U)                     
#define GPIO_BSRR_BR_12                      (0x10000000U)                     
#define GPIO_BSRR_BR_13                      (0x20000000U)                     
#define GPIO_BSRR_BR_14                      (0x40000000U)                     
#define GPIO_BSRR_BR_15                      (0x80000000U)                     

/****************** Bit definition for GPIO_LCKR register  ********************/
#define GPIO_LCKR_LCK0_Pos                   (0U)                              
#define GPIO_LCKR_LCK0_Msk                   (0x1U << GPIO_LCKR_LCK0_Pos)      /*!< 0x00000001 */
#define GPIO_LCKR_LCK0                       GPIO_LCKR_LCK0_Msk                
#define GPIO_LCKR_LCK1_Pos                   (1U)                              
#define GPIO_LCKR_LCK1_Msk                   (0x1U << GPIO_LCKR_LCK1_Pos)      /*!< 0x00000002 */
#define GPIO_LCKR_LCK1                       GPIO_LCKR_LCK1_Msk                
#define GPIO_LCKR_LCK2_Pos                   (2U)                              
#define GPIO_LCKR_LCK2_Msk                   (0x1U << GPIO_LCKR_LCK2_Pos)      /*!< 0x00000004 */
#define GPIO_LCKR_LCK2                       GPIO_LCKR_LCK2_Msk                
#define GPIO_LCKR_LCK3_Pos                   (3U)                              
#define GPIO_LCKR_LCK3_Msk                   (0x1U << GPIO_LCKR_LCK3_Pos)      /*!< 0x00000008 */
#define GPIO_LCKR_LCK3                       GPIO_LCKR_LCK3_Msk                
#define GPIO_LCKR_LCK4_Pos                   (4U)                              
#define GPIO_LCKR_LCK4_Msk                   (0x1U << GPIO_LCKR_LCK4_Pos)      /*!< 0x00000010 */
#define GPIO_LCKR_LCK4                       GPIO_LCKR_LCK4_Msk                
#define GPIO_LCKR_LCK5_Pos                   (5U)                              
#define GPIO_LCKR_LCK5_Msk                   (0x1U << GPIO_LCKR_LCK5_Pos)      /*!< 0x00000020 */
#define GPIO_LCKR_LCK5                       GPIO_LCKR_LCK5_Msk                
#define GPIO_LCKR_LCK6_Pos                   (6U)                              
#define GPIO_LCKR_LCK6_Msk                   (0x1U << GPIO_LCKR_LCK6_Pos)      /*!< 0x00000040 */
#define GPIO_LCKR_LCK6                       GPIO_LCKR_LCK6_Msk                
#define GPIO_LCKR_LCK7_Pos                   (7U)                              
#define GPIO_LCKR_LCK7_Msk                   (0x1U << GPIO_LCKR_LCK7_Pos)      /*!< 0x00000080 */
#define GPIO_LCKR_LCK7                       GPIO_LCKR_LCK7_Msk                
#define GPIO_LCKR_LCK8_Pos                   (8U)                              
#define GPIO_LCKR_LCK8_Msk                   (0x1U << GPIO_LCKR_LCK8_Pos)      /*!< 0x00000100 */
#define GPIO_LCKR_LCK8                       GPIO_LCKR_LCK8_Msk                
#define GPIO_LCKR_LCK9_Pos                   (9U)                              
#define GPIO_LCKR_LCK9_Msk                   (0x1U << GPIO_LCKR_LCK9_Pos)      /*!< 0x00000200 */
#define GPIO_LCKR_LCK9                       GPIO_LCKR_LCK9_Msk                
#define GPIO_LCKR_LCK10_Pos                  (10U)                             
#define GPIO_LCKR_LCK10_Msk                  (0x1U << GPIO_LCKR_LCK10_Pos)     /*!< 0x00000400 */
#define GPIO_LCKR_LCK10                      GPIO_LCKR_LCK10_Msk               
#define GPIO_LCKR_LCK11_Pos                  (11U)                             
#define GPIO_LCKR_LCK11_Msk                  (0x1U << GPIO_LCKR_LCK11_Pos)     /*!< 0x00000800 */
#define GPIO_LCKR_LCK11                      GPIO_LCKR_LCK11_Msk               
#define GPIO_LCKR_LCK12_Pos                  (12U)                             
#define GPIO_LCKR_LCK12_Msk                  (0x1U << GPIO_LCKR_LCK12_Pos)     /*!< 0x00001000 */
#define GPIO_LCKR_LCK12                      GPIO_LCKR_LCK12_Msk               
#define GPIO_LCKR_LCK13_Pos                  (13U)                             
#define GPIO_LCKR_LCK13_Msk                  (0x1U << GPIO_LCKR_LCK13_Pos)     /*!< 0x00002000 */
#define GPIO_LCKR_LCK13                      GPIO_LCKR_LCK13_Msk               
#define GPIO_LCKR_LCK14_Pos                  (14U)                             
#define GPIO_LCKR_LCK14_Msk                  (0x1U << GPIO_LCKR_LCK14_Pos)     /*!< 0x00004000 */
#define GPIO_LCKR_LCK14                      GPIO_LCKR_LCK14_Msk               
#define GPIO_LCKR_LCK15_Pos                  (15U)                             
#define GPIO_LCKR_LCK15_Msk                  (0x1U << GPIO_LCKR_LCK15_Pos)     /*!< 0x00008000 */
#define GPIO_LCKR_LCK15                      GPIO_LCKR_LCK15_Msk               
#define GPIO_LCKR_LCKK_Pos                   (16U)                             
#define GPIO_LCKR_LCKK_Msk                   (0x1U << GPIO_LCKR_LCKK_Pos)      /*!< 0x00010000 */
#define GPIO_LCKR_LCKK                       GPIO_LCKR_LCKK_Msk                

/****************** Bit definition for GPIO_AFRL register  ********************/
#define GPIO_AFRL_AFSEL0_Pos                  (0U)                              
#define GPIO_AFRL_AFSEL0_Msk                  (0xFU << GPIO_AFRL_AFSEL0_Pos)     /*!< 0x0000000F */
#define GPIO_AFRL_AFSEL0                      GPIO_AFRL_AFSEL0_Msk               
#define GPIO_AFRL_AFSEL1_Pos                  (4U)                              
#define GPIO_AFRL_AFSEL1_Msk                  (0xFU << GPIO_AFRL_AFSEL1_Pos)     /*!< 0x000000F0 */
#define GPIO_AFRL_AFSEL1                      GPIO_AFRL_AFSEL1_Msk               
#define GPIO_AFRL_AFSEL2_Pos                  (8U)                              
#define GPIO_AFRL_AFSEL2_Msk                  (0xFU << GPIO_AFRL_AFSEL2_Pos)     /*!< 0x00000F00 */
#define GPIO_AFRL_AFSEL2                      GPIO_AFRL_AFSEL2_Msk               
#define GPIO_AFRL_AFSEL3_Pos                  (12U)                             
#define GPIO_AFRL_AFSEL3_Msk                  (0xFU << GPIO_AFRL_AFSEL3_Pos)     /*!< 0x0000F000 */
#define GPIO_AFRL_AFSEL3                      GPIO_AFRL_AFSEL3_Msk               
#define GPIO_AFRL_AFSEL4_Pos                  (16U)                             
#define GPIO_AFRL_AFSEL4_Msk                  (0xFU << GPIO_AFRL_AFSEL4_Pos)     /*!< 0x000F0000 */
#define GPIO_AFRL_AFSEL4                      GPIO_AFRL_AFSEL4_Msk               
#define GPIO_AFRL_AFSEL5_Pos                  (20U)                             
#define GPIO_AFRL_AFSEL5_Msk                  (0xFU << GPIO_AFRL_AFSEL5_Pos)     /*!< 0x00F00000 */
#define GPIO_AFRL_AFSEL5                      GPIO_AFRL_AFSEL5_Msk               
#define GPIO_AFRL_AFSEL6_Pos                  (24U)                             
#define GPIO_AFRL_AFSEL6_Msk                  (0xFU << GPIO_AFRL_AFSEL6_Pos)     /*!< 0x0F000000 */
#define GPIO_AFRL_AFSEL6                      GPIO_AFRL_AFSEL6_Msk               
#define GPIO_AFRL_AFSEL7_Pos                  (28U)                             
#define GPIO_AFRL_AFSEL7_Msk                  (0xFU << GPIO_AFRL_AFSEL7_Pos)     /*!< 0xF0000000 */
#define GPIO_AFRL_AFSEL7                      GPIO_AFRL_AFSEL7_Msk               

/****************** Bit definition for GPIO_AFRH register  ********************/
#define GPIO_AFRH_AFSEL8_Pos                  (0U)                              
#define GPIO_AFRH_AFSEL8_Msk                  (0xFU << GPIO_AFRH_AFSEL8_Pos)     /*!< 0x0000000F */
#define GPIO_AFRH_AFSEL8                      GPIO_AFRH_AFSEL8_Msk               
#define GPIO_AFRH_AFSEL9_Pos                  (4U)                              
#define GPIO_AFRH_AFSEL9_Msk                  (0xFU << GPIO_AFRH_AFSEL9_Pos)     /*!< 0x000000F0 */
#define GPIO_AFRH_AFSEL9                      GPIO_AFRH_AFSEL9_Msk               
#define GPIO_AFRH_AFSEL10_Pos                  (8U)                              
#define GPIO_AFRH_AFSEL10_Msk                  (0xFU << GPIO_AFRH_AFSEL10_Pos)     /*!< 0x00000F00 */
#define GPIO_AFRH_AFSEL10                      GPIO_AFRH_AFSEL10_Msk               
#define GPIO_AFRH_AFSEL11_Pos                  (12U)                             
#define GPIO_AFRH_AFSEL11_Msk                  (0xFU << GPIO_AFRH_AFSEL11_Pos)     /*!< 0x0000F000 */
#define GPIO_AFRH_AFSEL11                      GPIO_AFRH_AFSEL11_Msk               
#define GPIO_AFRH_AFSEL12_Pos                  (16U)                             
#define GPIO_AFRH_AFSEL12_Msk                  (0xFU << GPIO_AFRH_AFSEL12_Pos)     /*!< 0x000F0000 */
#define GPIO_AFRH_AFSEL12                      GPIO_AFRH_AFSEL12_Msk               
#define GPIO_AFRH_AFSEL13_Pos                  (20U)                             
#define GPIO_AFRH_AFSEL13_Msk                  (0xFU << GPIO_AFRH_AFSEL13_Pos)     /*!< 0x00F00000 */
#define GPIO_AFRH_AFSEL13                      GPIO_AFRH_AFSEL13_Msk               
#define GPIO_AFRH_AFSEL14_Pos                  (24U)                             
#define GPIO_AFRH_AFSEL14_Msk                  (0xFU << GPIO_AFRH_AFSEL14_Pos)     /*!< 0x0F000000 */
#define GPIO_AFRH_AFSEL14                      GPIO_AFRH_AFSEL14_Msk               
#define GPIO_AFRH_AFSEL15_Pos                  (28U)                             
#define GPIO_AFRH_AFSEL15_Msk                  (0xFU << GPIO_AFRH_AFSEL15_Pos)     /*!< 0xF0000000 */
#define GPIO_AFRH_AFSEL15                      GPIO_AFRH_AFSEL15_Msk               

/****************** Bit definition for GPIO_BRR register  *********************/
#define GPIO_BRR_BR_0                        (0x00000001U)                     
#define GPIO_BRR_BR_1                        (0x00000002U)                     
#define GPIO_BRR_BR_2                        (0x00000004U)                     
#define GPIO_BRR_BR_3                        (0x00000008U)                     
#define GPIO_BRR_BR_4                        (0x00000010U)                     
#define GPIO_BRR_BR_5                        (0x00000020U)                     
#define GPIO_BRR_BR_6                        (0x00000040U)                     
#define GPIO_BRR_BR_7                        (0x00000080U)                     
#define GPIO_BRR_BR_8                        (0x00000100U)                     
#define GPIO_BRR_BR_9                        (0x00000200U)                     
#define GPIO_BRR_BR_10                       (0x00000400U)                     
#define GPIO_BRR_BR_11                       (0x00000800U)                     
#define GPIO_BRR_BR_12                       (0x00001000U)                     
#define GPIO_BRR_BR_13                       (0x00002000U)                     
#define GPIO_BRR_BR_14                       (0x00004000U)                     
#define GPIO_BRR_BR_15                       (0x00008000U)                     

/******************************************************************************/
/*                                                                            */
/*                   Inter-integrated Circuit Interface (I2C)                 */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for I2C_CR1 register  ********************/
#define I2C_CR1_PE_Pos                      (0U)                               
#define I2C_CR1_PE_Msk                      (0x1U << I2C_CR1_PE_Pos)           /*!< 0x00000001 */
#define I2C_CR1_PE                          I2C_CR1_PE_Msk                     /*!< Peripheral Enable */
#define I2C_CR1_SMBUS_Pos                   (1U)                               
#define I2C_CR1_SMBUS_Msk                   (0x1U << I2C_CR1_SMBUS_Pos)        /*!< 0x00000002 */
#define I2C_CR1_SMBUS                       I2C_CR1_SMBUS_Msk                  /*!< SMBus Mode */
#define I2C_CR1_SMBTYPE_Pos                 (3U)                               
#define I2C_CR1_SMBTYPE_Msk                 (0x1U << I2C_CR1_SMBTYPE_Pos)      /*!< 0x00000008 */
#define I2C_CR1_SMBTYPE                     I2C_CR1_SMBTYPE_Msk                /*!< SMBus Type */
#define I2C_CR1_ENARP_Pos                   (4U)                               
#define I2C_CR1_ENARP_Msk                   (0x1U << I2C_CR1_ENARP_Pos)        /*!< 0x00000010 */
#define I2C_CR1_ENARP                       I2C_CR1_ENARP_Msk                  /*!< ARP Enable */
#define I2C_CR1_ENPEC_Pos                   (5U)                               
#define I2C_CR1_ENPEC_Msk                   (0x1U << I2C_CR1_ENPEC_Pos)        /*!< 0x00000020 */
#define I2C_CR1_ENPEC                       I2C_CR1_ENPEC_Msk                  /*!< PEC Enable */
#define I2C_CR1_ENGC_Pos                    (6U)                               
#define I2C_CR1_ENGC_Msk                    (0x1U << I2C_CR1_ENGC_Pos)         /*!< 0x00000040 */
#define I2C_CR1_ENGC                        I2C_CR1_ENGC_Msk                   /*!< General Call Enable */
#define I2C_CR1_NOSTRETCH_Pos               (7U)                               
#define I2C_CR1_NOSTRETCH_Msk               (0x1U << I2C_CR1_NOSTRETCH_Pos)    /*!< 0x00000080 */
#define I2C_CR1_NOSTRETCH                   I2C_CR1_NOSTRETCH_Msk              /*!< Clock Stretching Disable (Slave mode) */
#define I2C_CR1_START_Pos                   (8U)                               
#define I2C_CR1_START_Msk                   (0x1U << I2C_CR1_START_Pos)        /*!< 0x00000100 */
#define I2C_CR1_START                       I2C_CR1_START_Msk                  /*!< Start Generation */
#define I2C_CR1_STOP_Pos                    (9U)                               
#define I2C_CR1_STOP_Msk                    (0x1U << I2C_CR1_STOP_Pos)         /*!< 0x00000200 */
#define I2C_CR1_STOP                        I2C_CR1_STOP_Msk                   /*!< Stop Generation */
#define I2C_CR1_ACK_Pos                     (10U)                              
#define I2C_CR1_ACK_Msk                     (0x1U << I2C_CR1_ACK_Pos)          /*!< 0x00000400 */
#define I2C_CR1_ACK                         I2C_CR1_ACK_Msk                    /*!< Acknowledge Enable */
#define I2C_CR1_POS_Pos                     (11U)                              
#define I2C_CR1_POS_Msk                     (0x1U << I2C_CR1_POS_Pos)          /*!< 0x00000800 */
#define I2C_CR1_POS                         I2C_CR1_POS_Msk                    /*!< Acknowledge/PEC Position (for data reception) */
#define I2C_CR1_PEC_Pos                     (12U)                              
#define I2C_CR1_PEC_Msk                     (0x1U << I2C_CR1_PEC_Pos)          /*!< 0x00001000 */
#define I2C_CR1_PEC                         I2C_CR1_PEC_Msk                    /*!< Packet Error Checking */
#define I2C_CR1_ALERT_Pos                   (13U)                              
#define I2C_CR1_ALERT_Msk                   (0x1U << I2C_CR1_ALERT_Pos)        /*!< 0x00002000 */
#define I2C_CR1_ALERT                       I2C_CR1_ALERT_Msk                  /*!< SMBus Alert */
#define I2C_CR1_SWRST_Pos                   (15U)                              
#define I2C_CR1_SWRST_Msk                   (0x1U << I2C_CR1_SWRST_Pos)        /*!< 0x00008000 */
#define I2C_CR1_SWRST                       I2C_CR1_SWRST_Msk                  /*!< Software Reset */

/*******************  Bit definition for I2C_CR2 register  ********************/
#define I2C_CR2_FREQ_Pos                    (0U)                               
#define I2C_CR2_FREQ_Msk                    (0x3FU << I2C_CR2_FREQ_Pos)        /*!< 0x0000003F */
#define I2C_CR2_FREQ                        I2C_CR2_FREQ_Msk                   /*!< FREQ[5:0] bits (Peripheral Clock Frequency) */
#define I2C_CR2_FREQ_0                      (0x01U << I2C_CR2_FREQ_Pos)        /*!< 0x00000001 */
#define I2C_CR2_FREQ_1                      (0x02U << I2C_CR2_FREQ_Pos)        /*!< 0x00000002 */
#define I2C_CR2_FREQ_2                      (0x04U << I2C_CR2_FREQ_Pos)        /*!< 0x00000004 */
#define I2C_CR2_FREQ_3                      (0x08U << I2C_CR2_FREQ_Pos)        /*!< 0x00000008 */
#define I2C_CR2_FREQ_4                      (0x10U << I2C_CR2_FREQ_Pos)        /*!< 0x00000010 */
#define I2C_CR2_FREQ_5                      (0x20U << I2C_CR2_FREQ_Pos)        /*!< 0x00000020 */

#define I2C_CR2_ITERREN_Pos                 (8U)                               
#define I2C_CR2_ITERREN_Msk                 (0x1U << I2C_CR2_ITERREN_Pos)      /*!< 0x00000100 */
#define I2C_CR2_ITERREN                     I2C_CR2_ITERREN_Msk                /*!< Error Interrupt Enable */
#define I2C_CR2_ITEVTEN_Pos                 (9U)                               
#define I2C_CR2_ITEVTEN_Msk                 (0x1U << I2C_CR2_ITEVTEN_Pos)      /*!< 0x00000200 */
#define I2C_CR2_ITEVTEN                     I2C_CR2_ITEVTEN_Msk                /*!< Event Interrupt Enable */
#define I2C_CR2_ITBUFEN_Pos                 (10U)                              
#define I2C_CR2_ITBUFEN_Msk                 (0x1U << I2C_CR2_ITBUFEN_Pos)      /*!< 0x00000400 */
#define I2C_CR2_ITBUFEN                     I2C_CR2_ITBUFEN_Msk                /*!< Buffer Interrupt Enable */
#define I2C_CR2_DMAEN_Pos                   (11U)                              
#define I2C_CR2_DMAEN_Msk                   (0x1U << I2C_CR2_DMAEN_Pos)        /*!< 0x00000800 */
#define I2C_CR2_DMAEN                       I2C_CR2_DMAEN_Msk                  /*!< DMA Requests Enable */
#define I2C_CR2_LAST_Pos                    (12U)                              
#define I2C_CR2_LAST_Msk                    (0x1U << I2C_CR2_LAST_Pos)         /*!< 0x00001000 */
#define I2C_CR2_LAST                        I2C_CR2_LAST_Msk                   /*!< DMA Last Transfer */

/*******************  Bit definition for I2C_OAR1 register  *******************/
#define I2C_OAR1_ADD1_7                     (0x000000FEU)                      /*!< Interface Address */
#define I2C_OAR1_ADD8_9                     (0x00000300U)                      /*!< Interface Address */

#define I2C_OAR1_ADD0_Pos                   (0U)                               
#define I2C_OAR1_ADD0_Msk                   (0x1U << I2C_OAR1_ADD0_Pos)        /*!< 0x00000001 */
#define I2C_OAR1_ADD0                       I2C_OAR1_ADD0_Msk                  /*!< Bit 0 */
#define I2C_OAR1_ADD1_Pos                   (1U)                               
#define I2C_OAR1_ADD1_Msk                   (0x1U << I2C_OAR1_ADD1_Pos)        /*!< 0x00000002 */
#define I2C_OAR1_ADD1                       I2C_OAR1_ADD1_Msk                  /*!< Bit 1 */
#define I2C_OAR1_ADD2_Pos                   (2U)                               
#define I2C_OAR1_ADD2_Msk                   (0x1U << I2C_OAR1_ADD2_Pos)        /*!< 0x00000004 */
#define I2C_OAR1_ADD2                       I2C_OAR1_ADD2_Msk                  /*!< Bit 2 */
#define I2C_OAR1_ADD3_Pos                   (3U)                               
#define I2C_OAR1_ADD3_Msk                   (0x1U << I2C_OAR1_ADD3_Pos)        /*!< 0x00000008 */
#define I2C_OAR1_ADD3                       I2C_OAR1_ADD3_Msk                  /*!< Bit 3 */
#define I2C_OAR1_ADD4_Pos                   (4U)                               
#define I2C_OAR1_ADD4_Msk                   (0x1U << I2C_OAR1_ADD4_Pos)        /*!< 0x00000010 */
#define I2C_OAR1_ADD4                       I2C_OAR1_ADD4_Msk                  /*!< Bit 4 */
#define I2C_OAR1_ADD5_Pos                   (5U)                               
#define I2C_OAR1_ADD5_Msk                   (0x1U << I2C_OAR1_ADD5_Pos)        /*!< 0x00000020 */
#define I2C_OAR1_ADD5                       I2C_OAR1_ADD5_Msk                  /*!< Bit 5 */
#define I2C_OAR1_ADD6_Pos                   (6U)                               
#define I2C_OAR1_ADD6_Msk                   (0x1U << I2C_OAR1_ADD6_Pos)        /*!< 0x00000040 */
#define I2C_OAR1_ADD6                       I2C_OAR1_ADD6_Msk                  /*!< Bit 6 */
#define I2C_OAR1_ADD7_Pos                   (7U)                               
#define I2C_OAR1_ADD7_Msk                   (0x1U << I2C_OAR1_ADD7_Pos)        /*!< 0x00000080 */
#define I2C_OAR1_ADD7                       I2C_OAR1_ADD7_Msk                  /*!< Bit 7 */
#define I2C_OAR1_ADD8_Pos                   (8U)                               
#define I2C_OAR1_ADD8_Msk                   (0x1U << I2C_OAR1_ADD8_Pos)        /*!< 0x00000100 */
#define I2C_OAR1_ADD8                       I2C_OAR1_ADD8_Msk                  /*!< Bit 8 */
#define I2C_OAR1_ADD9_Pos                   (9U)                               
#define I2C_OAR1_ADD9_Msk                   (0x1U << I2C_OAR1_ADD9_Pos)        /*!< 0x00000200 */
#define I2C_OAR1_ADD9                       I2C_OAR1_ADD9_Msk                  /*!< Bit 9 */

#define I2C_OAR1_ADDMODE_Pos                (15U)                              
#define I2C_OAR1_ADDMODE_Msk                (0x1U << I2C_OAR1_ADDMODE_Pos)     /*!< 0x00008000 */
#define I2C_OAR1_ADDMODE                    I2C_OAR1_ADDMODE_Msk               /*!< Addressing Mode (Slave mode) */

/*******************  Bit definition for I2C_OAR2 register  *******************/
#define I2C_OAR2_ENDUAL_Pos                 (0U)                               
#define I2C_OAR2_ENDUAL_Msk                 (0x1U << I2C_OAR2_ENDUAL_Pos)      /*!< 0x00000001 */
#define I2C_OAR2_ENDUAL                     I2C_OAR2_ENDUAL_Msk                /*!< Dual addressing mode enable */
#define I2C_OAR2_ADD2_Pos                   (1U)                               
#define I2C_OAR2_ADD2_Msk                   (0x7FU << I2C_OAR2_ADD2_Pos)       /*!< 0x000000FE */
#define I2C_OAR2_ADD2                       I2C_OAR2_ADD2_Msk                  /*!< Interface address */

/********************  Bit definition for I2C_DR register  ********************/
#define I2C_DR_DR_Pos                       (0U)                               
#define I2C_DR_DR_Msk                       (0xFFU << I2C_DR_DR_Pos)           /*!< 0x000000FF */
#define I2C_DR_DR                           I2C_DR_DR_Msk                      /*!< 8-bit Data Register */

/*******************  Bit definition for I2C_SR1 register  ********************/
#define I2C_SR1_SB_Pos                      (0U)                               
#define I2C_SR1_SB_Msk                      (0x1U << I2C_SR1_SB_Pos)           /*!< 0x00000001 */
#define I2C_SR1_SB                          I2C_SR1_SB_Msk                     /*!< Start Bit (Master mode) */
#define I2C_SR1_ADDR_Pos                    (1U)                               
#define I2C_SR1_ADDR_Msk                    (0x1U << I2C_SR1_ADDR_Pos)         /*!< 0x00000002 */
#define I2C_SR1_ADDR                        I2C_SR1_ADDR_Msk                   /*!< Address sent (master mode)/matched (slave mode) */
#define I2C_SR1_BTF_Pos                     (2U)                               
#define I2C_SR1_BTF_Msk                     (0x1U << I2C_SR1_BTF_Pos)          /*!< 0x00000004 */
#define I2C_SR1_BTF                         I2C_SR1_BTF_Msk                    /*!< Byte Transfer Finished */
#define I2C_SR1_ADD10_Pos                   (3U)                               
#define I2C_SR1_ADD10_Msk                   (0x1U << I2C_SR1_ADD10_Pos)        /*!< 0x00000008 */
#define I2C_SR1_ADD10                       I2C_SR1_ADD10_Msk                  /*!< 10-bit header sent (Master mode) */
#define I2C_SR1_STOPF_Pos                   (4U)                               
#define I2C_SR1_STOPF_Msk                   (0x1U << I2C_SR1_STOPF_Pos)        /*!< 0x00000010 */
#define I2C_SR1_STOPF                       I2C_SR1_STOPF_Msk                  /*!< Stop detection (Slave mode) */
#define I2C_SR1_RXNE_Pos                    (6U)                               
#define I2C_SR1_RXNE_Msk                    (0x1U << I2C_SR1_RXNE_Pos)         /*!< 0x00000040 */
#define I2C_SR1_RXNE                        I2C_SR1_RXNE_Msk                   /*!< Data Register not Empty (receivers) */
#define I2C_SR1_TXE_Pos                     (7U)                               
#define I2C_SR1_TXE_Msk                     (0x1U << I2C_SR1_TXE_Pos)          /*!< 0x00000080 */
#define I2C_SR1_TXE                         I2C_SR1_TXE_Msk                    /*!< Data Register Empty (transmitters) */
#define I2C_SR1_BERR_Pos                    (8U)                               
#define I2C_SR1_BERR_Msk                    (0x1U << I2C_SR1_BERR_Pos)         /*!< 0x00000100 */
#define I2C_SR1_BERR                        I2C_SR1_BERR_Msk                   /*!< Bus Error */
#define I2C_SR1_ARLO_Pos                    (9U)                               
#define I2C_SR1_ARLO_Msk                    (0x1U << I2C_SR1_ARLO_Pos)         /*!< 0x00000200 */
#define I2C_SR1_ARLO                        I2C_SR1_ARLO_Msk                   /*!< Arbitration Lost (master mode) */
#define I2C_SR1_AF_Pos                      (10U)                              
#define I2C_SR1_AF_Msk                      (0x1U << I2C_SR1_AF_Pos)           /*!< 0x00000400 */
#define I2C_SR1_AF                          I2C_SR1_AF_Msk                     /*!< Acknowledge Failure */
#define I2C_SR1_OVR_Pos                     (11U)                              
#define I2C_SR1_OVR_Msk                     (0x1U << I2C_SR1_OVR_Pos)          /*!< 0x00000800 */
#define I2C_SR1_OVR                         I2C_SR1_OVR_Msk                    /*!< Overrun/Underrun */
#define I2C_SR1_PECERR_Pos                  (12U)                              
#define I2C_SR1_PECERR_Msk                  (0x1U << I2C_SR1_PECERR_Pos)       /*!< 0x00001000 */
#define I2C_SR1_PECERR                      I2C_SR1_PECERR_Msk                 /*!< PEC Error in reception */
#define I2C_SR1_TIMEOUT_Pos                 (14U)                              
#define I2C_SR1_TIMEOUT_Msk                 (0x1U << I2C_SR1_TIMEOUT_Pos)      /*!< 0x00004000 */
#define I2C_SR1_TIMEOUT                     I2C_SR1_TIMEOUT_Msk                /*!< Timeout or Tlow Error */
#define I2C_SR1_SMBALERT_Pos                (15U)                              
#define I2C_SR1_SMBALERT_Msk                (0x1U << I2C_SR1_SMBALERT_Pos)     /*!< 0x00008000 */
#define I2C_SR1_SMBALERT                    I2C_SR1_SMBALERT_Msk               /*!< SMBus Alert */

/*******************  Bit definition for I2C_SR2 register  ********************/
#define I2C_SR2_MSL_Pos                     (0U)                               
#define I2C_SR2_MSL_Msk                     (0x1U << I2C_SR2_MSL_Pos)          /*!< 0x00000001 */
#define I2C_SR2_MSL                         I2C_SR2_MSL_Msk                    /*!< Master/Slave */
#define I2C_SR2_BUSY_Pos                    (1U)                               
#define I2C_SR2_BUSY_Msk                    (0x1U << I2C_SR2_BUSY_Pos)         /*!< 0x00000002 */
#define I2C_SR2_BUSY                        I2C_SR2_BUSY_Msk                   /*!< Bus Busy */
#define I2C_SR2_TRA_Pos                     (2U)                               
#define I2C_SR2_TRA_Msk                     (0x1U << I2C_SR2_TRA_Pos)          /*!< 0x00000004 */
#define I2C_SR2_TRA                         I2C_SR2_TRA_Msk                    /*!< Transmitter/Receiver */
#define I2C_SR2_GENCALL_Pos                 (4U)                               
#define I2C_SR2_GENCALL_Msk                 (0x1U << I2C_SR2_GENCALL_Pos)      /*!< 0x00000010 */
#define I2C_SR2_GENCALL                     I2C_SR2_GENCALL_Msk                /*!< General Call Address (Slave mode) */
#define I2C_SR2_SMBDEFAULT_Pos              (5U)                               
#define I2C_SR2_SMBDEFAULT_Msk              (0x1U << I2C_SR2_SMBDEFAULT_Pos)   /*!< 0x00000020 */
#define I2C_SR2_SMBDEFAULT                  I2C_SR2_SMBDEFAULT_Msk             /*!< SMBus Device Default Address (Slave mode) */
#define I2C_SR2_SMBHOST_Pos                 (6U)                               
#define I2C_SR2_SMBHOST_Msk                 (0x1U << I2C_SR2_SMBHOST_Pos)      /*!< 0x00000040 */
#define I2C_SR2_SMBHOST                     I2C_SR2_SMBHOST_Msk                /*!< SMBus Host Header (Slave mode) */
#define I2C_SR2_DUALF_Pos                   (7U)                               
#define I2C_SR2_DUALF_Msk                   (0x1U << I2C_SR2_DUALF_Pos)        /*!< 0x00000080 */
#define I2C_SR2_DUALF                       I2C_SR2_DUALF_Msk                  /*!< Dual Flag (Slave mode) */
#define I2C_SR2_PEC_Pos                     (8U)                               
#define I2C_SR2_PEC_Msk                     (0xFFU << I2C_SR2_PEC_Pos)         /*!< 0x0000FF00 */
#define I2C_SR2_PEC                         I2C_SR2_PEC_Msk                    /*!< Packet Error Checking Register */

/*******************  Bit definition for I2C_CCR register  ********************/
#define I2C_CCR_CCR_Pos                     (0U)                               
#define I2C_CCR_CCR_Msk                     (0xFFFU << I2C_CCR_CCR_Pos)        /*!< 0x00000FFF */
#define I2C_CCR_CCR                         I2C_CCR_CCR_Msk                    /*!< Clock Control Register in Fast/Standard mode (Master mode) */
#define I2C_CCR_DUTY_Pos                    (14U)                              
#define I2C_CCR_DUTY_Msk                    (0x1U << I2C_CCR_DUTY_Pos)         /*!< 0x00004000 */
#define I2C_CCR_DUTY                        I2C_CCR_DUTY_Msk                   /*!< Fast Mode Duty Cycle */
#define I2C_CCR_FS_Pos                      (15U)                              
#define I2C_CCR_FS_Msk                      (0x1U << I2C_CCR_FS_Pos)           /*!< 0x00008000 */
#define I2C_CCR_FS                          I2C_CCR_FS_Msk                     /*!< I2C Master Mode Selection */

/******************  Bit definition for I2C_TRISE register  *******************/
#define I2C_TRISE_TRISE_Pos                 (0U)                               
#define I2C_TRISE_TRISE_Msk                 (0x3FU << I2C_TRISE_TRISE_Pos)     /*!< 0x0000003F */
#define I2C_TRISE_TRISE                     I2C_TRISE_TRISE_Msk                /*!< Maximum Rise Time in Fast/Standard mode (Master mode) */

/******************************************************************************/
/*                                                                            */
/*                        Independent WATCHDOG (IWDG)                         */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for IWDG_KR register  ********************/
#define IWDG_KR_KEY_Pos                     (0U)                               
#define IWDG_KR_KEY_Msk                     (0xFFFFU << IWDG_KR_KEY_Pos)       /*!< 0x0000FFFF */
#define IWDG_KR_KEY                         IWDG_KR_KEY_Msk                    /*!< Key value (write only, read 0000h) */

/*******************  Bit definition for IWDG_PR register  ********************/
#define IWDG_PR_PR_Pos                      (0U)                               
#define IWDG_PR_PR_Msk                      (0x7U << IWDG_PR_PR_Pos)           /*!< 0x00000007 */
#define IWDG_PR_PR                          IWDG_PR_PR_Msk                     /*!< PR[2:0] (Prescaler divider) */
#define IWDG_PR_PR_0                        (0x1U << IWDG_PR_PR_Pos)           /*!< 0x00000001 */
#define IWDG_PR_PR_1                        (0x2U << IWDG_PR_PR_Pos)           /*!< 0x00000002 */
#define IWDG_PR_PR_2                        (0x4U << IWDG_PR_PR_Pos)           /*!< 0x00000004 */

/*******************  Bit definition for IWDG_RLR register  *******************/
#define IWDG_RLR_RL_Pos                     (0U)                               
#define IWDG_RLR_RL_Msk                     (0xFFFU << IWDG_RLR_RL_Pos)        /*!< 0x00000FFF */
#define IWDG_RLR_RL                         IWDG_RLR_RL_Msk                    /*!< Watchdog counter reload value */

/*******************  Bit definition for IWDG_SR register  ********************/
#define IWDG_SR_PVU_Pos                     (0U)                               
#define IWDG_SR_PVU_Msk                     (0x1U << IWDG_SR_PVU_Pos)          /*!< 0x00000001 */
#define IWDG_SR_PVU                         IWDG_SR_PVU_Msk                    /*!< Watchdog prescaler value update */
#define IWDG_SR_RVU_Pos                     (1U)                               
#define IWDG_SR_RVU_Msk                     (0x1U << IWDG_SR_RVU_Pos)          /*!< 0x00000002 */
#define IWDG_SR_RVU                         IWDG_SR_RVU_Msk                    /*!< Watchdog counter reload value update */

/******************************************************************************/
/*                                                                            */
/*                          LCD Controller (LCD)                              */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for LCD_CR register  *********************/
#define LCD_CR_LCDEN_Pos           (0U)                                        
#define LCD_CR_LCDEN_Msk           (0x1U << LCD_CR_LCDEN_Pos)                  /*!< 0x00000001 */
#define LCD_CR_LCDEN               LCD_CR_LCDEN_Msk                            /*!< LCD Enable Bit */
#define LCD_CR_VSEL_Pos            (1U)                                        
#define LCD_CR_VSEL_Msk            (0x1U << LCD_CR_VSEL_Pos)                   /*!< 0x00000002 */
#define LCD_CR_VSEL                LCD_CR_VSEL_Msk                             /*!< Voltage source selector Bit */

#define LCD_CR_DUTY_Pos            (2U)                                        
#define LCD_CR_DUTY_Msk            (0x7U << LCD_CR_DUTY_Pos)                   /*!< 0x0000001C */
#define LCD_CR_DUTY                LCD_CR_DUTY_Msk                             /*!< DUTY[2:0] bits (Duty selector) */
#define LCD_CR_DUTY_0              (0x1U << LCD_CR_DUTY_Pos)                   /*!< 0x00000004 */
#define LCD_CR_DUTY_1              (0x2U << LCD_CR_DUTY_Pos)                   /*!< 0x00000008 */
#define LCD_CR_DUTY_2              (0x4U << LCD_CR_DUTY_Pos)                   /*!< 0x00000010 */

#define LCD_CR_BIAS_Pos            (5U)                                        
#define LCD_CR_BIAS_Msk            (0x3U << LCD_CR_BIAS_Pos)                   /*!< 0x00000060 */
#define LCD_CR_BIAS                LCD_CR_BIAS_Msk                             /*!< BIAS[1:0] bits (Bias selector) */
#define LCD_CR_BIAS_0              (0x1U << LCD_CR_BIAS_Pos)                   /*!< 0x00000020 */
#define LCD_CR_BIAS_1              (0x2U << LCD_CR_BIAS_Pos)                   /*!< 0x00000040 */

#define LCD_CR_MUX_SEG_Pos         (7U)                                        
#define LCD_CR_MUX_SEG_Msk         (0x1U << LCD_CR_MUX_SEG_Pos)                /*!< 0x00000080 */
#define LCD_CR_MUX_SEG             LCD_CR_MUX_SEG_Msk                          /*!< Mux Segment Enable Bit */

/*******************  Bit definition for LCD_FCR register  ********************/
#define LCD_FCR_HD_Pos             (0U)                                        
#define LCD_FCR_HD_Msk             (0x1U << LCD_FCR_HD_Pos)                    /*!< 0x00000001 */
#define LCD_FCR_HD                 LCD_FCR_HD_Msk                              /*!< High Drive Enable Bit */
#define LCD_FCR_SOFIE_Pos          (1U)                                        
#define LCD_FCR_SOFIE_Msk          (0x1U << LCD_FCR_SOFIE_Pos)                 /*!< 0x00000002 */
#define LCD_FCR_SOFIE              LCD_FCR_SOFIE_Msk                           /*!< Start of Frame Interrupt Enable Bit */
#define LCD_FCR_UDDIE_Pos          (3U)                                        
#define LCD_FCR_UDDIE_Msk          (0x1U << LCD_FCR_UDDIE_Pos)                 /*!< 0x00000008 */
#define LCD_FCR_UDDIE              LCD_FCR_UDDIE_Msk                           /*!< Update Display Done Interrupt Enable Bit */

#define LCD_FCR_PON_Pos            (4U)                                        
#define LCD_FCR_PON_Msk            (0x7U << LCD_FCR_PON_Pos)                   /*!< 0x00000070 */
#define LCD_FCR_PON                LCD_FCR_PON_Msk                             /*!< PON[2:0] bits (Puls ON Duration) */
#define LCD_FCR_PON_0              (0x1U << LCD_FCR_PON_Pos)                   /*!< 0x00000010 */
#define LCD_FCR_PON_1              (0x2U << LCD_FCR_PON_Pos)                   /*!< 0x00000020 */
#define LCD_FCR_PON_2              (0x4U << LCD_FCR_PON_Pos)                   /*!< 0x00000040 */

#define LCD_FCR_DEAD_Pos           (7U)                                        
#define LCD_FCR_DEAD_Msk           (0x7U << LCD_FCR_DEAD_Pos)                  /*!< 0x00000380 */
#define LCD_FCR_DEAD               LCD_FCR_DEAD_Msk                            /*!< DEAD[2:0] bits (DEAD Time) */
#define LCD_FCR_DEAD_0             (0x1U << LCD_FCR_DEAD_Pos)                  /*!< 0x00000080 */
#define LCD_FCR_DEAD_1             (0x2U << LCD_FCR_DEAD_Pos)                  /*!< 0x00000100 */
#define LCD_FCR_DEAD_2             (0x4U << LCD_FCR_DEAD_Pos)                  /*!< 0x00000200 */

#define LCD_FCR_CC_Pos             (10U)                                       
#define LCD_FCR_CC_Msk             (0x7U << LCD_FCR_CC_Pos)                    /*!< 0x00001C00 */
#define LCD_FCR_CC                 LCD_FCR_CC_Msk                              /*!< CC[2:0] bits (Contrast Control) */
#define LCD_FCR_CC_0               (0x1U << LCD_FCR_CC_Pos)                    /*!< 0x00000400 */
#define LCD_FCR_CC_1               (0x2U << LCD_FCR_CC_Pos)                    /*!< 0x00000800 */
#define LCD_FCR_CC_2               (0x4U << LCD_FCR_CC_Pos)                    /*!< 0x00001000 */

#define LCD_FCR_BLINKF_Pos         (13U)                                       
#define LCD_FCR_BLINKF_Msk         (0x7U << LCD_FCR_BLINKF_Pos)                /*!< 0x0000E000 */
#define LCD_FCR_BLINKF             LCD_FCR_BLINKF_Msk                          /*!< BLINKF[2:0] bits (Blink Frequency) */
#define LCD_FCR_BLINKF_0           (0x1U << LCD_FCR_BLINKF_Pos)                /*!< 0x00002000 */
#define LCD_FCR_BLINKF_1           (0x2U << LCD_FCR_BLINKF_Pos)                /*!< 0x00004000 */
#define LCD_FCR_BLINKF_2           (0x4U << LCD_FCR_BLINKF_Pos)                /*!< 0x00008000 */

#define LCD_FCR_BLINK_Pos          (16U)                                       
#define LCD_FCR_BLINK_Msk          (0x3U << LCD_FCR_BLINK_Pos)                 /*!< 0x00030000 */
#define LCD_FCR_BLINK              LCD_FCR_BLINK_Msk                           /*!< BLINK[1:0] bits (Blink Enable) */
#define LCD_FCR_BLINK_0            (0x1U << LCD_FCR_BLINK_Pos)                 /*!< 0x00010000 */
#define LCD_FCR_BLINK_1            (0x2U << LCD_FCR_BLINK_Pos)                 /*!< 0x00020000 */

#define LCD_FCR_DIV_Pos            (18U)                                       
#define LCD_FCR_DIV_Msk            (0xFU << LCD_FCR_DIV_Pos)                   /*!< 0x003C0000 */
#define LCD_FCR_DIV                LCD_FCR_DIV_Msk                             /*!< DIV[3:0] bits (Divider) */
#define LCD_FCR_PS_Pos             (22U)                                       
#define LCD_FCR_PS_Msk             (0xFU << LCD_FCR_PS_Pos)                    /*!< 0x03C00000 */
#define LCD_FCR_PS                 LCD_FCR_PS_Msk                              /*!< PS[3:0] bits (Prescaler) */

/*******************  Bit definition for LCD_SR register  *********************/
#define LCD_SR_ENS_Pos             (0U)                                        
#define LCD_SR_ENS_Msk             (0x1U << LCD_SR_ENS_Pos)                    /*!< 0x00000001 */
#define LCD_SR_ENS                 LCD_SR_ENS_Msk                              /*!< LCD Enabled Bit */
#define LCD_SR_SOF_Pos             (1U)                                        
#define LCD_SR_SOF_Msk             (0x1U << LCD_SR_SOF_Pos)                    /*!< 0x00000002 */
#define LCD_SR_SOF                 LCD_SR_SOF_Msk                              /*!< Start Of Frame Flag Bit */
#define LCD_SR_UDR_Pos             (2U)                                        
#define LCD_SR_UDR_Msk             (0x1U << LCD_SR_UDR_Pos)                    /*!< 0x00000004 */
#define LCD_SR_UDR                 LCD_SR_UDR_Msk                              /*!< Update Display Request Bit */
#define LCD_SR_UDD_Pos             (3U)                                        
#define LCD_SR_UDD_Msk             (0x1U << LCD_SR_UDD_Pos)                    /*!< 0x00000008 */
#define LCD_SR_UDD                 LCD_SR_UDD_Msk                              /*!< Update Display Done Flag Bit */
#define LCD_SR_RDY_Pos             (4U)                                        
#define LCD_SR_RDY_Msk             (0x1U << LCD_SR_RDY_Pos)                    /*!< 0x00000010 */
#define LCD_SR_RDY                 LCD_SR_RDY_Msk                              /*!< Ready Flag Bit */
#define LCD_SR_FCRSR_Pos           (5U)                                        
#define LCD_SR_FCRSR_Msk           (0x1U << LCD_SR_FCRSR_Pos)                  /*!< 0x00000020 */
#define LCD_SR_FCRSR               LCD_SR_FCRSR_Msk                            /*!< LCD FCR Register Synchronization Flag Bit */

/*******************  Bit definition for LCD_CLR register  ********************/
#define LCD_CLR_SOFC_Pos           (1U)                                        
#define LCD_CLR_SOFC_Msk           (0x1U << LCD_CLR_SOFC_Pos)                  /*!< 0x00000002 */
#define LCD_CLR_SOFC               LCD_CLR_SOFC_Msk                            /*!< Start Of Frame Flag Clear Bit */
#define LCD_CLR_UDDC_Pos           (3U)                                        
#define LCD_CLR_UDDC_Msk           (0x1U << LCD_CLR_UDDC_Pos)                  /*!< 0x00000008 */
#define LCD_CLR_UDDC               LCD_CLR_UDDC_Msk                            /*!< Update Display Done Flag Clear Bit */

/*******************  Bit definition for LCD_RAM register  ********************/
#define LCD_RAM_SEGMENT_DATA_Pos   (0U)                                        
#define LCD_RAM_SEGMENT_DATA_Msk   (0xFFFFFFFFU << LCD_RAM_SEGMENT_DATA_Pos)   /*!< 0xFFFFFFFF */
#define LCD_RAM_SEGMENT_DATA       LCD_RAM_SEGMENT_DATA_Msk                    /*!< Segment Data Bits */

/******************************************************************************/
/*                                                                            */
/*                          Power Control (PWR)                               */
/*                                                                            */
/******************************************************************************/

#define PWR_PVD_SUPPORT                       /*!< PWR feature available only on specific devices: Power Voltage Detection feature */

/********************  Bit definition for PWR_CR register  ********************/
#define PWR_CR_LPSDSR_Pos                   (0U)                               
#define PWR_CR_LPSDSR_Msk                   (0x1U << PWR_CR_LPSDSR_Pos)        /*!< 0x00000001 */
#define PWR_CR_LPSDSR                       PWR_CR_LPSDSR_Msk                  /*!< Low-power deepsleep/sleep/low power run */
#define PWR_CR_PDDS_Pos                     (1U)                               
#define PWR_CR_PDDS_Msk                     (0x1U << PWR_CR_PDDS_Pos)          /*!< 0x00000002 */
#define PWR_CR_PDDS                         PWR_CR_PDDS_Msk                    /*!< Power Down Deepsleep */
#define PWR_CR_CWUF_Pos                     (2U)                               
#define PWR_CR_CWUF_Msk                     (0x1U << PWR_CR_CWUF_Pos)          /*!< 0x00000004 */
#define PWR_CR_CWUF                         PWR_CR_CWUF_Msk                    /*!< Clear Wakeup Flag */
#define PWR_CR_CSBF_Pos                     (3U)                               
#define PWR_CR_CSBF_Msk                     (0x1U << PWR_CR_CSBF_Pos)          /*!< 0x00000008 */
#define PWR_CR_CSBF                         PWR_CR_CSBF_Msk                    /*!< Clear Standby Flag */
#define PWR_CR_PVDE_Pos                     (4U)                               
#define PWR_CR_PVDE_Msk                     (0x1U << PWR_CR_PVDE_Pos)          /*!< 0x00000010 */
#define PWR_CR_PVDE                         PWR_CR_PVDE_Msk                    /*!< Power Voltage Detector Enable */

#define PWR_CR_PLS_Pos                      (5U)                               
#define PWR_CR_PLS_Msk                      (0x7U << PWR_CR_PLS_Pos)           /*!< 0x000000E0 */
#define PWR_CR_PLS                          PWR_CR_PLS_Msk                     /*!< PLS[2:0] bits (PVD Level Selection) */
#define PWR_CR_PLS_0                        (0x1U << PWR_CR_PLS_Pos)           /*!< 0x00000020 */
#define PWR_CR_PLS_1                        (0x2U << PWR_CR_PLS_Pos)           /*!< 0x00000040 */
#define PWR_CR_PLS_2                        (0x4U << PWR_CR_PLS_Pos)           /*!< 0x00000080 */

/*!< PVD level configuration */
#define PWR_CR_PLS_LEV0                     (0x00000000U)                      /*!< PVD level 0 */
#define PWR_CR_PLS_LEV1                     (0x00000020U)                      /*!< PVD level 1 */
#define PWR_CR_PLS_LEV2                     (0x00000040U)                      /*!< PVD level 2 */
#define PWR_CR_PLS_LEV3                     (0x00000060U)                      /*!< PVD level 3 */
#define PWR_CR_PLS_LEV4                     (0x00000080U)                      /*!< PVD level 4 */
#define PWR_CR_PLS_LEV5                     (0x000000A0U)                      /*!< PVD level 5 */
#define PWR_CR_PLS_LEV6                     (0x000000C0U)                      /*!< PVD level 6 */
#define PWR_CR_PLS_LEV7                     (0x000000E0U)                      /*!< PVD level 7 */

#define PWR_CR_DBP_Pos                      (8U)                               
#define PWR_CR_DBP_Msk                      (0x1U << PWR_CR_DBP_Pos)           /*!< 0x00000100 */
#define PWR_CR_DBP                          PWR_CR_DBP_Msk                     /*!< Disable Backup Domain write protection */
#define PWR_CR_ULP_Pos                      (9U)                               
#define PWR_CR_ULP_Msk                      (0x1U << PWR_CR_ULP_Pos)           /*!< 0x00000200 */
#define PWR_CR_ULP                          PWR_CR_ULP_Msk                     /*!< Ultra Low Power mode */
#define PWR_CR_FWU_Pos                      (10U)                              
#define PWR_CR_FWU_Msk                      (0x1U << PWR_CR_FWU_Pos)           /*!< 0x00000400 */
#define PWR_CR_FWU                          PWR_CR_FWU_Msk                     /*!< Fast wakeup */

#define PWR_CR_VOS_Pos                      (11U)                              
#define PWR_CR_VOS_Msk                      (0x3U << PWR_CR_VOS_Pos)           /*!< 0x00001800 */
#define PWR_CR_VOS                          PWR_CR_VOS_Msk                     /*!< VOS[1:0] bits (Voltage scaling range selection) */
#define PWR_CR_VOS_0                        (0x1U << PWR_CR_VOS_Pos)           /*!< 0x00000800 */
#define PWR_CR_VOS_1                        (0x2U << PWR_CR_VOS_Pos)           /*!< 0x00001000 */
#define PWR_CR_LPRUN_Pos                    (14U)                              
#define PWR_CR_LPRUN_Msk                    (0x1U << PWR_CR_LPRUN_Pos)         /*!< 0x00004000 */
#define PWR_CR_LPRUN                        PWR_CR_LPRUN_Msk                   /*!< Low power run mode */

/*******************  Bit definition for PWR_CSR register  ********************/
#define PWR_CSR_WUF_Pos                     (0U)                               
#define PWR_CSR_WUF_Msk                     (0x1U << PWR_CSR_WUF_Pos)          /*!< 0x00000001 */
#define PWR_CSR_WUF                         PWR_CSR_WUF_Msk                    /*!< Wakeup Flag */
#define PWR_CSR_SBF_Pos                     (1U)                               
#define PWR_CSR_SBF_Msk                     (0x1U << PWR_CSR_SBF_Pos)          /*!< 0x00000002 */
#define PWR_CSR_SBF                         PWR_CSR_SBF_Msk                    /*!< Standby Flag */
#define PWR_CSR_PVDO_Pos                    (2U)                               
#define PWR_CSR_PVDO_Msk                    (0x1U << PWR_CSR_PVDO_Pos)         /*!< 0x00000004 */
#define PWR_CSR_PVDO                        PWR_CSR_PVDO_Msk                   /*!< PVD Output */
#define PWR_CSR_VREFINTRDYF_Pos             (3U)                               
#define PWR_CSR_VREFINTRDYF_Msk             (0x1U << PWR_CSR_VREFINTRDYF_Pos)  /*!< 0x00000008 */
#define PWR_CSR_VREFINTRDYF                 PWR_CSR_VREFINTRDYF_Msk            /*!< Internal voltage reference (VREFINT) ready flag */
#define PWR_CSR_VOSF_Pos                    (4U)                               
#define PWR_CSR_VOSF_Msk                    (0x1U << PWR_CSR_VOSF_Pos)         /*!< 0x00000010 */
#define PWR_CSR_VOSF                        PWR_CSR_VOSF_Msk                   /*!< Voltage Scaling select flag */
#define PWR_CSR_REGLPF_Pos                  (5U)                               
#define PWR_CSR_REGLPF_Msk                  (0x1U << PWR_CSR_REGLPF_Pos)       /*!< 0x00000020 */
#define PWR_CSR_REGLPF                      PWR_CSR_REGLPF_Msk                 /*!< Regulator LP flag */

#define PWR_CSR_EWUP1_Pos                   (8U)                               
#define PWR_CSR_EWUP1_Msk                   (0x1U << PWR_CSR_EWUP1_Pos)        /*!< 0x00000100 */
#define PWR_CSR_EWUP1                       PWR_CSR_EWUP1_Msk                  /*!< Enable WKUP pin 1 */
#define PWR_CSR_EWUP2_Pos                   (9U)                               
#define PWR_CSR_EWUP2_Msk                   (0x1U << PWR_CSR_EWUP2_Pos)        /*!< 0x00000200 */
#define PWR_CSR_EWUP2                       PWR_CSR_EWUP2_Msk                  /*!< Enable WKUP pin 2 */
#define PWR_CSR_EWUP3_Pos                   (10U)                              
#define PWR_CSR_EWUP3_Msk                   (0x1U << PWR_CSR_EWUP3_Pos)        /*!< 0x00000400 */
#define PWR_CSR_EWUP3                       PWR_CSR_EWUP3_Msk                  /*!< Enable WKUP pin 3 */

/******************************************************************************/
/*                                                                            */
/*                      Reset and Clock Control (RCC)                         */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions  (not present on all devices in the STM32F0 serie)
*/
#define RCC_LSECSS_SUPPORT          /*!< LSE CSS feature support */

/********************  Bit definition for RCC_CR register  ********************/
#define RCC_CR_HSION_Pos                    (0U)                               
#define RCC_CR_HSION_Msk                    (0x1U << RCC_CR_HSION_Pos)         /*!< 0x00000001 */
#define RCC_CR_HSION                        RCC_CR_HSION_Msk                   /*!< Internal High Speed clock enable */
#define RCC_CR_HSIRDY_Pos                   (1U)                               
#define RCC_CR_HSIRDY_Msk                   (0x1U << RCC_CR_HSIRDY_Pos)        /*!< 0x00000002 */
#define RCC_CR_HSIRDY                       RCC_CR_HSIRDY_Msk                  /*!< Internal High Speed clock ready flag */

#define RCC_CR_MSION_Pos                    (8U)                               
#define RCC_CR_MSION_Msk                    (0x1U << RCC_CR_MSION_Pos)         /*!< 0x00000100 */
#define RCC_CR_MSION                        RCC_CR_MSION_Msk                   /*!< Internal Multi Speed clock enable */
#define RCC_CR_MSIRDY_Pos                   (9U)                               
#define RCC_CR_MSIRDY_Msk                   (0x1U << RCC_CR_MSIRDY_Pos)        /*!< 0x00000200 */
#define RCC_CR_MSIRDY                       RCC_CR_MSIRDY_Msk                  /*!< Internal Multi Speed clock ready flag */

#define RCC_CR_HSEON_Pos                    (16U)                              
#define RCC_CR_HSEON_Msk                    (0x1U << RCC_CR_HSEON_Pos)         /*!< 0x00010000 */
#define RCC_CR_HSEON                        RCC_CR_HSEON_Msk                   /*!< External High Speed clock enable */
#define RCC_CR_HSERDY_Pos                   (17U)                              
#define RCC_CR_HSERDY_Msk                   (0x1U << RCC_CR_HSERDY_Pos)        /*!< 0x00020000 */
#define RCC_CR_HSERDY                       RCC_CR_HSERDY_Msk                  /*!< External High Speed clock ready flag */
#define RCC_CR_HSEBYP_Pos                   (18U)                              
#define RCC_CR_HSEBYP_Msk                   (0x1U << RCC_CR_HSEBYP_Pos)        /*!< 0x00040000 */
#define RCC_CR_HSEBYP                       RCC_CR_HSEBYP_Msk                  /*!< External High Speed clock Bypass */

#define RCC_CR_PLLON_Pos                    (24U)                              
#define RCC_CR_PLLON_Msk                    (0x1U << RCC_CR_PLLON_Pos)         /*!< 0x01000000 */
#define RCC_CR_PLLON                        RCC_CR_PLLON_Msk                   /*!< PLL enable */
#define RCC_CR_PLLRDY_Pos                   (25U)                              
#define RCC_CR_PLLRDY_Msk                   (0x1U << RCC_CR_PLLRDY_Pos)        /*!< 0x02000000 */
#define RCC_CR_PLLRDY                       RCC_CR_PLLRDY_Msk                  /*!< PLL clock ready flag */
#define RCC_CR_CSSON_Pos                    (28U)                              
#define RCC_CR_CSSON_Msk                    (0x1U << RCC_CR_CSSON_Pos)         /*!< 0x10000000 */
#define RCC_CR_CSSON                        RCC_CR_CSSON_Msk                   /*!< Clock Security System enable */

#define RCC_CR_RTCPRE_Pos                   (29U)                              
#define RCC_CR_RTCPRE_Msk                   (0x3U << RCC_CR_RTCPRE_Pos)        /*!< 0x60000000 */
#define RCC_CR_RTCPRE                       RCC_CR_RTCPRE_Msk                  /*!< RTC/LCD Prescaler */
#define RCC_CR_RTCPRE_0                     (0x20000000U)                      /*!< Bit0 */
#define RCC_CR_RTCPRE_1                     (0x40000000U)                      /*!< Bit1 */

/********************  Bit definition for RCC_ICSCR register  *****************/
#define RCC_ICSCR_HSICAL_Pos                (0U)                               
#define RCC_ICSCR_HSICAL_Msk                (0xFFU << RCC_ICSCR_HSICAL_Pos)    /*!< 0x000000FF */
#define RCC_ICSCR_HSICAL                    RCC_ICSCR_HSICAL_Msk               /*!< Internal High Speed clock Calibration */
#define RCC_ICSCR_HSITRIM_Pos               (8U)                               
#define RCC_ICSCR_HSITRIM_Msk               (0x1FU << RCC_ICSCR_HSITRIM_Pos)   /*!< 0x00001F00 */
#define RCC_ICSCR_HSITRIM                   RCC_ICSCR_HSITRIM_Msk              /*!< Internal High Speed clock trimming */

#define RCC_ICSCR_MSIRANGE_Pos              (13U)                              
#define RCC_ICSCR_MSIRANGE_Msk              (0x7U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x0000E000 */
#define RCC_ICSCR_MSIRANGE                  RCC_ICSCR_MSIRANGE_Msk             /*!< Internal Multi Speed clock Range */
#define RCC_ICSCR_MSIRANGE_0                (0x0U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x00000000 */
#define RCC_ICSCR_MSIRANGE_1                (0x1U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x00002000 */
#define RCC_ICSCR_MSIRANGE_2                (0x2U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x00004000 */
#define RCC_ICSCR_MSIRANGE_3                (0x3U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x00006000 */
#define RCC_ICSCR_MSIRANGE_4                (0x4U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x00008000 */
#define RCC_ICSCR_MSIRANGE_5                (0x5U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x0000A000 */
#define RCC_ICSCR_MSIRANGE_6                (0x6U << RCC_ICSCR_MSIRANGE_Pos)   /*!< 0x0000C000 */
#define RCC_ICSCR_MSICAL_Pos                (16U)                              
#define RCC_ICSCR_MSICAL_Msk                (0xFFU << RCC_ICSCR_MSICAL_Pos)    /*!< 0x00FF0000 */
#define RCC_ICSCR_MSICAL                    RCC_ICSCR_MSICAL_Msk               /*!< Internal Multi Speed clock Calibration */
#define RCC_ICSCR_MSITRIM_Pos               (24U)                              
#define RCC_ICSCR_MSITRIM_Msk               (0xFFU << RCC_ICSCR_MSITRIM_Pos)   /*!< 0xFF000000 */
#define RCC_ICSCR_MSITRIM                   RCC_ICSCR_MSITRIM_Msk              /*!< Internal Multi Speed clock trimming */

/********************  Bit definition for RCC_CFGR register  ******************/
#define RCC_CFGR_SW_Pos                     (0U)                               
#define RCC_CFGR_SW_Msk                     (0x3U << RCC_CFGR_SW_Pos)          /*!< 0x00000003 */
#define RCC_CFGR_SW                         RCC_CFGR_SW_Msk                    /*!< SW[1:0] bits (System clock Switch) */
#define RCC_CFGR_SW_0                       (0x1U << RCC_CFGR_SW_Pos)          /*!< 0x00000001 */
#define RCC_CFGR_SW_1                       (0x2U << RCC_CFGR_SW_Pos)          /*!< 0x00000002 */

/*!< SW configuration */
#define RCC_CFGR_SW_MSI                     (0x00000000U)                      /*!< MSI selected as system clock */
#define RCC_CFGR_SW_HSI                     (0x00000001U)                      /*!< HSI selected as system clock */
#define RCC_CFGR_SW_HSE                     (0x00000002U)                      /*!< HSE selected as system clock */
#define RCC_CFGR_SW_PLL                     (0x00000003U)                      /*!< PLL selected as system clock */

#define RCC_CFGR_SWS_Pos                    (2U)                               
#define RCC_CFGR_SWS_Msk                    (0x3U << RCC_CFGR_SWS_Pos)         /*!< 0x0000000C */
#define RCC_CFGR_SWS                        RCC_CFGR_SWS_Msk                   /*!< SWS[1:0] bits (System Clock Switch Status) */
#define RCC_CFGR_SWS_0                      (0x1U << RCC_CFGR_SWS_Pos)         /*!< 0x00000004 */
#define RCC_CFGR_SWS_1                      (0x2U << RCC_CFGR_SWS_Pos)         /*!< 0x00000008 */

/*!< SWS configuration */
#define RCC_CFGR_SWS_MSI                    (0x00000000U)                      /*!< MSI oscillator used as system clock */
#define RCC_CFGR_SWS_HSI                    (0x00000004U)                      /*!< HSI oscillator used as system clock */
#define RCC_CFGR_SWS_HSE                    (0x00000008U)                      /*!< HSE oscillator used as system clock */
#define RCC_CFGR_SWS_PLL                    (0x0000000CU)                      /*!< PLL used as system clock */

#define RCC_CFGR_HPRE_Pos                   (4U)                               
#define RCC_CFGR_HPRE_Msk                   (0xFU << RCC_CFGR_HPRE_Pos)        /*!< 0x000000F0 */
#define RCC_CFGR_HPRE                       RCC_CFGR_HPRE_Msk                  /*!< HPRE[3:0] bits (AHB prescaler) */
#define RCC_CFGR_HPRE_0                     (0x1U << RCC_CFGR_HPRE_Pos)        /*!< 0x00000010 */
#define RCC_CFGR_HPRE_1                     (0x2U << RCC_CFGR_HPRE_Pos)        /*!< 0x00000020 */
#define RCC_CFGR_HPRE_2                     (0x4U << RCC_CFGR_HPRE_Pos)        /*!< 0x00000040 */
#define RCC_CFGR_HPRE_3                     (0x8U << RCC_CFGR_HPRE_Pos)        /*!< 0x00000080 */

/*!< HPRE configuration */
#define RCC_CFGR_HPRE_DIV1                  (0x00000000U)                      /*!< SYSCLK not divided */
#define RCC_CFGR_HPRE_DIV2                  (0x00000080U)                      /*!< SYSCLK divided by 2 */
#define RCC_CFGR_HPRE_DIV4                  (0x00000090U)                      /*!< SYSCLK divided by 4 */
#define RCC_CFGR_HPRE_DIV8                  (0x000000A0U)                      /*!< SYSCLK divided by 8 */
#define RCC_CFGR_HPRE_DIV16                 (0x000000B0U)                      /*!< SYSCLK divided by 16 */
#define RCC_CFGR_HPRE_DIV64                 (0x000000C0U)                      /*!< SYSCLK divided by 64 */
#define RCC_CFGR_HPRE_DIV128                (0x000000D0U)                      /*!< SYSCLK divided by 128 */
#define RCC_CFGR_HPRE_DIV256                (0x000000E0U)                      /*!< SYSCLK divided by 256 */
#define RCC_CFGR_HPRE_DIV512                (0x000000F0U)                      /*!< SYSCLK divided by 512 */

#define RCC_CFGR_PPRE1_Pos                  (8U)                               
#define RCC_CFGR_PPRE1_Msk                  (0x7U << RCC_CFGR_PPRE1_Pos)       /*!< 0x00000700 */
#define RCC_CFGR_PPRE1                      RCC_CFGR_PPRE1_Msk                 /*!< PRE1[2:0] bits (APB1 prescaler) */
#define RCC_CFGR_PPRE1_0                    (0x1U << RCC_CFGR_PPRE1_Pos)       /*!< 0x00000100 */
#define RCC_CFGR_PPRE1_1                    (0x2U << RCC_CFGR_PPRE1_Pos)       /*!< 0x00000200 */
#define RCC_CFGR_PPRE1_2                    (0x4U << RCC_CFGR_PPRE1_Pos)       /*!< 0x00000400 */

/*!< PPRE1 configuration */
#define RCC_CFGR_PPRE1_DIV1                 (0x00000000U)                      /*!< HCLK not divided */
#define RCC_CFGR_PPRE1_DIV2                 (0x00000400U)                      /*!< HCLK divided by 2 */
#define RCC_CFGR_PPRE1_DIV4                 (0x00000500U)                      /*!< HCLK divided by 4 */
#define RCC_CFGR_PPRE1_DIV8                 (0x00000600U)                      /*!< HCLK divided by 8 */
#define RCC_CFGR_PPRE1_DIV16                (0x00000700U)                      /*!< HCLK divided by 16 */

#define RCC_CFGR_PPRE2_Pos                  (11U)                              
#define RCC_CFGR_PPRE2_Msk                  (0x7U << RCC_CFGR_PPRE2_Pos)       /*!< 0x00003800 */
#define RCC_CFGR_PPRE2                      RCC_CFGR_PPRE2_Msk                 /*!< PRE2[2:0] bits (APB2 prescaler) */
#define RCC_CFGR_PPRE2_0                    (0x1U << RCC_CFGR_PPRE2_Pos)       /*!< 0x00000800 */
#define RCC_CFGR_PPRE2_1                    (0x2U << RCC_CFGR_PPRE2_Pos)       /*!< 0x00001000 */
#define RCC_CFGR_PPRE2_2                    (0x4U << RCC_CFGR_PPRE2_Pos)       /*!< 0x00002000 */

/*!< PPRE2 configuration */
#define RCC_CFGR_PPRE2_DIV1                 (0x00000000U)                      /*!< HCLK not divided */
#define RCC_CFGR_PPRE2_DIV2                 (0x00002000U)                      /*!< HCLK divided by 2 */
#define RCC_CFGR_PPRE2_DIV4                 (0x00002800U)                      /*!< HCLK divided by 4 */
#define RCC_CFGR_PPRE2_DIV8                 (0x00003000U)                      /*!< HCLK divided by 8 */
#define RCC_CFGR_PPRE2_DIV16                (0x00003800U)                      /*!< HCLK divided by 16 */

/*!< PLL entry clock source*/
#define RCC_CFGR_PLLSRC_Pos                 (16U)                              
#define RCC_CFGR_PLLSRC_Msk                 (0x1U << RCC_CFGR_PLLSRC_Pos)      /*!< 0x00010000 */
#define RCC_CFGR_PLLSRC                     RCC_CFGR_PLLSRC_Msk                /*!< PLL entry clock source */

#define RCC_CFGR_PLLSRC_HSI                 (0x00000000U)                      /*!< HSI as PLL entry clock source */
#define RCC_CFGR_PLLSRC_HSE                 (0x00010000U)                      /*!< HSE as PLL entry clock source */


/*!< PLLMUL configuration */
#define RCC_CFGR_PLLMUL_Pos                 (18U)                              
#define RCC_CFGR_PLLMUL_Msk                 (0xFU << RCC_CFGR_PLLMUL_Pos)      /*!< 0x003C0000 */
#define RCC_CFGR_PLLMUL                     RCC_CFGR_PLLMUL_Msk                /*!< PLLMUL[3:0] bits (PLL multiplication factor) */
#define RCC_CFGR_PLLMUL_0                   (0x1U << RCC_CFGR_PLLMUL_Pos)      /*!< 0x00040000 */
#define RCC_CFGR_PLLMUL_1                   (0x2U << RCC_CFGR_PLLMUL_Pos)      /*!< 0x00080000 */
#define RCC_CFGR_PLLMUL_2                   (0x4U << RCC_CFGR_PLLMUL_Pos)      /*!< 0x00100000 */
#define RCC_CFGR_PLLMUL_3                   (0x8U << RCC_CFGR_PLLMUL_Pos)      /*!< 0x00200000 */

/*!< PLLMUL configuration */
#define RCC_CFGR_PLLMUL3                    (0x00000000U)                      /*!< PLL input clock * 3 */
#define RCC_CFGR_PLLMUL4                    (0x00040000U)                      /*!< PLL input clock * 4 */
#define RCC_CFGR_PLLMUL6                    (0x00080000U)                      /*!< PLL input clock * 6 */
#define RCC_CFGR_PLLMUL8                    (0x000C0000U)                      /*!< PLL input clock * 8 */
#define RCC_CFGR_PLLMUL12                   (0x00100000U)                      /*!< PLL input clock * 12 */
#define RCC_CFGR_PLLMUL16                   (0x00140000U)                      /*!< PLL input clock * 16 */
#define RCC_CFGR_PLLMUL24                   (0x00180000U)                      /*!< PLL input clock * 24 */
#define RCC_CFGR_PLLMUL32                   (0x001C0000U)                      /*!< PLL input clock * 32 */
#define RCC_CFGR_PLLMUL48                   (0x00200000U)                      /*!< PLL input clock * 48 */

/*!< PLLDIV configuration */
#define RCC_CFGR_PLLDIV_Pos                 (22U)                              
#define RCC_CFGR_PLLDIV_Msk                 (0x3U << RCC_CFGR_PLLDIV_Pos)      /*!< 0x00C00000 */
#define RCC_CFGR_PLLDIV                     RCC_CFGR_PLLDIV_Msk                /*!< PLLDIV[1:0] bits (PLL Output Division) */
#define RCC_CFGR_PLLDIV_0                   (0x1U << RCC_CFGR_PLLDIV_Pos)      /*!< 0x00400000 */
#define RCC_CFGR_PLLDIV_1                   (0x2U << RCC_CFGR_PLLDIV_Pos)      /*!< 0x00800000 */


/*!< PLLDIV configuration */
#define RCC_CFGR_PLLDIV1                    (0x00000000U)                      /*!< PLL clock output = CKVCO / 1 */
#define RCC_CFGR_PLLDIV2_Pos                (22U)                              
#define RCC_CFGR_PLLDIV2_Msk                (0x1U << RCC_CFGR_PLLDIV2_Pos)     /*!< 0x00400000 */
#define RCC_CFGR_PLLDIV2                    RCC_CFGR_PLLDIV2_Msk               /*!< PLL clock output = CKVCO / 2 */
#define RCC_CFGR_PLLDIV3_Pos                (23U)                              
#define RCC_CFGR_PLLDIV3_Msk                (0x1U << RCC_CFGR_PLLDIV3_Pos)     /*!< 0x00800000 */
#define RCC_CFGR_PLLDIV3                    RCC_CFGR_PLLDIV3_Msk               /*!< PLL clock output = CKVCO / 3 */
#define RCC_CFGR_PLLDIV4_Pos                (22U)                              
#define RCC_CFGR_PLLDIV4_Msk                (0x3U << RCC_CFGR_PLLDIV4_Pos)     /*!< 0x00C00000 */
#define RCC_CFGR_PLLDIV4                    RCC_CFGR_PLLDIV4_Msk               /*!< PLL clock output = CKVCO / 4 */


#define RCC_CFGR_MCOSEL_Pos                 (24U)                              
#define RCC_CFGR_MCOSEL_Msk                 (0x7U << RCC_CFGR_MCOSEL_Pos)      /*!< 0x07000000 */
#define RCC_CFGR_MCOSEL                     RCC_CFGR_MCOSEL_Msk                /*!< MCO[2:0] bits (Microcontroller Clock Output) */
#define RCC_CFGR_MCOSEL_0                   (0x1U << RCC_CFGR_MCOSEL_Pos)      /*!< 0x01000000 */
#define RCC_CFGR_MCOSEL_1                   (0x2U << RCC_CFGR_MCOSEL_Pos)      /*!< 0x02000000 */
#define RCC_CFGR_MCOSEL_2                   (0x4U << RCC_CFGR_MCOSEL_Pos)      /*!< 0x04000000 */

/*!< MCO configuration */
#define RCC_CFGR_MCOSEL_NOCLOCK             (0x00000000U)                      /*!< No clock */
#define RCC_CFGR_MCOSEL_SYSCLK_Pos          (24U)                              
#define RCC_CFGR_MCOSEL_SYSCLK_Msk          (0x1U << RCC_CFGR_MCOSEL_SYSCLK_Pos) /*!< 0x01000000 */
#define RCC_CFGR_MCOSEL_SYSCLK              RCC_CFGR_MCOSEL_SYSCLK_Msk         /*!< System clock selected */
#define RCC_CFGR_MCOSEL_HSI_Pos             (25U)                              
#define RCC_CFGR_MCOSEL_HSI_Msk             (0x1U << RCC_CFGR_MCOSEL_HSI_Pos)  /*!< 0x02000000 */
#define RCC_CFGR_MCOSEL_HSI                 RCC_CFGR_MCOSEL_HSI_Msk            /*!< Internal 16 MHz RC oscillator clock selected */
#define RCC_CFGR_MCOSEL_MSI_Pos             (24U)                              
#define RCC_CFGR_MCOSEL_MSI_Msk             (0x3U << RCC_CFGR_MCOSEL_MSI_Pos)  /*!< 0x03000000 */
#define RCC_CFGR_MCOSEL_MSI                 RCC_CFGR_MCOSEL_MSI_Msk            /*!< Internal Medium Speed RC oscillator clock selected */
#define RCC_CFGR_MCOSEL_HSE_Pos             (26U)                              
#define RCC_CFGR_MCOSEL_HSE_Msk             (0x1U << RCC_CFGR_MCOSEL_HSE_Pos)  /*!< 0x04000000 */
#define RCC_CFGR_MCOSEL_HSE                 RCC_CFGR_MCOSEL_HSE_Msk            /*!< External 1-25 MHz oscillator clock selected */
#define RCC_CFGR_MCOSEL_PLL_Pos             (24U)                              
#define RCC_CFGR_MCOSEL_PLL_Msk             (0x5U << RCC_CFGR_MCOSEL_PLL_Pos)  /*!< 0x05000000 */
#define RCC_CFGR_MCOSEL_PLL                 RCC_CFGR_MCOSEL_PLL_Msk            /*!< PLL clock divided */
#define RCC_CFGR_MCOSEL_LSI_Pos             (25U)                              
#define RCC_CFGR_MCOSEL_LSI_Msk             (0x3U << RCC_CFGR_MCOSEL_LSI_Pos)  /*!< 0x06000000 */
#define RCC_CFGR_MCOSEL_LSI                 RCC_CFGR_MCOSEL_LSI_Msk            /*!< LSI selected */
#define RCC_CFGR_MCOSEL_LSE_Pos             (24U)                              
#define RCC_CFGR_MCOSEL_LSE_Msk             (0x7U << RCC_CFGR_MCOSEL_LSE_Pos)  /*!< 0x07000000 */
#define RCC_CFGR_MCOSEL_LSE                 RCC_CFGR_MCOSEL_LSE_Msk            /*!< LSE selected */

#define RCC_CFGR_MCOPRE_Pos                 (28U)                              
#define RCC_CFGR_MCOPRE_Msk                 (0x7U << RCC_CFGR_MCOPRE_Pos)      /*!< 0x70000000 */
#define RCC_CFGR_MCOPRE                     RCC_CFGR_MCOPRE_Msk                /*!< MCOPRE[2:0] bits (Microcontroller Clock Output Prescaler) */
#define RCC_CFGR_MCOPRE_0                   (0x1U << RCC_CFGR_MCOPRE_Pos)      /*!< 0x10000000 */
#define RCC_CFGR_MCOPRE_1                   (0x2U << RCC_CFGR_MCOPRE_Pos)      /*!< 0x20000000 */
#define RCC_CFGR_MCOPRE_2                   (0x4U << RCC_CFGR_MCOPRE_Pos)      /*!< 0x40000000 */

/*!< MCO Prescaler configuration */ 
#define RCC_CFGR_MCOPRE_DIV1                (0x00000000U)                      /*!< MCO is divided by 1 */
#define RCC_CFGR_MCOPRE_DIV2                (0x10000000U)                      /*!< MCO is divided by 2 */
#define RCC_CFGR_MCOPRE_DIV4                (0x20000000U)                      /*!< MCO is divided by 4 */
#define RCC_CFGR_MCOPRE_DIV8                (0x30000000U)                      /*!< MCO is divided by 8 */
#define RCC_CFGR_MCOPRE_DIV16               (0x40000000U)                      /*!< MCO is divided by 16 */
 
/* Legacy aliases */
#define  RCC_CFGR_MCO_DIV1                  RCC_CFGR_MCOPRE_DIV1
#define  RCC_CFGR_MCO_DIV2                  RCC_CFGR_MCOPRE_DIV2
#define  RCC_CFGR_MCO_DIV4                  RCC_CFGR_MCOPRE_DIV4
#define  RCC_CFGR_MCO_DIV8                  RCC_CFGR_MCOPRE_DIV8
#define  RCC_CFGR_MCO_DIV16                 RCC_CFGR_MCOPRE_DIV16
#define  RCC_CFGR_MCO_NOCLOCK               RCC_CFGR_MCOSEL_NOCLOCK
#define  RCC_CFGR_MCO_SYSCLK                RCC_CFGR_MCOSEL_SYSCLK
#define  RCC_CFGR_MCO_HSI                   RCC_CFGR_MCOSEL_HSI
#define  RCC_CFGR_MCO_MSI                   RCC_CFGR_MCOSEL_MSI
#define  RCC_CFGR_MCO_HSE                   RCC_CFGR_MCOSEL_HSE
#define  RCC_CFGR_MCO_PLL                   RCC_CFGR_MCOSEL_PLL
#define  RCC_CFGR_MCO_LSI                   RCC_CFGR_MCOSEL_LSI
#define  RCC_CFGR_MCO_LSE                   RCC_CFGR_MCOSEL_LSE

/*!<******************  Bit definition for RCC_CIR register  ********************/
#define RCC_CIR_LSIRDYF_Pos                 (0U)                               
#define RCC_CIR_LSIRDYF_Msk                 (0x1U << RCC_CIR_LSIRDYF_Pos)      /*!< 0x00000001 */
#define RCC_CIR_LSIRDYF                     RCC_CIR_LSIRDYF_Msk                /*!< LSI Ready Interrupt flag */
#define RCC_CIR_LSERDYF_Pos                 (1U)                               
#define RCC_CIR_LSERDYF_Msk                 (0x1U << RCC_CIR_LSERDYF_Pos)      /*!< 0x00000002 */
#define RCC_CIR_LSERDYF                     RCC_CIR_LSERDYF_Msk                /*!< LSE Ready Interrupt flag */
#define RCC_CIR_HSIRDYF_Pos                 (2U)                               
#define RCC_CIR_HSIRDYF_Msk                 (0x1U << RCC_CIR_HSIRDYF_Pos)      /*!< 0x00000004 */
#define RCC_CIR_HSIRDYF                     RCC_CIR_HSIRDYF_Msk                /*!< HSI Ready Interrupt flag */
#define RCC_CIR_HSERDYF_Pos                 (3U)                               
#define RCC_CIR_HSERDYF_Msk                 (0x1U << RCC_CIR_HSERDYF_Pos)      /*!< 0x00000008 */
#define RCC_CIR_HSERDYF                     RCC_CIR_HSERDYF_Msk                /*!< HSE Ready Interrupt flag */
#define RCC_CIR_PLLRDYF_Pos                 (4U)                               
#define RCC_CIR_PLLRDYF_Msk                 (0x1U << RCC_CIR_PLLRDYF_Pos)      /*!< 0x00000010 */
#define RCC_CIR_PLLRDYF                     RCC_CIR_PLLRDYF_Msk                /*!< PLL Ready Interrupt flag */
#define RCC_CIR_MSIRDYF_Pos                 (5U)                               
#define RCC_CIR_MSIRDYF_Msk                 (0x1U << RCC_CIR_MSIRDYF_Pos)      /*!< 0x00000020 */
#define RCC_CIR_MSIRDYF                     RCC_CIR_MSIRDYF_Msk                /*!< MSI Ready Interrupt flag */
#define RCC_CIR_LSECSSF_Pos                 (6U)                               
#define RCC_CIR_LSECSSF_Msk                 (0x1U << RCC_CIR_LSECSSF_Pos)      /*!< 0x00000040 */
#define RCC_CIR_LSECSSF                     RCC_CIR_LSECSSF_Msk                /*!< LSE CSS Interrupt flag */
#define RCC_CIR_CSSF_Pos                    (7U)                               
#define RCC_CIR_CSSF_Msk                    (0x1U << RCC_CIR_CSSF_Pos)         /*!< 0x00000080 */
#define RCC_CIR_CSSF                        RCC_CIR_CSSF_Msk                   /*!< Clock Security System Interrupt flag */

#define RCC_CIR_LSIRDYIE_Pos                (8U)                               
#define RCC_CIR_LSIRDYIE_Msk                (0x1U << RCC_CIR_LSIRDYIE_Pos)     /*!< 0x00000100 */
#define RCC_CIR_LSIRDYIE                    RCC_CIR_LSIRDYIE_Msk               /*!< LSI Ready Interrupt Enable */
#define RCC_CIR_LSERDYIE_Pos                (9U)                               
#define RCC_CIR_LSERDYIE_Msk                (0x1U << RCC_CIR_LSERDYIE_Pos)     /*!< 0x00000200 */
#define RCC_CIR_LSERDYIE                    RCC_CIR_LSERDYIE_Msk               /*!< LSE Ready Interrupt Enable */
#define RCC_CIR_HSIRDYIE_Pos                (10U)                              
#define RCC_CIR_HSIRDYIE_Msk                (0x1U << RCC_CIR_HSIRDYIE_Pos)     /*!< 0x00000400 */
#define RCC_CIR_HSIRDYIE                    RCC_CIR_HSIRDYIE_Msk               /*!< HSI Ready Interrupt Enable */
#define RCC_CIR_HSERDYIE_Pos                (11U)                              
#define RCC_CIR_HSERDYIE_Msk                (0x1U << RCC_CIR_HSERDYIE_Pos)     /*!< 0x00000800 */
#define RCC_CIR_HSERDYIE                    RCC_CIR_HSERDYIE_Msk               /*!< HSE Ready Interrupt Enable */
#define RCC_CIR_PLLRDYIE_Pos                (12U)                              
#define RCC_CIR_PLLRDYIE_Msk                (0x1U << RCC_CIR_PLLRDYIE_Pos)     /*!< 0x00001000 */
#define RCC_CIR_PLLRDYIE                    RCC_CIR_PLLRDYIE_Msk               /*!< PLL Ready Interrupt Enable */
#define RCC_CIR_MSIRDYIE_Pos                (13U)                              
#define RCC_CIR_MSIRDYIE_Msk                (0x1U << RCC_CIR_MSIRDYIE_Pos)     /*!< 0x00002000 */
#define RCC_CIR_MSIRDYIE                    RCC_CIR_MSIRDYIE_Msk               /*!< MSI Ready Interrupt Enable */
#define RCC_CIR_LSECSSIE_Pos                (14U)                              
#define RCC_CIR_LSECSSIE_Msk                (0x1U << RCC_CIR_LSECSSIE_Pos)     /*!< 0x00004000 */
#define RCC_CIR_LSECSSIE                    RCC_CIR_LSECSSIE_Msk               /*!< LSE CSS Interrupt Enable */

#define RCC_CIR_LSIRDYC_Pos                 (16U)                              
#define RCC_CIR_LSIRDYC_Msk                 (0x1U << RCC_CIR_LSIRDYC_Pos)      /*!< 0x00010000 */
#define RCC_CIR_LSIRDYC                     RCC_CIR_LSIRDYC_Msk                /*!< LSI Ready Interrupt Clear */
#define RCC_CIR_LSERDYC_Pos                 (17U)                              
#define RCC_CIR_LSERDYC_Msk                 (0x1U << RCC_CIR_LSERDYC_Pos)      /*!< 0x00020000 */
#define RCC_CIR_LSERDYC                     RCC_CIR_LSERDYC_Msk                /*!< LSE Ready Interrupt Clear */
#define RCC_CIR_HSIRDYC_Pos                 (18U)                              
#define RCC_CIR_HSIRDYC_Msk                 (0x1U << RCC_CIR_HSIRDYC_Pos)      /*!< 0x00040000 */
#define RCC_CIR_HSIRDYC                     RCC_CIR_HSIRDYC_Msk                /*!< HSI Ready Interrupt Clear */
#define RCC_CIR_HSERDYC_Pos                 (19U)                              
#define RCC_CIR_HSERDYC_Msk                 (0x1U << RCC_CIR_HSERDYC_Pos)      /*!< 0x00080000 */
#define RCC_CIR_HSERDYC                     RCC_CIR_HSERDYC_Msk                /*!< HSE Ready Interrupt Clear */
#define RCC_CIR_PLLRDYC_Pos                 (20U)                              
#define RCC_CIR_PLLRDYC_Msk                 (0x1U << RCC_CIR_PLLRDYC_Pos)      /*!< 0x00100000 */
#define RCC_CIR_PLLRDYC                     RCC_CIR_PLLRDYC_Msk                /*!< PLL Ready Interrupt Clear */
#define RCC_CIR_MSIRDYC_Pos                 (21U)                              
#define RCC_CIR_MSIRDYC_Msk                 (0x1U << RCC_CIR_MSIRDYC_Pos)      /*!< 0x00200000 */
#define RCC_CIR_MSIRDYC                     RCC_CIR_MSIRDYC_Msk                /*!< MSI Ready Interrupt Clear */
#define RCC_CIR_LSECSSC_Pos                 (22U)                              
#define RCC_CIR_LSECSSC_Msk                 (0x1U << RCC_CIR_LSECSSC_Pos)      /*!< 0x00400000 */
#define RCC_CIR_LSECSSC                     RCC_CIR_LSECSSC_Msk                /*!< LSE CSS Interrupt Clear */
#define RCC_CIR_CSSC_Pos                    (23U)                              
#define RCC_CIR_CSSC_Msk                    (0x1U << RCC_CIR_CSSC_Pos)         /*!< 0x00800000 */
#define RCC_CIR_CSSC                        RCC_CIR_CSSC_Msk                   /*!< Clock Security System Interrupt Clear */

/*****************  Bit definition for RCC_AHBRSTR register  ******************/
#define RCC_AHBRSTR_GPIOARST_Pos            (0U)                               
#define RCC_AHBRSTR_GPIOARST_Msk            (0x1U << RCC_AHBRSTR_GPIOARST_Pos) /*!< 0x00000001 */
#define RCC_AHBRSTR_GPIOARST                RCC_AHBRSTR_GPIOARST_Msk           /*!< GPIO port A reset */
#define RCC_AHBRSTR_GPIOBRST_Pos            (1U)                               
#define RCC_AHBRSTR_GPIOBRST_Msk            (0x1U << RCC_AHBRSTR_GPIOBRST_Pos) /*!< 0x00000002 */
#define RCC_AHBRSTR_GPIOBRST                RCC_AHBRSTR_GPIOBRST_Msk           /*!< GPIO port B reset */
#define RCC_AHBRSTR_GPIOCRST_Pos            (2U)                               
#define RCC_AHBRSTR_GPIOCRST_Msk            (0x1U << RCC_AHBRSTR_GPIOCRST_Pos) /*!< 0x00000004 */
#define RCC_AHBRSTR_GPIOCRST                RCC_AHBRSTR_GPIOCRST_Msk           /*!< GPIO port C reset */
#define RCC_AHBRSTR_GPIODRST_Pos            (3U)                               
#define RCC_AHBRSTR_GPIODRST_Msk            (0x1U << RCC_AHBRSTR_GPIODRST_Pos) /*!< 0x00000008 */
#define RCC_AHBRSTR_GPIODRST                RCC_AHBRSTR_GPIODRST_Msk           /*!< GPIO port D reset */
#define RCC_AHBRSTR_GPIOERST_Pos            (4U)                               
#define RCC_AHBRSTR_GPIOERST_Msk            (0x1U << RCC_AHBRSTR_GPIOERST_Pos) /*!< 0x00000010 */
#define RCC_AHBRSTR_GPIOERST                RCC_AHBRSTR_GPIOERST_Msk           /*!< GPIO port E reset */
#define RCC_AHBRSTR_GPIOHRST_Pos            (5U)                               
#define RCC_AHBRSTR_GPIOHRST_Msk            (0x1U << RCC_AHBRSTR_GPIOHRST_Pos) /*!< 0x00000020 */
#define RCC_AHBRSTR_GPIOHRST                RCC_AHBRSTR_GPIOHRST_Msk           /*!< GPIO port H reset */
#define RCC_AHBRSTR_GPIOFRST_Pos            (6U)                               
#define RCC_AHBRSTR_GPIOFRST_Msk            (0x1U << RCC_AHBRSTR_GPIOFRST_Pos) /*!< 0x00000040 */
#define RCC_AHBRSTR_GPIOFRST                RCC_AHBRSTR_GPIOFRST_Msk           /*!< GPIO port F reset */
#define RCC_AHBRSTR_GPIOGRST_Pos            (7U)                               
#define RCC_AHBRSTR_GPIOGRST_Msk            (0x1U << RCC_AHBRSTR_GPIOGRST_Pos) /*!< 0x00000080 */
#define RCC_AHBRSTR_GPIOGRST                RCC_AHBRSTR_GPIOGRST_Msk           /*!< GPIO port G reset */
#define RCC_AHBRSTR_CRCRST_Pos              (12U)                              
#define RCC_AHBRSTR_CRCRST_Msk              (0x1U << RCC_AHBRSTR_CRCRST_Pos)   /*!< 0x00001000 */
#define RCC_AHBRSTR_CRCRST                  RCC_AHBRSTR_CRCRST_Msk             /*!< CRC reset */
#define RCC_AHBRSTR_FLITFRST_Pos            (15U)                              
#define RCC_AHBRSTR_FLITFRST_Msk            (0x1U << RCC_AHBRSTR_FLITFRST_Pos) /*!< 0x00008000 */
#define RCC_AHBRSTR_FLITFRST                RCC_AHBRSTR_FLITFRST_Msk           /*!< FLITF reset */
#define RCC_AHBRSTR_DMA1RST_Pos             (24U)                              
#define RCC_AHBRSTR_DMA1RST_Msk             (0x1U << RCC_AHBRSTR_DMA1RST_Pos)  /*!< 0x01000000 */
#define RCC_AHBRSTR_DMA1RST                 RCC_AHBRSTR_DMA1RST_Msk            /*!< DMA1 reset */
#define RCC_AHBRSTR_DMA2RST_Pos             (25U)                              
#define RCC_AHBRSTR_DMA2RST_Msk             (0x1U << RCC_AHBRSTR_DMA2RST_Pos)  /*!< 0x02000000 */
#define RCC_AHBRSTR_DMA2RST                 RCC_AHBRSTR_DMA2RST_Msk            /*!< DMA2 reset */
#define RCC_AHBRSTR_FSMCRST_Pos             (30U)                              
#define RCC_AHBRSTR_FSMCRST_Msk             (0x1U << RCC_AHBRSTR_FSMCRST_Pos)  /*!< 0x40000000 */
#define RCC_AHBRSTR_FSMCRST                 RCC_AHBRSTR_FSMCRST_Msk            /*!< FSMC reset */
 
/*****************  Bit definition for RCC_APB2RSTR register  *****************/
#define RCC_APB2RSTR_SYSCFGRST_Pos          (0U)                               
#define RCC_APB2RSTR_SYSCFGRST_Msk          (0x1U << RCC_APB2RSTR_SYSCFGRST_Pos) /*!< 0x00000001 */
#define RCC_APB2RSTR_SYSCFGRST              RCC_APB2RSTR_SYSCFGRST_Msk         /*!< System Configuration SYSCFG reset */
#define RCC_APB2RSTR_TIM9RST_Pos            (2U)                               
#define RCC_APB2RSTR_TIM9RST_Msk            (0x1U << RCC_APB2RSTR_TIM9RST_Pos) /*!< 0x00000004 */
#define RCC_APB2RSTR_TIM9RST                RCC_APB2RSTR_TIM9RST_Msk           /*!< TIM9 reset */
#define RCC_APB2RSTR_TIM10RST_Pos           (3U)                               
#define RCC_APB2RSTR_TIM10RST_Msk           (0x1U << RCC_APB2RSTR_TIM10RST_Pos) /*!< 0x00000008 */
#define RCC_APB2RSTR_TIM10RST               RCC_APB2RSTR_TIM10RST_Msk          /*!< TIM10 reset */
#define RCC_APB2RSTR_TIM11RST_Pos           (4U)                               
#define RCC_APB2RSTR_TIM11RST_Msk           (0x1U << RCC_APB2RSTR_TIM11RST_Pos) /*!< 0x00000010 */
#define RCC_APB2RSTR_TIM11RST               RCC_APB2RSTR_TIM11RST_Msk          /*!< TIM11 reset */
#define RCC_APB2RSTR_ADC1RST_Pos            (9U)                               
#define RCC_APB2RSTR_ADC1RST_Msk            (0x1U << RCC_APB2RSTR_ADC1RST_Pos) /*!< 0x00000200 */
#define RCC_APB2RSTR_ADC1RST                RCC_APB2RSTR_ADC1RST_Msk           /*!< ADC1 reset */
#define RCC_APB2RSTR_SDIORST_Pos            (11U)                              
#define RCC_APB2RSTR_SDIORST_Msk            (0x1U << RCC_APB2RSTR_SDIORST_Pos) /*!< 0x00000800 */
#define RCC_APB2RSTR_SDIORST                RCC_APB2RSTR_SDIORST_Msk           /*!< SDIO reset */
#define RCC_APB2RSTR_SPI1RST_Pos            (12U)                              
#define RCC_APB2RSTR_SPI1RST_Msk            (0x1U << RCC_APB2RSTR_SPI1RST_Pos) /*!< 0x00001000 */
#define RCC_APB2RSTR_SPI1RST                RCC_APB2RSTR_SPI1RST_Msk           /*!< SPI1 reset */
#define RCC_APB2RSTR_USART1RST_Pos          (14U)                              
#define RCC_APB2RSTR_USART1RST_Msk          (0x1U << RCC_APB2RSTR_USART1RST_Pos) /*!< 0x00004000 */
#define RCC_APB2RSTR_USART1RST              RCC_APB2RSTR_USART1RST_Msk         /*!< USART1 reset */

/*****************  Bit definition for RCC_APB1RSTR register  *****************/
#define RCC_APB1RSTR_TIM2RST_Pos            (0U)                               
#define RCC_APB1RSTR_TIM2RST_Msk            (0x1U << RCC_APB1RSTR_TIM2RST_Pos) /*!< 0x00000001 */
#define RCC_APB1RSTR_TIM2RST                RCC_APB1RSTR_TIM2RST_Msk           /*!< Timer 2 reset */
#define RCC_APB1RSTR_TIM3RST_Pos            (1U)                               
#define RCC_APB1RSTR_TIM3RST_Msk            (0x1U << RCC_APB1RSTR_TIM3RST_Pos) /*!< 0x00000002 */
#define RCC_APB1RSTR_TIM3RST                RCC_APB1RSTR_TIM3RST_Msk           /*!< Timer 3 reset */
#define RCC_APB1RSTR_TIM4RST_Pos            (2U)                               
#define RCC_APB1RSTR_TIM4RST_Msk            (0x1U << RCC_APB1RSTR_TIM4RST_Pos) /*!< 0x00000004 */
#define RCC_APB1RSTR_TIM4RST                RCC_APB1RSTR_TIM4RST_Msk           /*!< Timer 4 reset */
#define RCC_APB1RSTR_TIM5RST_Pos            (3U)                               
#define RCC_APB1RSTR_TIM5RST_Msk            (0x1U << RCC_APB1RSTR_TIM5RST_Pos) /*!< 0x00000008 */
#define RCC_APB1RSTR_TIM5RST                RCC_APB1RSTR_TIM5RST_Msk           /*!< Timer 5 reset */
#define RCC_APB1RSTR_TIM6RST_Pos            (4U)                               
#define RCC_APB1RSTR_TIM6RST_Msk            (0x1U << RCC_APB1RSTR_TIM6RST_Pos) /*!< 0x00000010 */
#define RCC_APB1RSTR_TIM6RST                RCC_APB1RSTR_TIM6RST_Msk           /*!< Timer 6 reset */
#define RCC_APB1RSTR_TIM7RST_Pos            (5U)                               
#define RCC_APB1RSTR_TIM7RST_Msk            (0x1U << RCC_APB1RSTR_TIM7RST_Pos) /*!< 0x00000020 */
#define RCC_APB1RSTR_TIM7RST                RCC_APB1RSTR_TIM7RST_Msk           /*!< Timer 7 reset */
#define RCC_APB1RSTR_LCDRST_Pos             (9U)                               
#define RCC_APB1RSTR_LCDRST_Msk             (0x1U << RCC_APB1RSTR_LCDRST_Pos)  /*!< 0x00000200 */
#define RCC_APB1RSTR_LCDRST                 RCC_APB1RSTR_LCDRST_Msk            /*!< LCD reset */
#define RCC_APB1RSTR_WWDGRST_Pos            (11U)                              
#define RCC_APB1RSTR_WWDGRST_Msk            (0x1U << RCC_APB1RSTR_WWDGRST_Pos) /*!< 0x00000800 */
#define RCC_APB1RSTR_WWDGRST                RCC_APB1RSTR_WWDGRST_Msk           /*!< Window Watchdog reset */
#define RCC_APB1RSTR_SPI2RST_Pos            (14U)                              
#define RCC_APB1RSTR_SPI2RST_Msk            (0x1U << RCC_APB1RSTR_SPI2RST_Pos) /*!< 0x00004000 */
#define RCC_APB1RSTR_SPI2RST                RCC_APB1RSTR_SPI2RST_Msk           /*!< SPI 2 reset */
#define RCC_APB1RSTR_SPI3RST_Pos            (15U)                              
#define RCC_APB1RSTR_SPI3RST_Msk            (0x1U << RCC_APB1RSTR_SPI3RST_Pos) /*!< 0x00008000 */
#define RCC_APB1RSTR_SPI3RST                RCC_APB1RSTR_SPI3RST_Msk           /*!< SPI 3 reset */
#define RCC_APB1RSTR_USART2RST_Pos          (17U)                              
#define RCC_APB1RSTR_USART2RST_Msk          (0x1U << RCC_APB1RSTR_USART2RST_Pos) /*!< 0x00020000 */
#define RCC_APB1RSTR_USART2RST              RCC_APB1RSTR_USART2RST_Msk         /*!< USART 2 reset */
#define RCC_APB1RSTR_USART3RST_Pos          (18U)                              
#define RCC_APB1RSTR_USART3RST_Msk          (0x1U << RCC_APB1RSTR_USART3RST_Pos) /*!< 0x00040000 */
#define RCC_APB1RSTR_USART3RST              RCC_APB1RSTR_USART3RST_Msk         /*!< USART 3 reset */
#define RCC_APB1RSTR_UART4RST_Pos           (19U)                              
#define RCC_APB1RSTR_UART4RST_Msk           (0x1U << RCC_APB1RSTR_UART4RST_Pos) /*!< 0x00080000 */
#define RCC_APB1RSTR_UART4RST               RCC_APB1RSTR_UART4RST_Msk          /*!< UART 4 reset */
#define RCC_APB1RSTR_UART5RST_Pos           (20U)                              
#define RCC_APB1RSTR_UART5RST_Msk           (0x1U << RCC_APB1RSTR_UART5RST_Pos) /*!< 0x00100000 */
#define RCC_APB1RSTR_UART5RST               RCC_APB1RSTR_UART5RST_Msk          /*!< UART 5 reset */
#define RCC_APB1RSTR_I2C1RST_Pos            (21U)                              
#define RCC_APB1RSTR_I2C1RST_Msk            (0x1U << RCC_APB1RSTR_I2C1RST_Pos) /*!< 0x00200000 */
#define RCC_APB1RSTR_I2C1RST                RCC_APB1RSTR_I2C1RST_Msk           /*!< I2C 1 reset */
#define RCC_APB1RSTR_I2C2RST_Pos            (22U)                              
#define RCC_APB1RSTR_I2C2RST_Msk            (0x1U << RCC_APB1RSTR_I2C2RST_Pos) /*!< 0x00400000 */
#define RCC_APB1RSTR_I2C2RST                RCC_APB1RSTR_I2C2RST_Msk           /*!< I2C 2 reset */
#define RCC_APB1RSTR_USBRST_Pos             (23U)                              
#define RCC_APB1RSTR_USBRST_Msk             (0x1U << RCC_APB1RSTR_USBRST_Pos)  /*!< 0x00800000 */
#define RCC_APB1RSTR_USBRST                 RCC_APB1RSTR_USBRST_Msk            /*!< USB reset */
#define RCC_APB1RSTR_PWRRST_Pos             (28U)                              
#define RCC_APB1RSTR_PWRRST_Msk             (0x1U << RCC_APB1RSTR_PWRRST_Pos)  /*!< 0x10000000 */
#define RCC_APB1RSTR_PWRRST                 RCC_APB1RSTR_PWRRST_Msk            /*!< Power interface reset */
#define RCC_APB1RSTR_DACRST_Pos             (29U)                              
#define RCC_APB1RSTR_DACRST_Msk             (0x1U << RCC_APB1RSTR_DACRST_Pos)  /*!< 0x20000000 */
#define RCC_APB1RSTR_DACRST                 RCC_APB1RSTR_DACRST_Msk            /*!< DAC interface reset */
#define RCC_APB1RSTR_COMPRST_Pos            (31U)                              
#define RCC_APB1RSTR_COMPRST_Msk            (0x1U << RCC_APB1RSTR_COMPRST_Pos) /*!< 0x80000000 */
#define RCC_APB1RSTR_COMPRST                RCC_APB1RSTR_COMPRST_Msk           /*!< Comparator interface reset */

/******************  Bit definition for RCC_AHBENR register  ******************/
#define RCC_AHBENR_GPIOAEN_Pos              (0U)                               
#define RCC_AHBENR_GPIOAEN_Msk              (0x1U << RCC_AHBENR_GPIOAEN_Pos)   /*!< 0x00000001 */
#define RCC_AHBENR_GPIOAEN                  RCC_AHBENR_GPIOAEN_Msk             /*!< GPIO port A clock enable */
#define RCC_AHBENR_GPIOBEN_Pos              (1U)                               
#define RCC_AHBENR_GPIOBEN_Msk              (0x1U << RCC_AHBENR_GPIOBEN_Pos)   /*!< 0x00000002 */
#define RCC_AHBENR_GPIOBEN                  RCC_AHBENR_GPIOBEN_Msk             /*!< GPIO port B clock enable */
#define RCC_AHBENR_GPIOCEN_Pos              (2U)                               
#define RCC_AHBENR_GPIOCEN_Msk              (0x1U << RCC_AHBENR_GPIOCEN_Pos)   /*!< 0x00000004 */
#define RCC_AHBENR_GPIOCEN                  RCC_AHBENR_GPIOCEN_Msk             /*!< GPIO port C clock enable */
#define RCC_AHBENR_GPIODEN_Pos              (3U)                               
#define RCC_AHBENR_GPIODEN_Msk              (0x1U << RCC_AHBENR_GPIODEN_Pos)   /*!< 0x00000008 */
#define RCC_AHBENR_GPIODEN                  RCC_AHBENR_GPIODEN_Msk             /*!< GPIO port D clock enable */
#define RCC_AHBENR_GPIOEEN_Pos              (4U)                               
#define RCC_AHBENR_GPIOEEN_Msk              (0x1U << RCC_AHBENR_GPIOEEN_Pos)   /*!< 0x00000010 */
#define RCC_AHBENR_GPIOEEN                  RCC_AHBENR_GPIOEEN_Msk             /*!< GPIO port E clock enable */
#define RCC_AHBENR_GPIOHEN_Pos              (5U)                               
#define RCC_AHBENR_GPIOHEN_Msk              (0x1U << RCC_AHBENR_GPIOHEN_Pos)   /*!< 0x00000020 */
#define RCC_AHBENR_GPIOHEN                  RCC_AHBENR_GPIOHEN_Msk             /*!< GPIO port H clock enable */
#define RCC_AHBENR_GPIOFEN_Pos              (6U)                               
#define RCC_AHBENR_GPIOFEN_Msk              (0x1U << RCC_AHBENR_GPIOFEN_Pos)   /*!< 0x00000040 */
#define RCC_AHBENR_GPIOFEN                  RCC_AHBENR_GPIOFEN_Msk             /*!< GPIO port F clock enable */
#define RCC_AHBENR_GPIOGEN_Pos              (7U)                               
#define RCC_AHBENR_GPIOGEN_Msk              (0x1U << RCC_AHBENR_GPIOGEN_Pos)   /*!< 0x00000080 */
#define RCC_AHBENR_GPIOGEN                  RCC_AHBENR_GPIOGEN_Msk             /*!< GPIO port G clock enable */
#define RCC_AHBENR_CRCEN_Pos                (12U)                              
#define RCC_AHBENR_CRCEN_Msk                (0x1U << RCC_AHBENR_CRCEN_Pos)     /*!< 0x00001000 */
#define RCC_AHBENR_CRCEN                    RCC_AHBENR_CRCEN_Msk               /*!< CRC clock enable */
#define RCC_AHBENR_FLITFEN_Pos              (15U)                              
#define RCC_AHBENR_FLITFEN_Msk              (0x1U << RCC_AHBENR_FLITFEN_Pos)   /*!< 0x00008000 */
#define RCC_AHBENR_FLITFEN                  RCC_AHBENR_FLITFEN_Msk             /*!< FLITF clock enable (has effect only when
                                                                                the Flash memory is in power down mode) */
#define RCC_AHBENR_DMA1EN_Pos               (24U)                              
#define RCC_AHBENR_DMA1EN_Msk               (0x1U << RCC_AHBENR_DMA1EN_Pos)    /*!< 0x01000000 */
#define RCC_AHBENR_DMA1EN                   RCC_AHBENR_DMA1EN_Msk              /*!< DMA1 clock enable */
#define RCC_AHBENR_DMA2EN_Pos               (25U)                              
#define RCC_AHBENR_DMA2EN_Msk               (0x1U << RCC_AHBENR_DMA2EN_Pos)    /*!< 0x02000000 */
#define RCC_AHBENR_DMA2EN                   RCC_AHBENR_DMA2EN_Msk              /*!< DMA2 clock enable */
#define RCC_AHBENR_FSMCEN_Pos               (30U)                              
#define RCC_AHBENR_FSMCEN_Msk               (0x1U << RCC_AHBENR_FSMCEN_Pos)    /*!< 0x40000000 */
#define RCC_AHBENR_FSMCEN                   RCC_AHBENR_FSMCEN_Msk              /*!< FSMC clock enable */

/******************  Bit definition for RCC_APB2ENR register  *****************/
#define RCC_APB2ENR_SYSCFGEN_Pos            (0U)                               
#define RCC_APB2ENR_SYSCFGEN_Msk            (0x1U << RCC_APB2ENR_SYSCFGEN_Pos) /*!< 0x00000001 */
#define RCC_APB2ENR_SYSCFGEN                RCC_APB2ENR_SYSCFGEN_Msk           /*!< System Configuration SYSCFG clock enable */
#define RCC_APB2ENR_TIM9EN_Pos              (2U)                               
#define RCC_APB2ENR_TIM9EN_Msk              (0x1U << RCC_APB2ENR_TIM9EN_Pos)   /*!< 0x00000004 */
#define RCC_APB2ENR_TIM9EN                  RCC_APB2ENR_TIM9EN_Msk             /*!< TIM9 interface clock enable */
#define RCC_APB2ENR_TIM10EN_Pos             (3U)                               
#define RCC_APB2ENR_TIM10EN_Msk             (0x1U << RCC_APB2ENR_TIM10EN_Pos)  /*!< 0x00000008 */
#define RCC_APB2ENR_TIM10EN                 RCC_APB2ENR_TIM10EN_Msk            /*!< TIM10 interface clock enable */
#define RCC_APB2ENR_TIM11EN_Pos             (4U)                               
#define RCC_APB2ENR_TIM11EN_Msk             (0x1U << RCC_APB2ENR_TIM11EN_Pos)  /*!< 0x00000010 */
#define RCC_APB2ENR_TIM11EN                 RCC_APB2ENR_TIM11EN_Msk            /*!< TIM11 Timer clock enable */
#define RCC_APB2ENR_ADC1EN_Pos              (9U)                               
#define RCC_APB2ENR_ADC1EN_Msk              (0x1U << RCC_APB2ENR_ADC1EN_Pos)   /*!< 0x00000200 */
#define RCC_APB2ENR_ADC1EN                  RCC_APB2ENR_ADC1EN_Msk             /*!< ADC1 clock enable */
#define RCC_APB2ENR_SDIOEN_Pos              (11U)                              
#define RCC_APB2ENR_SDIOEN_Msk              (0x1U << RCC_APB2ENR_SDIOEN_Pos)   /*!< 0x00000800 */
#define RCC_APB2ENR_SDIOEN                  RCC_APB2ENR_SDIOEN_Msk             /*!< SDIO clock enable */
#define RCC_APB2ENR_SPI1EN_Pos              (12U)                              
#define RCC_APB2ENR_SPI1EN_Msk              (0x1U << RCC_APB2ENR_SPI1EN_Pos)   /*!< 0x00001000 */
#define RCC_APB2ENR_SPI1EN                  RCC_APB2ENR_SPI1EN_Msk             /*!< SPI1 clock enable */
#define RCC_APB2ENR_USART1EN_Pos            (14U)                              
#define RCC_APB2ENR_USART1EN_Msk            (0x1U << RCC_APB2ENR_USART1EN_Pos) /*!< 0x00004000 */
#define RCC_APB2ENR_USART1EN                RCC_APB2ENR_USART1EN_Msk           /*!< USART1 clock enable */

/*****************  Bit definition for RCC_APB1ENR register  ******************/
#define RCC_APB1ENR_TIM2EN_Pos              (0U)                               
#define RCC_APB1ENR_TIM2EN_Msk              (0x1U << RCC_APB1ENR_TIM2EN_Pos)   /*!< 0x00000001 */
#define RCC_APB1ENR_TIM2EN                  RCC_APB1ENR_TIM2EN_Msk             /*!< Timer 2 clock enabled*/
#define RCC_APB1ENR_TIM3EN_Pos              (1U)                               
#define RCC_APB1ENR_TIM3EN_Msk              (0x1U << RCC_APB1ENR_TIM3EN_Pos)   /*!< 0x00000002 */
#define RCC_APB1ENR_TIM3EN                  RCC_APB1ENR_TIM3EN_Msk             /*!< Timer 3 clock enable */
#define RCC_APB1ENR_TIM4EN_Pos              (2U)                               
#define RCC_APB1ENR_TIM4EN_Msk              (0x1U << RCC_APB1ENR_TIM4EN_Pos)   /*!< 0x00000004 */
#define RCC_APB1ENR_TIM4EN                  RCC_APB1ENR_TIM4EN_Msk             /*!< Timer 4 clock enable */
#define RCC_APB1ENR_TIM5EN_Pos              (3U)                               
#define RCC_APB1ENR_TIM5EN_Msk              (0x1U << RCC_APB1ENR_TIM5EN_Pos)   /*!< 0x00000008 */
#define RCC_APB1ENR_TIM5EN                  RCC_APB1ENR_TIM5EN_Msk             /*!< Timer 5 clock enable */
#define RCC_APB1ENR_TIM6EN_Pos              (4U)                               
#define RCC_APB1ENR_TIM6EN_Msk              (0x1U << RCC_APB1ENR_TIM6EN_Pos)   /*!< 0x00000010 */
#define RCC_APB1ENR_TIM6EN                  RCC_APB1ENR_TIM6EN_Msk             /*!< Timer 6 clock enable */
#define RCC_APB1ENR_TIM7EN_Pos              (5U)                               
#define RCC_APB1ENR_TIM7EN_Msk              (0x1U << RCC_APB1ENR_TIM7EN_Pos)   /*!< 0x00000020 */
#define RCC_APB1ENR_TIM7EN                  RCC_APB1ENR_TIM7EN_Msk             /*!< Timer 7 clock enable */
#define RCC_APB1ENR_LCDEN_Pos               (9U)                               
#define RCC_APB1ENR_LCDEN_Msk               (0x1U << RCC_APB1ENR_LCDEN_Pos)    /*!< 0x00000200 */
#define RCC_APB1ENR_LCDEN                   RCC_APB1ENR_LCDEN_Msk              /*!< LCD clock enable */
#define RCC_APB1ENR_WWDGEN_Pos              (11U)                              
#define RCC_APB1ENR_WWDGEN_Msk              (0x1U << RCC_APB1ENR_WWDGEN_Pos)   /*!< 0x00000800 */
#define RCC_APB1ENR_WWDGEN                  RCC_APB1ENR_WWDGEN_Msk             /*!< Window Watchdog clock enable */
#define RCC_APB1ENR_SPI2EN_Pos              (14U)                              
#define RCC_APB1ENR_SPI2EN_Msk              (0x1U << RCC_APB1ENR_SPI2EN_Pos)   /*!< 0x00004000 */
#define RCC_APB1ENR_SPI2EN                  RCC_APB1ENR_SPI2EN_Msk             /*!< SPI 2 clock enable */
#define RCC_APB1ENR_SPI3EN_Pos              (15U)                              
#define RCC_APB1ENR_SPI3EN_Msk              (0x1U << RCC_APB1ENR_SPI3EN_Pos)   /*!< 0x00008000 */
#define RCC_APB1ENR_SPI3EN                  RCC_APB1ENR_SPI3EN_Msk             /*!< SPI 3 clock enable */
#define RCC_APB1ENR_USART2EN_Pos            (17U)                              
#define RCC_APB1ENR_USART2EN_Msk            (0x1U << RCC_APB1ENR_USART2EN_Pos) /*!< 0x00020000 */
#define RCC_APB1ENR_USART2EN                RCC_APB1ENR_USART2EN_Msk           /*!< USART 2 clock enable */
#define RCC_APB1ENR_USART3EN_Pos            (18U)                              
#define RCC_APB1ENR_USART3EN_Msk            (0x1U << RCC_APB1ENR_USART3EN_Pos) /*!< 0x00040000 */
#define RCC_APB1ENR_USART3EN                RCC_APB1ENR_USART3EN_Msk           /*!< USART 3 clock enable */
#define RCC_APB1ENR_UART4EN_Pos             (19U)                              
#define RCC_APB1ENR_UART4EN_Msk             (0x1U << RCC_APB1ENR_UART4EN_Pos)  /*!< 0x00080000 */
#define RCC_APB1ENR_UART4EN                 RCC_APB1ENR_UART4EN_Msk            /*!< UART 4 clock enable */
#define RCC_APB1ENR_UART5EN_Pos             (20U)                              
#define RCC_APB1ENR_UART5EN_Msk             (0x1U << RCC_APB1ENR_UART5EN_Pos)  /*!< 0x00100000 */
#define RCC_APB1ENR_UART5EN                 RCC_APB1ENR_UART5EN_Msk            /*!< UART 5 clock enable */
#define RCC_APB1ENR_I2C1EN_Pos              (21U)                              
#define RCC_APB1ENR_I2C1EN_Msk              (0x1U << RCC_APB1ENR_I2C1EN_Pos)   /*!< 0x00200000 */
#define RCC_APB1ENR_I2C1EN                  RCC_APB1ENR_I2C1EN_Msk             /*!< I2C 1 clock enable */
#define RCC_APB1ENR_I2C2EN_Pos              (22U)                              
#define RCC_APB1ENR_I2C2EN_Msk              (0x1U << RCC_APB1ENR_I2C2EN_Pos)   /*!< 0x00400000 */
#define RCC_APB1ENR_I2C2EN                  RCC_APB1ENR_I2C2EN_Msk             /*!< I2C 2 clock enable */
#define RCC_APB1ENR_USBEN_Pos               (23U)                              
#define RCC_APB1ENR_USBEN_Msk               (0x1U << RCC_APB1ENR_USBEN_Pos)    /*!< 0x00800000 */
#define RCC_APB1ENR_USBEN                   RCC_APB1ENR_USBEN_Msk              /*!< USB clock enable */
#define RCC_APB1ENR_PWREN_Pos               (28U)                              
#define RCC_APB1ENR_PWREN_Msk               (0x1U << RCC_APB1ENR_PWREN_Pos)    /*!< 0x10000000 */
#define RCC_APB1ENR_PWREN                   RCC_APB1ENR_PWREN_Msk              /*!< Power interface clock enable */
#define RCC_APB1ENR_DACEN_Pos               (29U)                              
#define RCC_APB1ENR_DACEN_Msk               (0x1U << RCC_APB1ENR_DACEN_Pos)    /*!< 0x20000000 */
#define RCC_APB1ENR_DACEN                   RCC_APB1ENR_DACEN_Msk              /*!< DAC interface clock enable */
#define RCC_APB1ENR_COMPEN_Pos              (31U)                              
#define RCC_APB1ENR_COMPEN_Msk              (0x1U << RCC_APB1ENR_COMPEN_Pos)   /*!< 0x80000000 */
#define RCC_APB1ENR_COMPEN                  RCC_APB1ENR_COMPEN_Msk             /*!< Comparator interface clock enable */

/******************  Bit definition for RCC_AHBLPENR register  ****************/
#define RCC_AHBLPENR_GPIOALPEN_Pos          (0U)                               
#define RCC_AHBLPENR_GPIOALPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOALPEN_Pos) /*!< 0x00000001 */
#define RCC_AHBLPENR_GPIOALPEN              RCC_AHBLPENR_GPIOALPEN_Msk         /*!< GPIO port A clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOBLPEN_Pos          (1U)                               
#define RCC_AHBLPENR_GPIOBLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOBLPEN_Pos) /*!< 0x00000002 */
#define RCC_AHBLPENR_GPIOBLPEN              RCC_AHBLPENR_GPIOBLPEN_Msk         /*!< GPIO port B clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOCLPEN_Pos          (2U)                               
#define RCC_AHBLPENR_GPIOCLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOCLPEN_Pos) /*!< 0x00000004 */
#define RCC_AHBLPENR_GPIOCLPEN              RCC_AHBLPENR_GPIOCLPEN_Msk         /*!< GPIO port C clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIODLPEN_Pos          (3U)                               
#define RCC_AHBLPENR_GPIODLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIODLPEN_Pos) /*!< 0x00000008 */
#define RCC_AHBLPENR_GPIODLPEN              RCC_AHBLPENR_GPIODLPEN_Msk         /*!< GPIO port D clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOELPEN_Pos          (4U)                               
#define RCC_AHBLPENR_GPIOELPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOELPEN_Pos) /*!< 0x00000010 */
#define RCC_AHBLPENR_GPIOELPEN              RCC_AHBLPENR_GPIOELPEN_Msk         /*!< GPIO port E clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOHLPEN_Pos          (5U)                               
#define RCC_AHBLPENR_GPIOHLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOHLPEN_Pos) /*!< 0x00000020 */
#define RCC_AHBLPENR_GPIOHLPEN              RCC_AHBLPENR_GPIOHLPEN_Msk         /*!< GPIO port H clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOFLPEN_Pos          (6U)                               
#define RCC_AHBLPENR_GPIOFLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOFLPEN_Pos) /*!< 0x00000040 */
#define RCC_AHBLPENR_GPIOFLPEN              RCC_AHBLPENR_GPIOFLPEN_Msk         /*!< GPIO port F clock enabled in sleep mode */
#define RCC_AHBLPENR_GPIOGLPEN_Pos          (7U)                               
#define RCC_AHBLPENR_GPIOGLPEN_Msk          (0x1U << RCC_AHBLPENR_GPIOGLPEN_Pos) /*!< 0x00000080 */
#define RCC_AHBLPENR_GPIOGLPEN              RCC_AHBLPENR_GPIOGLPEN_Msk         /*!< GPIO port G clock enabled in sleep mode */
#define RCC_AHBLPENR_CRCLPEN_Pos            (12U)                              
#define RCC_AHBLPENR_CRCLPEN_Msk            (0x1U << RCC_AHBLPENR_CRCLPEN_Pos) /*!< 0x00001000 */
#define RCC_AHBLPENR_CRCLPEN                RCC_AHBLPENR_CRCLPEN_Msk           /*!< CRC clock enabled in sleep mode */
#define RCC_AHBLPENR_FLITFLPEN_Pos          (15U)                              
#define RCC_AHBLPENR_FLITFLPEN_Msk          (0x1U << RCC_AHBLPENR_FLITFLPEN_Pos) /*!< 0x00008000 */
#define RCC_AHBLPENR_FLITFLPEN              RCC_AHBLPENR_FLITFLPEN_Msk         /*!< Flash Interface clock enabled in sleep mode
                                                                                (has effect only when the Flash memory is
                                                                                 in power down mode) */
#define RCC_AHBLPENR_SRAMLPEN_Pos           (16U)                              
#define RCC_AHBLPENR_SRAMLPEN_Msk           (0x1U << RCC_AHBLPENR_SRAMLPEN_Pos) /*!< 0x00010000 */
#define RCC_AHBLPENR_SRAMLPEN               RCC_AHBLPENR_SRAMLPEN_Msk          /*!< SRAM clock enabled in sleep mode */
#define RCC_AHBLPENR_DMA1LPEN_Pos           (24U)                              
#define RCC_AHBLPENR_DMA1LPEN_Msk           (0x1U << RCC_AHBLPENR_DMA1LPEN_Pos) /*!< 0x01000000 */
#define RCC_AHBLPENR_DMA1LPEN               RCC_AHBLPENR_DMA1LPEN_Msk          /*!< DMA1 clock enabled in sleep mode */
#define RCC_AHBLPENR_DMA2LPEN_Pos           (25U)                              
#define RCC_AHBLPENR_DMA2LPEN_Msk           (0x1U << RCC_AHBLPENR_DMA2LPEN_Pos) /*!< 0x02000000 */
#define RCC_AHBLPENR_DMA2LPEN               RCC_AHBLPENR_DMA2LPEN_Msk          /*!< DMA2 clock enabled in sleep mode */
#define RCC_AHBLPENR_FSMCLPEN_Pos           (30U)                              
#define RCC_AHBLPENR_FSMCLPEN_Msk           (0x1U << RCC_AHBLPENR_FSMCLPEN_Pos) /*!< 0x40000000 */
#define RCC_AHBLPENR_FSMCLPEN               RCC_AHBLPENR_FSMCLPEN_Msk          /*!< FSMC clock enabled in sleep mode */

/******************  Bit definition for RCC_APB2LPENR register  ***************/
#define RCC_APB2LPENR_SYSCFGLPEN_Pos        (0U)                               
#define RCC_APB2LPENR_SYSCFGLPEN_Msk        (0x1U << RCC_APB2LPENR_SYSCFGLPEN_Pos) /*!< 0x00000001 */
#define RCC_APB2LPENR_SYSCFGLPEN            RCC_APB2LPENR_SYSCFGLPEN_Msk       /*!< System Configuration SYSCFG clock enabled in sleep mode */
#define RCC_APB2LPENR_TIM9LPEN_Pos          (2U)                               
#define RCC_APB2LPENR_TIM9LPEN_Msk          (0x1U << RCC_APB2LPENR_TIM9LPEN_Pos) /*!< 0x00000004 */
#define RCC_APB2LPENR_TIM9LPEN              RCC_APB2LPENR_TIM9LPEN_Msk         /*!< TIM9 interface clock enabled in sleep mode */
#define RCC_APB2LPENR_TIM10LPEN_Pos         (3U)                               
#define RCC_APB2LPENR_TIM10LPEN_Msk         (0x1U << RCC_APB2LPENR_TIM10LPEN_Pos) /*!< 0x00000008 */
#define RCC_APB2LPENR_TIM10LPEN             RCC_APB2LPENR_TIM10LPEN_Msk        /*!< TIM10 interface clock enabled in sleep mode */
#define RCC_APB2LPENR_TIM11LPEN_Pos         (4U)                               
#define RCC_APB2LPENR_TIM11LPEN_Msk         (0x1U << RCC_APB2LPENR_TIM11LPEN_Pos) /*!< 0x00000010 */
#define RCC_APB2LPENR_TIM11LPEN             RCC_APB2LPENR_TIM11LPEN_Msk        /*!< TIM11 Timer clock enabled in sleep mode */
#define RCC_APB2LPENR_ADC1LPEN_Pos          (9U)                               
#define RCC_APB2LPENR_ADC1LPEN_Msk          (0x1U << RCC_APB2LPENR_ADC1LPEN_Pos) /*!< 0x00000200 */
#define RCC_APB2LPENR_ADC1LPEN              RCC_APB2LPENR_ADC1LPEN_Msk         /*!< ADC1 clock enabled in sleep mode */
#define RCC_APB2LPENR_SDIOLPEN_Pos          (11U)                              
#define RCC_APB2LPENR_SDIOLPEN_Msk          (0x1U << RCC_APB2LPENR_SDIOLPEN_Pos) /*!< 0x00000800 */
#define RCC_APB2LPENR_SDIOLPEN              RCC_APB2LPENR_SDIOLPEN_Msk         /*!< SDIO clock enabled in sleep mode */
#define RCC_APB2LPENR_SPI1LPEN_Pos          (12U)                              
#define RCC_APB2LPENR_SPI1LPEN_Msk          (0x1U << RCC_APB2LPENR_SPI1LPEN_Pos) /*!< 0x00001000 */
#define RCC_APB2LPENR_SPI1LPEN              RCC_APB2LPENR_SPI1LPEN_Msk         /*!< SPI1 clock enabled in sleep mode */
#define RCC_APB2LPENR_USART1LPEN_Pos        (14U)                              
#define RCC_APB2LPENR_USART1LPEN_Msk        (0x1U << RCC_APB2LPENR_USART1LPEN_Pos) /*!< 0x00004000 */
#define RCC_APB2LPENR_USART1LPEN            RCC_APB2LPENR_USART1LPEN_Msk       /*!< USART1 clock enabled in sleep mode */

/*****************  Bit definition for RCC_APB1LPENR register  ****************/
#define RCC_APB1LPENR_TIM2LPEN_Pos          (0U)                               
#define RCC_APB1LPENR_TIM2LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM2LPEN_Pos) /*!< 0x00000001 */
#define RCC_APB1LPENR_TIM2LPEN              RCC_APB1LPENR_TIM2LPEN_Msk         /*!< Timer 2 clock enabled in sleep mode */
#define RCC_APB1LPENR_TIM3LPEN_Pos          (1U)                               
#define RCC_APB1LPENR_TIM3LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM3LPEN_Pos) /*!< 0x00000002 */
#define RCC_APB1LPENR_TIM3LPEN              RCC_APB1LPENR_TIM3LPEN_Msk         /*!< Timer 3 clock enabled in sleep mode */
#define RCC_APB1LPENR_TIM4LPEN_Pos          (2U)                               
#define RCC_APB1LPENR_TIM4LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM4LPEN_Pos) /*!< 0x00000004 */
#define RCC_APB1LPENR_TIM4LPEN              RCC_APB1LPENR_TIM4LPEN_Msk         /*!< Timer 4 clock enabled in sleep mode */
#define RCC_APB1LPENR_TIM5LPEN_Pos          (3U)                               
#define RCC_APB1LPENR_TIM5LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM5LPEN_Pos) /*!< 0x00000008 */
#define RCC_APB1LPENR_TIM5LPEN              RCC_APB1LPENR_TIM5LPEN_Msk         /*!< Timer 5 clock enabled in sleep mode */
#define RCC_APB1LPENR_TIM6LPEN_Pos          (4U)                               
#define RCC_APB1LPENR_TIM6LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM6LPEN_Pos) /*!< 0x00000010 */
#define RCC_APB1LPENR_TIM6LPEN              RCC_APB1LPENR_TIM6LPEN_Msk         /*!< Timer 6 clock enabled in sleep mode */
#define RCC_APB1LPENR_TIM7LPEN_Pos          (5U)                               
#define RCC_APB1LPENR_TIM7LPEN_Msk          (0x1U << RCC_APB1LPENR_TIM7LPEN_Pos) /*!< 0x00000020 */
#define RCC_APB1LPENR_TIM7LPEN              RCC_APB1LPENR_TIM7LPEN_Msk         /*!< Timer 7 clock enabled in sleep mode */
#define RCC_APB1LPENR_LCDLPEN_Pos           (9U)                               
#define RCC_APB1LPENR_LCDLPEN_Msk           (0x1U << RCC_APB1LPENR_LCDLPEN_Pos) /*!< 0x00000200 */
#define RCC_APB1LPENR_LCDLPEN               RCC_APB1LPENR_LCDLPEN_Msk          /*!< LCD clock enabled in sleep mode */
#define RCC_APB1LPENR_WWDGLPEN_Pos          (11U)                              
#define RCC_APB1LPENR_WWDGLPEN_Msk          (0x1U << RCC_APB1LPENR_WWDGLPEN_Pos) /*!< 0x00000800 */
#define RCC_APB1LPENR_WWDGLPEN              RCC_APB1LPENR_WWDGLPEN_Msk         /*!< Window Watchdog clock enabled in sleep mode */
#define RCC_APB1LPENR_SPI2LPEN_Pos          (14U)                              
#define RCC_APB1LPENR_SPI2LPEN_Msk          (0x1U << RCC_APB1LPENR_SPI2LPEN_Pos) /*!< 0x00004000 */
#define RCC_APB1LPENR_SPI2LPEN              RCC_APB1LPENR_SPI2LPEN_Msk         /*!< SPI 2 clock enabled in sleep mode */
#define RCC_APB1LPENR_SPI3LPEN_Pos          (15U)                              
#define RCC_APB1LPENR_SPI3LPEN_Msk          (0x1U << RCC_APB1LPENR_SPI3LPEN_Pos) /*!< 0x00008000 */
#define RCC_APB1LPENR_SPI3LPEN              RCC_APB1LPENR_SPI3LPEN_Msk         /*!< SPI 3 clock enabled in sleep mode */
#define RCC_APB1LPENR_USART2LPEN_Pos        (17U)                              
#define RCC_APB1LPENR_USART2LPEN_Msk        (0x1U << RCC_APB1LPENR_USART2LPEN_Pos) /*!< 0x00020000 */
#define RCC_APB1LPENR_USART2LPEN            RCC_APB1LPENR_USART2LPEN_Msk       /*!< USART 2 clock enabled in sleep mode */
#define RCC_APB1LPENR_USART3LPEN_Pos        (18U)                              
#define RCC_APB1LPENR_USART3LPEN_Msk        (0x1U << RCC_APB1LPENR_USART3LPEN_Pos) /*!< 0x00040000 */
#define RCC_APB1LPENR_USART3LPEN            RCC_APB1LPENR_USART3LPEN_Msk       /*!< USART 3 clock enabled in sleep mode */
#define RCC_APB1LPENR_UART4LPEN_Pos         (19U)                              
#define RCC_APB1LPENR_UART4LPEN_Msk         (0x1U << RCC_APB1LPENR_UART4LPEN_Pos) /*!< 0x00080000 */
#define RCC_APB1LPENR_UART4LPEN             RCC_APB1LPENR_UART4LPEN_Msk        /*!< UART 4 clock enabled in sleep mode */
#define RCC_APB1LPENR_UART5LPEN_Pos         (20U)                              
#define RCC_APB1LPENR_UART5LPEN_Msk         (0x1U << RCC_APB1LPENR_UART5LPEN_Pos) /*!< 0x00100000 */
#define RCC_APB1LPENR_UART5LPEN             RCC_APB1LPENR_UART5LPEN_Msk        /*!< UART 5 clock enabled in sleep mode */
#define RCC_APB1LPENR_I2C1LPEN_Pos          (21U)                              
#define RCC_APB1LPENR_I2C1LPEN_Msk          (0x1U << RCC_APB1LPENR_I2C1LPEN_Pos) /*!< 0x00200000 */
#define RCC_APB1LPENR_I2C1LPEN              RCC_APB1LPENR_I2C1LPEN_Msk         /*!< I2C 1 clock enabled in sleep mode */
#define RCC_APB1LPENR_I2C2LPEN_Pos          (22U)                              
#define RCC_APB1LPENR_I2C2LPEN_Msk          (0x1U << RCC_APB1LPENR_I2C2LPEN_Pos) /*!< 0x00400000 */
#define RCC_APB1LPENR_I2C2LPEN              RCC_APB1LPENR_I2C2LPEN_Msk         /*!< I2C 2 clock enabled in sleep mode */
#define RCC_APB1LPENR_USBLPEN_Pos           (23U)                              
#define RCC_APB1LPENR_USBLPEN_Msk           (0x1U << RCC_APB1LPENR_USBLPEN_Pos) /*!< 0x00800000 */
#define RCC_APB1LPENR_USBLPEN               RCC_APB1LPENR_USBLPEN_Msk          /*!< USB clock enabled in sleep mode */
#define RCC_APB1LPENR_PWRLPEN_Pos           (28U)                              
#define RCC_APB1LPENR_PWRLPEN_Msk           (0x1U << RCC_APB1LPENR_PWRLPEN_Pos) /*!< 0x10000000 */
#define RCC_APB1LPENR_PWRLPEN               RCC_APB1LPENR_PWRLPEN_Msk          /*!< Power interface clock enabled in sleep mode */
#define RCC_APB1LPENR_DACLPEN_Pos           (29U)                              
#define RCC_APB1LPENR_DACLPEN_Msk           (0x1U << RCC_APB1LPENR_DACLPEN_Pos) /*!< 0x20000000 */
#define RCC_APB1LPENR_DACLPEN               RCC_APB1LPENR_DACLPEN_Msk          /*!< DAC interface clock enabled in sleep mode */
#define RCC_APB1LPENR_COMPLPEN_Pos          (31U)                              
#define RCC_APB1LPENR_COMPLPEN_Msk          (0x1U << RCC_APB1LPENR_COMPLPEN_Pos) /*!< 0x80000000 */
#define RCC_APB1LPENR_COMPLPEN              RCC_APB1LPENR_COMPLPEN_Msk         /*!< Comparator interface clock enabled in sleep mode*/

/*******************  Bit definition for RCC_CSR register  ********************/
#define RCC_CSR_LSION_Pos                   (0U)                               
#define RCC_CSR_LSION_Msk                   (0x1U << RCC_CSR_LSION_Pos)        /*!< 0x00000001 */
#define RCC_CSR_LSION                       RCC_CSR_LSION_Msk                  /*!< Internal Low Speed oscillator enable */
#define RCC_CSR_LSIRDY_Pos                  (1U)                               
#define RCC_CSR_LSIRDY_Msk                  (0x1U << RCC_CSR_LSIRDY_Pos)       /*!< 0x00000002 */
#define RCC_CSR_LSIRDY                      RCC_CSR_LSIRDY_Msk                 /*!< Internal Low Speed oscillator Ready */

#define RCC_CSR_LSEON_Pos                   (8U)                               
#define RCC_CSR_LSEON_Msk                   (0x1U << RCC_CSR_LSEON_Pos)        /*!< 0x00000100 */
#define RCC_CSR_LSEON                       RCC_CSR_LSEON_Msk                  /*!< External Low Speed oscillator enable */
#define RCC_CSR_LSERDY_Pos                  (9U)                               
#define RCC_CSR_LSERDY_Msk                  (0x1U << RCC_CSR_LSERDY_Pos)       /*!< 0x00000200 */
#define RCC_CSR_LSERDY                      RCC_CSR_LSERDY_Msk                 /*!< External Low Speed oscillator Ready */
#define RCC_CSR_LSEBYP_Pos                  (10U)                              
#define RCC_CSR_LSEBYP_Msk                  (0x1U << RCC_CSR_LSEBYP_Pos)       /*!< 0x00000400 */
#define RCC_CSR_LSEBYP                      RCC_CSR_LSEBYP_Msk                 /*!< External Low Speed oscillator Bypass */

#define RCC_CSR_LSECSSON_Pos                (11U)                              
#define RCC_CSR_LSECSSON_Msk                (0x1U << RCC_CSR_LSECSSON_Pos)     /*!< 0x00000800 */
#define RCC_CSR_LSECSSON                    RCC_CSR_LSECSSON_Msk               /*!< External Low Speed oscillator CSS Enable */
#define RCC_CSR_LSECSSD_Pos                 (12U)                              
#define RCC_CSR_LSECSSD_Msk                 (0x1U << RCC_CSR_LSECSSD_Pos)      /*!< 0x00001000 */
#define RCC_CSR_LSECSSD                     RCC_CSR_LSECSSD_Msk                /*!< External Low Speed oscillator CSS Detected */

#define RCC_CSR_RTCSEL_Pos                  (16U)                              
#define RCC_CSR_RTCSEL_Msk                  (0x3U << RCC_CSR_RTCSEL_Pos)       /*!< 0x00030000 */
#define RCC_CSR_RTCSEL                      RCC_CSR_RTCSEL_Msk                 /*!< RTCSEL[1:0] bits (RTC clock source selection) */
#define RCC_CSR_RTCSEL_0                    (0x1U << RCC_CSR_RTCSEL_Pos)       /*!< 0x00010000 */
#define RCC_CSR_RTCSEL_1                    (0x2U << RCC_CSR_RTCSEL_Pos)       /*!< 0x00020000 */

/*!< RTC congiguration */
#define RCC_CSR_RTCSEL_NOCLOCK              (0x00000000U)                      /*!< No clock */
#define RCC_CSR_RTCSEL_LSE_Pos              (16U)                              
#define RCC_CSR_RTCSEL_LSE_Msk              (0x1U << RCC_CSR_RTCSEL_LSE_Pos)   /*!< 0x00010000 */
#define RCC_CSR_RTCSEL_LSE                  RCC_CSR_RTCSEL_LSE_Msk             /*!< LSE oscillator clock used as RTC clock */
#define RCC_CSR_RTCSEL_LSI_Pos              (17U)                              
#define RCC_CSR_RTCSEL_LSI_Msk              (0x1U << RCC_CSR_RTCSEL_LSI_Pos)   /*!< 0x00020000 */
#define RCC_CSR_RTCSEL_LSI                  RCC_CSR_RTCSEL_LSI_Msk             /*!< LSI oscillator clock used as RTC clock */
#define RCC_CSR_RTCSEL_HSE_Pos              (16U)                              
#define RCC_CSR_RTCSEL_HSE_Msk              (0x3U << RCC_CSR_RTCSEL_HSE_Pos)   /*!< 0x00030000 */
#define RCC_CSR_RTCSEL_HSE                  RCC_CSR_RTCSEL_HSE_Msk             /*!< HSE oscillator clock divided by 2, 4, 8 or 16 by RTCPRE used as RTC clock */

#define RCC_CSR_RTCEN_Pos                   (22U)                              
#define RCC_CSR_RTCEN_Msk                   (0x1U << RCC_CSR_RTCEN_Pos)        /*!< 0x00400000 */
#define RCC_CSR_RTCEN                       RCC_CSR_RTCEN_Msk                  /*!< RTC clock enable */
#define RCC_CSR_RTCRST_Pos                  (23U)                              
#define RCC_CSR_RTCRST_Msk                  (0x1U << RCC_CSR_RTCRST_Pos)       /*!< 0x00800000 */
#define RCC_CSR_RTCRST                      RCC_CSR_RTCRST_Msk                 /*!< RTC reset  */
 
#define RCC_CSR_RMVF_Pos                    (24U)                              
#define RCC_CSR_RMVF_Msk                    (0x1U << RCC_CSR_RMVF_Pos)         /*!< 0x01000000 */
#define RCC_CSR_RMVF                        RCC_CSR_RMVF_Msk                   /*!< Remove reset flag */
#define RCC_CSR_OBLRSTF_Pos                 (25U)                              
#define RCC_CSR_OBLRSTF_Msk                 (0x1U << RCC_CSR_OBLRSTF_Pos)      /*!< 0x02000000 */
#define RCC_CSR_OBLRSTF                     RCC_CSR_OBLRSTF_Msk                /*!< Option Bytes Loader reset flag */
#define RCC_CSR_PINRSTF_Pos                 (26U)                              
#define RCC_CSR_PINRSTF_Msk                 (0x1U << RCC_CSR_PINRSTF_Pos)      /*!< 0x04000000 */
#define RCC_CSR_PINRSTF                     RCC_CSR_PINRSTF_Msk                /*!< PIN reset flag */
#define RCC_CSR_PORRSTF_Pos                 (27U)                              
#define RCC_CSR_PORRSTF_Msk                 (0x1U << RCC_CSR_PORRSTF_Pos)      /*!< 0x08000000 */
#define RCC_CSR_PORRSTF                     RCC_CSR_PORRSTF_Msk                /*!< POR/PDR reset flag */
#define RCC_CSR_SFTRSTF_Pos                 (28U)                              
#define RCC_CSR_SFTRSTF_Msk                 (0x1U << RCC_CSR_SFTRSTF_Pos)      /*!< 0x10000000 */
#define RCC_CSR_SFTRSTF                     RCC_CSR_SFTRSTF_Msk                /*!< Software Reset flag */
#define RCC_CSR_IWDGRSTF_Pos                (29U)                              
#define RCC_CSR_IWDGRSTF_Msk                (0x1U << RCC_CSR_IWDGRSTF_Pos)     /*!< 0x20000000 */
#define RCC_CSR_IWDGRSTF                    RCC_CSR_IWDGRSTF_Msk               /*!< Independent Watchdog reset flag */
#define RCC_CSR_WWDGRSTF_Pos                (30U)                              
#define RCC_CSR_WWDGRSTF_Msk                (0x1U << RCC_CSR_WWDGRSTF_Pos)     /*!< 0x40000000 */
#define RCC_CSR_WWDGRSTF                    RCC_CSR_WWDGRSTF_Msk               /*!< Window watchdog reset flag */
#define RCC_CSR_LPWRRSTF_Pos                (31U)                              
#define RCC_CSR_LPWRRSTF_Msk                (0x1U << RCC_CSR_LPWRRSTF_Pos)     /*!< 0x80000000 */
#define RCC_CSR_LPWRRSTF                    RCC_CSR_LPWRRSTF_Msk               /*!< Low-Power reset flag */

/******************************************************************************/
/*                                                                            */
/*                           Real-Time Clock (RTC)                            */
/*                                                                            */
/******************************************************************************/
/*
* @brief Specific device feature definitions  (not present on all devices in the STM32F0 serie)
*/
#define RTC_TAMPER1_SUPPORT       /*!< TAMPER 1 feature support */
#define RTC_TAMPER2_SUPPORT       /*!< TAMPER 2 feature support */
#define RTC_TAMPER3_SUPPORT       /*!< TAMPER 3 feature support */
#define RTC_BACKUP_SUPPORT        /*!< BACKUP register feature support */
#define RTC_WAKEUP_SUPPORT        /*!< WAKEUP feature support */
#define RTC_SMOOTHCALIB_SUPPORT   /*!< Smooth digital calibration feature support */
#define RTC_SUBSECOND_SUPPORT     /*!< Sub-second feature support */

/********************  Bits definition for RTC_TR register  *******************/
#define RTC_TR_PM_Pos                        (22U)                             
#define RTC_TR_PM_Msk                        (0x1U << RTC_TR_PM_Pos)           /*!< 0x00400000 */
#define RTC_TR_PM                            RTC_TR_PM_Msk                     
#define RTC_TR_HT_Pos                        (20U)                             
#define RTC_TR_HT_Msk                        (0x3U << RTC_TR_HT_Pos)           /*!< 0x00300000 */
#define RTC_TR_HT                            RTC_TR_HT_Msk                     
#define RTC_TR_HT_0                          (0x1U << RTC_TR_HT_Pos)           /*!< 0x00100000 */
#define RTC_TR_HT_1                          (0x2U << RTC_TR_HT_Pos)           /*!< 0x00200000 */
#define RTC_TR_HU_Pos                        (16U)                             
#define RTC_TR_HU_Msk                        (0xFU << RTC_TR_HU_Pos)           /*!< 0x000F0000 */
#define RTC_TR_HU                            RTC_TR_HU_Msk                     
#define RTC_TR_HU_0                          (0x1U << RTC_TR_HU_Pos)           /*!< 0x00010000 */
#define RTC_TR_HU_1                          (0x2U << RTC_TR_HU_Pos)           /*!< 0x00020000 */
#define RTC_TR_HU_2                          (0x4U << RTC_TR_HU_Pos)           /*!< 0x00040000 */
#define RTC_TR_HU_3                          (0x8U << RTC_TR_HU_Pos)           /*!< 0x00080000 */
#define RTC_TR_MNT_Pos                       (12U)                             
#define RTC_TR_MNT_Msk                       (0x7U << RTC_TR_MNT_Pos)          /*!< 0x00007000 */
#define RTC_TR_MNT                           RTC_TR_MNT_Msk                    
#define RTC_TR_MNT_0                         (0x1U << RTC_TR_MNT_Pos)          /*!< 0x00001000 */
#define RTC_TR_MNT_1                         (0x2U << RTC_TR_MNT_Pos)          /*!< 0x00002000 */
#define RTC_TR_MNT_2                         (0x4U << RTC_TR_MNT_Pos)          /*!< 0x00004000 */
#define RTC_TR_MNU_Pos                       (8U)                              
#define RTC_TR_MNU_Msk                       (0xFU << RTC_TR_MNU_Pos)          /*!< 0x00000F00 */
#define RTC_TR_MNU                           RTC_TR_MNU_Msk                    
#define RTC_TR_MNU_0                         (0x1U << RTC_TR_MNU_Pos)          /*!< 0x00000100 */
#define RTC_TR_MNU_1                         (0x2U << RTC_TR_MNU_Pos)          /*!< 0x00000200 */
#define RTC_TR_MNU_2                         (0x4U << RTC_TR_MNU_Pos)          /*!< 0x00000400 */
#define RTC_TR_MNU_3                         (0x8U << RTC_TR_MNU_Pos)          /*!< 0x00000800 */
#define RTC_TR_ST_Pos                        (4U)                              
#define RTC_TR_ST_Msk                        (0x7U << RTC_TR_ST_Pos)           /*!< 0x00000070 */
#define RTC_TR_ST                            RTC_TR_ST_Msk                     
#define RTC_TR_ST_0                          (0x1U << RTC_TR_ST_Pos)           /*!< 0x00000010 */
#define RTC_TR_ST_1                          (0x2U << RTC_TR_ST_Pos)           /*!< 0x00000020 */
#define RTC_TR_ST_2                          (0x4U << RTC_TR_ST_Pos)           /*!< 0x00000040 */
#define RTC_TR_SU_Pos                        (0U)                              
#define RTC_TR_SU_Msk                        (0xFU << RTC_TR_SU_Pos)           /*!< 0x0000000F */
#define RTC_TR_SU                            RTC_TR_SU_Msk                     
#define RTC_TR_SU_0                          (0x1U << RTC_TR_SU_Pos)           /*!< 0x00000001 */
#define RTC_TR_SU_1                          (0x2U << RTC_TR_SU_Pos)           /*!< 0x00000002 */
#define RTC_TR_SU_2                          (0x4U << RTC_TR_SU_Pos)           /*!< 0x00000004 */
#define RTC_TR_SU_3                          (0x8U << RTC_TR_SU_Pos)           /*!< 0x00000008 */

/********************  Bits definition for RTC_DR register  *******************/
#define RTC_DR_YT_Pos                        (20U)                             
#define RTC_DR_YT_Msk                        (0xFU << RTC_DR_YT_Pos)           /*!< 0x00F00000 */
#define RTC_DR_YT                            RTC_DR_YT_Msk                     
#define RTC_DR_YT_0                          (0x1U << RTC_DR_YT_Pos)           /*!< 0x00100000 */
#define RTC_DR_YT_1                          (0x2U << RTC_DR_YT_Pos)           /*!< 0x00200000 */
#define RTC_DR_YT_2                          (0x4U << RTC_DR_YT_Pos)           /*!< 0x00400000 */
#define RTC_DR_YT_3                          (0x8U << RTC_DR_YT_Pos)           /*!< 0x00800000 */
#define RTC_DR_YU_Pos                        (16U)                             
#define RTC_DR_YU_Msk                        (0xFU << RTC_DR_YU_Pos)           /*!< 0x000F0000 */
#define RTC_DR_YU                            RTC_DR_YU_Msk                     
#define RTC_DR_YU_0                          (0x1U << RTC_DR_YU_Pos)           /*!< 0x00010000 */
#define RTC_DR_YU_1                          (0x2U << RTC_DR_YU_Pos)           /*!< 0x00020000 */
#define RTC_DR_YU_2                          (0x4U << RTC_DR_YU_Pos)           /*!< 0x00040000 */
#define RTC_DR_YU_3                          (0x8U << RTC_DR_YU_Pos)           /*!< 0x00080000 */
#define RTC_DR_WDU_Pos                       (13U)                             
#define RTC_DR_WDU_Msk                       (0x7U << RTC_DR_WDU_Pos)          /*!< 0x0000E000 */
#define RTC_DR_WDU                           RTC_DR_WDU_Msk                    
#define RTC_DR_WDU_0                         (0x1U << RTC_DR_WDU_Pos)          /*!< 0x00002000 */
#define RTC_DR_WDU_1                         (0x2U << RTC_DR_WDU_Pos)          /*!< 0x00004000 */
#define RTC_DR_WDU_2                         (0x4U << RTC_DR_WDU_Pos)          /*!< 0x00008000 */
#define RTC_DR_MT_Pos                        (12U)                             
#define RTC_DR_MT_Msk                        (0x1U << RTC_DR_MT_Pos)           /*!< 0x00001000 */
#define RTC_DR_MT                            RTC_DR_MT_Msk                     
#define RTC_DR_MU_Pos                        (8U)                              
#define RTC_DR_MU_Msk                        (0xFU << RTC_DR_MU_Pos)           /*!< 0x00000F00 */
#define RTC_DR_MU                            RTC_DR_MU_Msk                     
#define RTC_DR_MU_0                          (0x1U << RTC_DR_MU_Pos)           /*!< 0x00000100 */
#define RTC_DR_MU_1                          (0x2U << RTC_DR_MU_Pos)           /*!< 0x00000200 */
#define RTC_DR_MU_2                          (0x4U << RTC_DR_MU_Pos)           /*!< 0x00000400 */
#define RTC_DR_MU_3                          (0x8U << RTC_DR_MU_Pos)           /*!< 0x00000800 */
#define RTC_DR_DT_Pos                        (4U)                              
#define RTC_DR_DT_Msk                        (0x3U << RTC_DR_DT_Pos)           /*!< 0x00000030 */
#define RTC_DR_DT                            RTC_DR_DT_Msk                     
#define RTC_DR_DT_0                          (0x1U << RTC_DR_DT_Pos)           /*!< 0x00000010 */
#define RTC_DR_DT_1                          (0x2U << RTC_DR_DT_Pos)           /*!< 0x00000020 */
#define RTC_DR_DU_Pos                        (0U)                              
#define RTC_DR_DU_Msk                        (0xFU << RTC_DR_DU_Pos)           /*!< 0x0000000F */
#define RTC_DR_DU                            RTC_DR_DU_Msk                     
#define RTC_DR_DU_0                          (0x1U << RTC_DR_DU_Pos)           /*!< 0x00000001 */
#define RTC_DR_DU_1                          (0x2U << RTC_DR_DU_Pos)           /*!< 0x00000002 */
#define RTC_DR_DU_2                          (0x4U << RTC_DR_DU_Pos)           /*!< 0x00000004 */
#define RTC_DR_DU_3                          (0x8U << RTC_DR_DU_Pos)           /*!< 0x00000008 */

/********************  Bits definition for RTC_CR register  *******************/
#define RTC_CR_COE_Pos                       (23U)                             
#define RTC_CR_COE_Msk                       (0x1U << RTC_CR_COE_Pos)          /*!< 0x00800000 */
#define RTC_CR_COE                           RTC_CR_COE_Msk                    
#define RTC_CR_OSEL_Pos                      (21U)                             
#define RTC_CR_OSEL_Msk                      (0x3U << RTC_CR_OSEL_Pos)         /*!< 0x00600000 */
#define RTC_CR_OSEL                          RTC_CR_OSEL_Msk                   
#define RTC_CR_OSEL_0                        (0x1U << RTC_CR_OSEL_Pos)         /*!< 0x00200000 */
#define RTC_CR_OSEL_1                        (0x2U << RTC_CR_OSEL_Pos)         /*!< 0x00400000 */
#define RTC_CR_POL_Pos                       (20U)                             
#define RTC_CR_POL_Msk                       (0x1U << RTC_CR_POL_Pos)          /*!< 0x00100000 */
#define RTC_CR_POL                           RTC_CR_POL_Msk                    
#define RTC_CR_COSEL_Pos                     (19U)                             
#define RTC_CR_COSEL_Msk                     (0x1U << RTC_CR_COSEL_Pos)        /*!< 0x00080000 */
#define RTC_CR_COSEL                         RTC_CR_COSEL_Msk                  
#define RTC_CR_BKP_Pos                       (18U)                             
#define RTC_CR_BKP_Msk                       (0x1U << RTC_CR_BKP_Pos)          /*!< 0x00040000 */
#define RTC_CR_BKP                           RTC_CR_BKP_Msk                    
#define RTC_CR_SUB1H_Pos                     (17U)                             
#define RTC_CR_SUB1H_Msk                     (0x1U << RTC_CR_SUB1H_Pos)        /*!< 0x00020000 */
#define RTC_CR_SUB1H                         RTC_CR_SUB1H_Msk                  
#define RTC_CR_ADD1H_Pos                     (16U)                             
#define RTC_CR_ADD1H_Msk                     (0x1U << RTC_CR_ADD1H_Pos)        /*!< 0x00010000 */
#define RTC_CR_ADD1H                         RTC_CR_ADD1H_Msk                  
#define RTC_CR_TSIE_Pos                      (15U)                             
#define RTC_CR_TSIE_Msk                      (0x1U << RTC_CR_TSIE_Pos)         /*!< 0x00008000 */
#define RTC_CR_TSIE                          RTC_CR_TSIE_Msk                   
#define RTC_CR_WUTIE_Pos                     (14U)                             
#define RTC_CR_WUTIE_Msk                     (0x1U << RTC_CR_WUTIE_Pos)        /*!< 0x00004000 */
#define RTC_CR_WUTIE                         RTC_CR_WUTIE_Msk                  
#define RTC_CR_ALRBIE_Pos                    (13U)                             
#define RTC_CR_ALRBIE_Msk                    (0x1U << RTC_CR_ALRBIE_Pos)       /*!< 0x00002000 */
#define RTC_CR_ALRBIE                        RTC_CR_ALRBIE_Msk                 
#define RTC_CR_ALRAIE_Pos                    (12U)                             
#define RTC_CR_ALRAIE_Msk                    (0x1U << RTC_CR_ALRAIE_Pos)       /*!< 0x00001000 */
#define RTC_CR_ALRAIE                        RTC_CR_ALRAIE_Msk                 
#define RTC_CR_TSE_Pos                       (11U)                             
#define RTC_CR_TSE_Msk                       (0x1U << RTC_CR_TSE_Pos)          /*!< 0x00000800 */
#define RTC_CR_TSE                           RTC_CR_TSE_Msk                    
#define RTC_CR_WUTE_Pos                      (10U)                             
#define RTC_CR_WUTE_Msk                      (0x1U << RTC_CR_WUTE_Pos)         /*!< 0x00000400 */
#define RTC_CR_WUTE                          RTC_CR_WUTE_Msk                   
#define RTC_CR_ALRBE_Pos                     (9U)                              
#define RTC_CR_ALRBE_Msk                     (0x1U << RTC_CR_ALRBE_Pos)        /*!< 0x00000200 */
#define RTC_CR_ALRBE                         RTC_CR_ALRBE_Msk                  
#define RTC_CR_ALRAE_Pos                     (8U)                              
#define RTC_CR_ALRAE_Msk                     (0x1U << RTC_CR_ALRAE_Pos)        /*!< 0x00000100 */
#define RTC_CR_ALRAE                         RTC_CR_ALRAE_Msk                  
#define RTC_CR_DCE_Pos                       (7U)                              
#define RTC_CR_DCE_Msk                       (0x1U << RTC_CR_DCE_Pos)          /*!< 0x00000080 */
#define RTC_CR_DCE                           RTC_CR_DCE_Msk                    
#define RTC_CR_FMT_Pos                       (6U)                              
#define RTC_CR_FMT_Msk                       (0x1U << RTC_CR_FMT_Pos)          /*!< 0x00000040 */
#define RTC_CR_FMT                           RTC_CR_FMT_Msk                    
#define RTC_CR_BYPSHAD_Pos                   (5U)                              
#define RTC_CR_BYPSHAD_Msk                   (0x1U << RTC_CR_BYPSHAD_Pos)      /*!< 0x00000020 */
#define RTC_CR_BYPSHAD                       RTC_CR_BYPSHAD_Msk                
#define RTC_CR_REFCKON_Pos                   (4U)                              
#define RTC_CR_REFCKON_Msk                   (0x1U << RTC_CR_REFCKON_Pos)      /*!< 0x00000010 */
#define RTC_CR_REFCKON                       RTC_CR_REFCKON_Msk                
#define RTC_CR_TSEDGE_Pos                    (3U)                              
#define RTC_CR_TSEDGE_Msk                    (0x1U << RTC_CR_TSEDGE_Pos)       /*!< 0x00000008 */
#define RTC_CR_TSEDGE                        RTC_CR_TSEDGE_Msk                 
#define RTC_CR_WUCKSEL_Pos                   (0U)                              
#define RTC_CR_WUCKSEL_Msk                   (0x7U << RTC_CR_WUCKSEL_Pos)      /*!< 0x00000007 */
#define RTC_CR_WUCKSEL                       RTC_CR_WUCKSEL_Msk                
#define RTC_CR_WUCKSEL_0                     (0x1U << RTC_CR_WUCKSEL_Pos)      /*!< 0x00000001 */
#define RTC_CR_WUCKSEL_1                     (0x2U << RTC_CR_WUCKSEL_Pos)      /*!< 0x00000002 */
#define RTC_CR_WUCKSEL_2                     (0x4U << RTC_CR_WUCKSEL_Pos)      /*!< 0x00000004 */

/* Legacy defines */
#define  RTC_CR_BCK_Pos RTC_CR_BKP_Pos
#define  RTC_CR_BCK_Msk RTC_CR_BKP_Msk
#define  RTC_CR_BCK     RTC_CR_BKP

/********************  Bits definition for RTC_ISR register  ******************/
#define RTC_ISR_RECALPF_Pos                  (16U)                             
#define RTC_ISR_RECALPF_Msk                  (0x1U << RTC_ISR_RECALPF_Pos)     /*!< 0x00010000 */
#define RTC_ISR_RECALPF                      RTC_ISR_RECALPF_Msk               
#define RTC_ISR_TAMP3F_Pos                   (15U)                             
#define RTC_ISR_TAMP3F_Msk                   (0x1U << RTC_ISR_TAMP3F_Pos)      /*!< 0x00008000 */
#define RTC_ISR_TAMP3F                       RTC_ISR_TAMP3F_Msk                
#define RTC_ISR_TAMP2F_Pos                   (14U)                             
#define RTC_ISR_TAMP2F_Msk                   (0x1U << RTC_ISR_TAMP2F_Pos)      /*!< 0x00004000 */
#define RTC_ISR_TAMP2F                       RTC_ISR_TAMP2F_Msk                
#define RTC_ISR_TAMP1F_Pos                   (13U)                             
#define RTC_ISR_TAMP1F_Msk                   (0x1U << RTC_ISR_TAMP1F_Pos)      /*!< 0x00002000 */
#define RTC_ISR_TAMP1F                       RTC_ISR_TAMP1F_Msk                
#define RTC_ISR_TSOVF_Pos                    (12U)                             
#define RTC_ISR_TSOVF_Msk                    (0x1U << RTC_ISR_TSOVF_Pos)       /*!< 0x00001000 */
#define RTC_ISR_TSOVF                        RTC_ISR_TSOVF_Msk                 
#define RTC_ISR_TSF_Pos                      (11U)                             
#define RTC_ISR_TSF_Msk                      (0x1U << RTC_ISR_TSF_Pos)         /*!< 0x00000800 */
#define RTC_ISR_TSF                          RTC_ISR_TSF_Msk                   
#define RTC_ISR_WUTF_Pos                     (10U)                             
#define RTC_ISR_WUTF_Msk                     (0x1U << RTC_ISR_WUTF_Pos)        /*!< 0x00000400 */
#define RTC_ISR_WUTF                         RTC_ISR_WUTF_Msk                  
#define RTC_ISR_ALRBF_Pos                    (9U)                              
#define RTC_ISR_ALRBF_Msk                    (0x1U << RTC_ISR_ALRBF_Pos)       /*!< 0x00000200 */
#define RTC_ISR_ALRBF                        RTC_ISR_ALRBF_Msk                 
#define RTC_ISR_ALRAF_Pos                    (8U)                              
#define RTC_ISR_ALRAF_Msk                    (0x1U << RTC_ISR_ALRAF_Pos)       /*!< 0x00000100 */
#define RTC_ISR_ALRAF                        RTC_ISR_ALRAF_Msk                 
#define RTC_ISR_INIT_Pos                     (7U)                              
#define RTC_ISR_INIT_Msk                     (0x1U << RTC_ISR_INIT_Pos)        /*!< 0x00000080 */
#define RTC_ISR_INIT                         RTC_ISR_INIT_Msk                  
#define RTC_ISR_INITF_Pos                    (6U)                              
#define RTC_ISR_INITF_Msk                    (0x1U << RTC_ISR_INITF_Pos)       /*!< 0x00000040 */
#define RTC_ISR_INITF                        RTC_ISR_INITF_Msk                 
#define RTC_ISR_RSF_Pos                      (5U)                              
#define RTC_ISR_RSF_Msk                      (0x1U << RTC_ISR_RSF_Pos)         /*!< 0x00000020 */
#define RTC_ISR_RSF                          RTC_ISR_RSF_Msk                   
#define RTC_ISR_INITS_Pos                    (4U)                              
#define RTC_ISR_INITS_Msk                    (0x1U << RTC_ISR_INITS_Pos)       /*!< 0x00000010 */
#define RTC_ISR_INITS                        RTC_ISR_INITS_Msk                 
#define RTC_ISR_SHPF_Pos                     (3U)                              
#define RTC_ISR_SHPF_Msk                     (0x1U << RTC_ISR_SHPF_Pos)        /*!< 0x00000008 */
#define RTC_ISR_SHPF                         RTC_ISR_SHPF_Msk                  
#define RTC_ISR_WUTWF_Pos                    (2U)                              
#define RTC_ISR_WUTWF_Msk                    (0x1U << RTC_ISR_WUTWF_Pos)       /*!< 0x00000004 */
#define RTC_ISR_WUTWF                        RTC_ISR_WUTWF_Msk                 
#define RTC_ISR_ALRBWF_Pos                   (1U)                              
#define RTC_ISR_ALRBWF_Msk                   (0x1U << RTC_ISR_ALRBWF_Pos)      /*!< 0x00000002 */
#define RTC_ISR_ALRBWF                       RTC_ISR_ALRBWF_Msk                
#define RTC_ISR_ALRAWF_Pos                   (0U)                              
#define RTC_ISR_ALRAWF_Msk                   (0x1U << RTC_ISR_ALRAWF_Pos)      /*!< 0x00000001 */
#define RTC_ISR_ALRAWF                       RTC_ISR_ALRAWF_Msk                

/********************  Bits definition for RTC_PRER register  *****************/
#define RTC_PRER_PREDIV_A_Pos                (16U)                             
#define RTC_PRER_PREDIV_A_Msk                (0x7FU << RTC_PRER_PREDIV_A_Pos)  /*!< 0x007F0000 */
#define RTC_PRER_PREDIV_A                    RTC_PRER_PREDIV_A_Msk             
#define RTC_PRER_PREDIV_S_Pos                (0U)                              
#define RTC_PRER_PREDIV_S_Msk                (0x7FFFU << RTC_PRER_PREDIV_S_Pos) /*!< 0x00007FFF */
#define RTC_PRER_PREDIV_S                    RTC_PRER_PREDIV_S_Msk             

/********************  Bits definition for RTC_WUTR register  *****************/
#define RTC_WUTR_WUT_Pos                     (0U)                              
#define RTC_WUTR_WUT_Msk                     (0xFFFFU << RTC_WUTR_WUT_Pos)     /*!< 0x0000FFFF */
#define RTC_WUTR_WUT                         RTC_WUTR_WUT_Msk                  

/********************  Bits definition for RTC_CALIBR register  ***************/
#define RTC_CALIBR_DCS_Pos                   (7U)                              
#define RTC_CALIBR_DCS_Msk                   (0x1U << RTC_CALIBR_DCS_Pos)      /*!< 0x00000080 */
#define RTC_CALIBR_DCS                       RTC_CALIBR_DCS_Msk                
#define RTC_CALIBR_DC_Pos                    (0U)                              
#define RTC_CALIBR_DC_Msk                    (0x1FU << RTC_CALIBR_DC_Pos)      /*!< 0x0000001F */
#define RTC_CALIBR_DC                        RTC_CALIBR_DC_Msk                 

/********************  Bits definition for RTC_ALRMAR register  ***************/
#define RTC_ALRMAR_MSK4_Pos                  (31U)                             
#define RTC_ALRMAR_MSK4_Msk                  (0x1U << RTC_ALRMAR_MSK4_Pos)     /*!< 0x80000000 */
#define RTC_ALRMAR_MSK4                      RTC_ALRMAR_MSK4_Msk               
#define RTC_ALRMAR_WDSEL_Pos                 (30U)                             
#define RTC_ALRMAR_WDSEL_Msk                 (0x1U << RTC_ALRMAR_WDSEL_Pos)    /*!< 0x40000000 */
#define RTC_ALRMAR_WDSEL                     RTC_ALRMAR_WDSEL_Msk              
#define RTC_ALRMAR_DT_Pos                    (28U)                             
#define RTC_ALRMAR_DT_Msk                    (0x3U << RTC_ALRMAR_DT_Pos)       /*!< 0x30000000 */
#define RTC_ALRMAR_DT                        RTC_ALRMAR_DT_Msk                 
#define RTC_ALRMAR_DT_0                      (0x1U << RTC_ALRMAR_DT_Pos)       /*!< 0x10000000 */
#define RTC_ALRMAR_DT_1                      (0x2U << RTC_ALRMAR_DT_Pos)       /*!< 0x20000000 */
#define RTC_ALRMAR_DU_Pos                    (24U)                             
#define RTC_ALRMAR_DU_Msk                    (0xFU << RTC_ALRMAR_DU_Pos)       /*!< 0x0F000000 */
#define RTC_ALRMAR_DU                        RTC_ALRMAR_DU_Msk                 
#define RTC_ALRMAR_DU_0                      (0x1U << RTC_ALRMAR_DU_Pos)       /*!< 0x01000000 */
#define RTC_ALRMAR_DU_1                      (0x2U << RTC_ALRMAR_DU_Pos)       /*!< 0x02000000 */
#define RTC_ALRMAR_DU_2                      (0x4U << RTC_ALRMAR_DU_Pos)       /*!< 0x04000000 */
#define RTC_ALRMAR_DU_3                      (0x8U << RTC_ALRMAR_DU_Pos)       /*!< 0x08000000 */
#define RTC_ALRMAR_MSK3_Pos                  (23U)                             
#define RTC_ALRMAR_MSK3_Msk                  (0x1U << RTC_ALRMAR_MSK3_Pos)     /*!< 0x00800000 */
#define RTC_ALRMAR_MSK3                      RTC_ALRMAR_MSK3_Msk               
#define RTC_ALRMAR_PM_Pos                    (22U)                             
#define RTC_ALRMAR_PM_Msk                    (0x1U << RTC_ALRMAR_PM_Pos)       /*!< 0x00400000 */
#define RTC_ALRMAR_PM                        RTC_ALRMAR_PM_Msk                 
#define RTC_ALRMAR_HT_Pos                    (20U)                             
#define RTC_ALRMAR_HT_Msk                    (0x3U << RTC_ALRMAR_HT_Pos)       /*!< 0x00300000 */
#define RTC_ALRMAR_HT                        RTC_ALRMAR_HT_Msk                 
#define RTC_ALRMAR_HT_0                      (0x1U << RTC_ALRMAR_HT_Pos)       /*!< 0x00100000 */
#define RTC_ALRMAR_HT_1                      (0x2U << RTC_ALRMAR_HT_Pos)       /*!< 0x00200000 */
#define RTC_ALRMAR_HU_Pos                    (16U)                             
#define RTC_ALRMAR_HU_Msk                    (0xFU << RTC_ALRMAR_HU_Pos)       /*!< 0x000F0000 */
#define RTC_ALRMAR_HU                        RTC_ALRMAR_HU_Msk                 
#define RTC_ALRMAR_HU_0                      (0x1U << RTC_ALRMAR_HU_Pos)       /*!< 0x00010000 */
#define RTC_ALRMAR_HU_1                      (0x2U << RTC_ALRMAR_HU_Pos)       /*!< 0x00020000 */
#define RTC_ALRMAR_HU_2                      (0x4U << RTC_ALRMAR_HU_Pos)       /*!< 0x00040000 */
#define RTC_ALRMAR_HU_3                      (0x8U << RTC_ALRMAR_HU_Pos)       /*!< 0x00080000 */
#define RTC_ALRMAR_MSK2_Pos                  (15U)                             
#define RTC_ALRMAR_MSK2_Msk                  (0x1U << RTC_ALRMAR_MSK2_Pos)     /*!< 0x00008000 */
#define RTC_ALRMAR_MSK2                      RTC_ALRMAR_MSK2_Msk               
#define RTC_ALRMAR_MNT_Pos                   (12U)                             
#define RTC_ALRMAR_MNT_Msk                   (0x7U << RTC_ALRMAR_MNT_Pos)      /*!< 0x00007000 */
#define RTC_ALRMAR_MNT                       RTC_ALRMAR_MNT_Msk                
#define RTC_ALRMAR_MNT_0                     (0x1U << RTC_ALRMAR_MNT_Pos)      /*!< 0x00001000 */
#define RTC_ALRMAR_MNT_1                     (0x2U << RTC_ALRMAR_MNT_Pos)      /*!< 0x00002000 */
#define RTC_ALRMAR_MNT_2                     (0x4U << RTC_ALRMAR_MNT_Pos)      /*!< 0x00004000 */
#define RTC_ALRMAR_MNU_Pos                   (8U)                              
#define RTC_ALRMAR_MNU_Msk                   (0xFU << RTC_ALRMAR_MNU_Pos)      /*!< 0x00000F00 */
#define RTC_ALRMAR_MNU                       RTC_ALRMAR_MNU_Msk                
#define RTC_ALRMAR_MNU_0                     (0x1U << RTC_ALRMAR_MNU_Pos)      /*!< 0x00000100 */
#define RTC_ALRMAR_MNU_1                     (0x2U << RTC_ALRMAR_MNU_Pos)      /*!< 0x00000200 */
#define RTC_ALRMAR_MNU_2                     (0x4U << RTC_ALRMAR_MNU_Pos)      /*!< 0x00000400 */
#define RTC_ALRMAR_MNU_3                     (0x8U << RTC_ALRMAR_MNU_Pos)      /*!< 0x00000800 */
#define RTC_ALRMAR_MSK1_Pos                  (7U)                              
#define RTC_ALRMAR_MSK1_Msk                  (0x1U << RTC_ALRMAR_MSK1_Pos)     /*!< 0x00000080 */
#define RTC_ALRMAR_MSK1                      RTC_ALRMAR_MSK1_Msk               
#define RTC_ALRMAR_ST_Pos                    (4U)                              
#define RTC_ALRMAR_ST_Msk                    (0x7U << RTC_ALRMAR_ST_Pos)       /*!< 0x00000070 */
#define RTC_ALRMAR_ST                        RTC_ALRMAR_ST_Msk                 
#define RTC_ALRMAR_ST_0                      (0x1U << RTC_ALRMAR_ST_Pos)       /*!< 0x00000010 */
#define RTC_ALRMAR_ST_1                      (0x2U << RTC_ALRMAR_ST_Pos)       /*!< 0x00000020 */
#define RTC_ALRMAR_ST_2                      (0x4U << RTC_ALRMAR_ST_Pos)       /*!< 0x00000040 */
#define RTC_ALRMAR_SU_Pos                    (0U)                              
#define RTC_ALRMAR_SU_Msk                    (0xFU << RTC_ALRMAR_SU_Pos)       /*!< 0x0000000F */
#define RTC_ALRMAR_SU                        RTC_ALRMAR_SU_Msk                 
#define RTC_ALRMAR_SU_0                      (0x1U << RTC_ALRMAR_SU_Pos)       /*!< 0x00000001 */
#define RTC_ALRMAR_SU_1                      (0x2U << RTC_ALRMAR_SU_Pos)       /*!< 0x00000002 */
#define RTC_ALRMAR_SU_2                      (0x4U << RTC_ALRMAR_SU_Pos)       /*!< 0x00000004 */
#define RTC_ALRMAR_SU_3                      (0x8U << RTC_ALRMAR_SU_Pos)       /*!< 0x00000008 */

/********************  Bits definition for RTC_ALRMBR register  ***************/
#define RTC_ALRMBR_MSK4_Pos                  (31U)                             
#define RTC_ALRMBR_MSK4_Msk                  (0x1U << RTC_ALRMBR_MSK4_Pos)     /*!< 0x80000000 */
#define RTC_ALRMBR_MSK4                      RTC_ALRMBR_MSK4_Msk               
#define RTC_ALRMBR_WDSEL_Pos                 (30U)                             
#define RTC_ALRMBR_WDSEL_Msk                 (0x1U << RTC_ALRMBR_WDSEL_Pos)    /*!< 0x40000000 */
#define RTC_ALRMBR_WDSEL                     RTC_ALRMBR_WDSEL_Msk              
#define RTC_ALRMBR_DT_Pos                    (28U)                             
#define RTC_ALRMBR_DT_Msk                    (0x3U << RTC_ALRMBR_DT_Pos)       /*!< 0x30000000 */
#define RTC_ALRMBR_DT                        RTC_ALRMBR_DT_Msk                 
#define RTC_ALRMBR_DT_0                      (0x1U << RTC_ALRMBR_DT_Pos)       /*!< 0x10000000 */
#define RTC_ALRMBR_DT_1                      (0x2U << RTC_ALRMBR_DT_Pos)       /*!< 0x20000000 */
#define RTC_ALRMBR_DU_Pos                    (24U)                             
#define RTC_ALRMBR_DU_Msk                    (0xFU << RTC_ALRMBR_DU_Pos)       /*!< 0x0F000000 */
#define RTC_ALRMBR_DU                        RTC_ALRMBR_DU_Msk                 
#define RTC_ALRMBR_DU_0                      (0x1U << RTC_ALRMBR_DU_Pos)       /*!< 0x01000000 */
#define RTC_ALRMBR_DU_1                      (0x2U << RTC_ALRMBR_DU_Pos)       /*!< 0x02000000 */
#define RTC_ALRMBR_DU_2                      (0x4U << RTC_ALRMBR_DU_Pos)       /*!< 0x04000000 */
#define RTC_ALRMBR_DU_3                      (0x8U << RTC_ALRMBR_DU_Pos)       /*!< 0x08000000 */
#define RTC_ALRMBR_MSK3_Pos                  (23U)                             
#define RTC_ALRMBR_MSK3_Msk                  (0x1U << RTC_ALRMBR_MSK3_Pos)     /*!< 0x00800000 */
#define RTC_ALRMBR_MSK3                      RTC_ALRMBR_MSK3_Msk               
#define RTC_ALRMBR_PM_Pos                    (22U)                             
#define RTC_ALRMBR_PM_Msk                    (0x1U << RTC_ALRMBR_PM_Pos)       /*!< 0x00400000 */
#define RTC_ALRMBR_PM                        RTC_ALRMBR_PM_Msk                 
#define RTC_ALRMBR_HT_Pos                    (20U)                             
#define RTC_ALRMBR_HT_Msk                    (0x3U << RTC_ALRMBR_HT_Pos)       /*!< 0x00300000 */
#define RTC_ALRMBR_HT                        RTC_ALRMBR_HT_Msk                 
#define RTC_ALRMBR_HT_0                      (0x1U << RTC_ALRMBR_HT_Pos)       /*!< 0x00100000 */
#define RTC_ALRMBR_HT_1                      (0x2U << RTC_ALRMBR_HT_Pos)       /*!< 0x00200000 */
#define RTC_ALRMBR_HU_Pos                    (16U)                             
#define RTC_ALRMBR_HU_Msk                    (0xFU << RTC_ALRMBR_HU_Pos)       /*!< 0x000F0000 */
#define RTC_ALRMBR_HU                        RTC_ALRMBR_HU_Msk                 
#define RTC_ALRMBR_HU_0                      (0x1U << RTC_ALRMBR_HU_Pos)       /*!< 0x00010000 */
#define RTC_ALRMBR_HU_1                      (0x2U << RTC_ALRMBR_HU_Pos)       /*!< 0x00020000 */
#define RTC_ALRMBR_HU_2                      (0x4U << RTC_ALRMBR_HU_Pos)       /*!< 0x00040000 */
#define RTC_ALRMBR_HU_3                      (0x8U << RTC_ALRMBR_HU_Pos)       /*!< 0x00080000 */
#define RTC_ALRMBR_MSK2_Pos                  (15U)                             
#define RTC_ALRMBR_MSK2_Msk                  (0x1U << RTC_ALRMBR_MSK2_Pos)     /*!< 0x00008000 */
#define RTC_ALRMBR_MSK2                      RTC_ALRMBR_MSK2_Msk               
#define RTC_ALRMBR_MNT_Pos                   (12U)                             
#define RTC_ALRMBR_MNT_Msk                   (0x7U << RTC_ALRMBR_MNT_Pos)      /*!< 0x00007000 */
#define RTC_ALRMBR_MNT                       RTC_ALRMBR_MNT_Msk                
#define RTC_ALRMBR_MNT_0                     (0x1U << RTC_ALRMBR_MNT_Pos)      /*!< 0x00001000 */
#define RTC_ALRMBR_MNT_1                     (0x2U << RTC_ALRMBR_MNT_Pos)      /*!< 0x00002000 */
#define RTC_ALRMBR_MNT_2                     (0x4U << RTC_ALRMBR_MNT_Pos)      /*!< 0x00004000 */
#define RTC_ALRMBR_MNU_Pos                   (8U)                              
#define RTC_ALRMBR_MNU_Msk                   (0xFU << RTC_ALRMBR_MNU_Pos)      /*!< 0x00000F00 */
#define RTC_ALRMBR_MNU                       RTC_ALRMBR_MNU_Msk                
#define RTC_ALRMBR_MNU_0                     (0x1U << RTC_ALRMBR_MNU_Pos)      /*!< 0x00000100 */
#define RTC_ALRMBR_MNU_1                     (0x2U << RTC_ALRMBR_MNU_Pos)      /*!< 0x00000200 */
#define RTC_ALRMBR_MNU_2                     (0x4U << RTC_ALRMBR_MNU_Pos)      /*!< 0x00000400 */
#define RTC_ALRMBR_MNU_3                     (0x8U << RTC_ALRMBR_MNU_Pos)      /*!< 0x00000800 */
#define RTC_ALRMBR_MSK1_Pos                  (7U)                              
#define RTC_ALRMBR_MSK1_Msk                  (0x1U << RTC_ALRMBR_MSK1_Pos)     /*!< 0x00000080 */
#define RTC_ALRMBR_MSK1                      RTC_ALRMBR_MSK1_Msk               
#define RTC_ALRMBR_ST_Pos                    (4U)                              
#define RTC_ALRMBR_ST_Msk                    (0x7U << RTC_ALRMBR_ST_Pos)       /*!< 0x00000070 */
#define RTC_ALRMBR_ST                        RTC_ALRMBR_ST_Msk                 
#define RTC_ALRMBR_ST_0                      (0x1U << RTC_ALRMBR_ST_Pos)       /*!< 0x00000010 */
#define RTC_ALRMBR_ST_1                      (0x2U << RTC_ALRMBR_ST_Pos)       /*!< 0x00000020 */
#define RTC_ALRMBR_ST_2                      (0x4U << RTC_ALRMBR_ST_Pos)       /*!< 0x00000040 */
#define RTC_ALRMBR_SU_Pos                    (0U)                              
#define RTC_ALRMBR_SU_Msk                    (0xFU << RTC_ALRMBR_SU_Pos)       /*!< 0x0000000F */
#define RTC_ALRMBR_SU                        RTC_ALRMBR_SU_Msk                 
#define RTC_ALRMBR_SU_0                      (0x1U << RTC_ALRMBR_SU_Pos)       /*!< 0x00000001 */
#define RTC_ALRMBR_SU_1                      (0x2U << RTC_ALRMBR_SU_Pos)       /*!< 0x00000002 */
#define RTC_ALRMBR_SU_2                      (0x4U << RTC_ALRMBR_SU_Pos)       /*!< 0x00000004 */
#define RTC_ALRMBR_SU_3                      (0x8U << RTC_ALRMBR_SU_Pos)       /*!< 0x00000008 */

/********************  Bits definition for RTC_WPR register  ******************/
#define RTC_WPR_KEY_Pos                      (0U)                              
#define RTC_WPR_KEY_Msk                      (0xFFU << RTC_WPR_KEY_Pos)        /*!< 0x000000FF */
#define RTC_WPR_KEY                          RTC_WPR_KEY_Msk                   

/********************  Bits definition for RTC_SSR register  ******************/
#define RTC_SSR_SS_Pos                       (0U)                              
#define RTC_SSR_SS_Msk                       (0xFFFFU << RTC_SSR_SS_Pos)       /*!< 0x0000FFFF */
#define RTC_SSR_SS                           RTC_SSR_SS_Msk                    

/********************  Bits definition for RTC_SHIFTR register  ***************/
#define RTC_SHIFTR_SUBFS_Pos                 (0U)                              
#define RTC_SHIFTR_SUBFS_Msk                 (0x7FFFU << RTC_SHIFTR_SUBFS_Pos) /*!< 0x00007FFF */
#define RTC_SHIFTR_SUBFS                     RTC_SHIFTR_SUBFS_Msk              
#define RTC_SHIFTR_ADD1S_Pos                 (31U)                             
#define RTC_SHIFTR_ADD1S_Msk                 (0x1U << RTC_SHIFTR_ADD1S_Pos)    /*!< 0x80000000 */
#define RTC_SHIFTR_ADD1S                     RTC_SHIFTR_ADD1S_Msk              

/********************  Bits definition for RTC_TSTR register  *****************/
#define RTC_TSTR_PM_Pos                      (22U)                             
#define RTC_TSTR_PM_Msk                      (0x1U << RTC_TSTR_PM_Pos)         /*!< 0x00400000 */
#define RTC_TSTR_PM                          RTC_TSTR_PM_Msk                   
#define RTC_TSTR_HT_Pos                      (20U)                             
#define RTC_TSTR_HT_Msk                      (0x3U << RTC_TSTR_HT_Pos)         /*!< 0x00300000 */
#define RTC_TSTR_HT                          RTC_TSTR_HT_Msk                   
#define RTC_TSTR_HT_0                        (0x1U << RTC_TSTR_HT_Pos)         /*!< 0x00100000 */
#define RTC_TSTR_HT_1                        (0x2U << RTC_TSTR_HT_Pos)         /*!< 0x00200000 */
#define RTC_TSTR_HU_Pos                      (16U)                             
#define RTC_TSTR_HU_Msk                      (0xFU << RTC_TSTR_HU_Pos)         /*!< 0x000F0000 */
#define RTC_TSTR_HU                          RTC_TSTR_HU_Msk                   
#define RTC_TSTR_HU_0                        (0x1U << RTC_TSTR_HU_Pos)         /*!< 0x00010000 */
#define RTC_TSTR_HU_1                        (0x2U << RTC_TSTR_HU_Pos)         /*!< 0x00020000 */
#define RTC_TSTR_HU_2                        (0x4U << RTC_TSTR_HU_Pos)         /*!< 0x00040000 */
#define RTC_TSTR_HU_3                        (0x8U << RTC_TSTR_HU_Pos)         /*!< 0x00080000 */
#define RTC_TSTR_MNT_Pos                     (12U)                             
#define RTC_TSTR_MNT_Msk                     (0x7U << RTC_TSTR_MNT_Pos)        /*!< 0x00007000 */
#define RTC_TSTR_MNT                         RTC_TSTR_MNT_Msk                  
#define RTC_TSTR_MNT_0                       (0x1U << RTC_TSTR_MNT_Pos)        /*!< 0x00001000 */
#define RTC_TSTR_MNT_1                       (0x2U << RTC_TSTR_MNT_Pos)        /*!< 0x00002000 */
#define RTC_TSTR_MNT_2                       (0x4U << RTC_TSTR_MNT_Pos)        /*!< 0x00004000 */
#define RTC_TSTR_MNU_Pos                     (8U)                              
#define RTC_TSTR_MNU_Msk                     (0xFU << RTC_TSTR_MNU_Pos)        /*!< 0x00000F00 */
#define RTC_TSTR_MNU                         RTC_TSTR_MNU_Msk                  
#define RTC_TSTR_MNU_0                       (0x1U << RTC_TSTR_MNU_Pos)        /*!< 0x00000100 */
#define RTC_TSTR_MNU_1                       (0x2U << RTC_TSTR_MNU_Pos)        /*!< 0x00000200 */
#define RTC_TSTR_MNU_2                       (0x4U << RTC_TSTR_MNU_Pos)        /*!< 0x00000400 */
#define RTC_TSTR_MNU_3                       (0x8U << RTC_TSTR_MNU_Pos)        /*!< 0x00000800 */
#define RTC_TSTR_ST_Pos                      (4U)                              
#define RTC_TSTR_ST_Msk                      (0x7U << RTC_TSTR_ST_Pos)         /*!< 0x00000070 */
#define RTC_TSTR_ST                          RTC_TSTR_ST_Msk                   
#define RTC_TSTR_ST_0                        (0x1U << RTC_TSTR_ST_Pos)         /*!< 0x00000010 */
#define RTC_TSTR_ST_1                        (0x2U << RTC_TSTR_ST_Pos)         /*!< 0x00000020 */
#define RTC_TSTR_ST_2                        (0x4U << RTC_TSTR_ST_Pos)         /*!< 0x00000040 */
#define RTC_TSTR_SU_Pos                      (0U)                              
#define RTC_TSTR_SU_Msk                      (0xFU << RTC_TSTR_SU_Pos)         /*!< 0x0000000F */
#define RTC_TSTR_SU                          RTC_TSTR_SU_Msk                   
#define RTC_TSTR_SU_0                        (0x1U << RTC_TSTR_SU_Pos)         /*!< 0x00000001 */
#define RTC_TSTR_SU_1                        (0x2U << RTC_TSTR_SU_Pos)         /*!< 0x00000002 */
#define RTC_TSTR_SU_2                        (0x4U << RTC_TSTR_SU_Pos)         /*!< 0x00000004 */
#define RTC_TSTR_SU_3                        (0x8U << RTC_TSTR_SU_Pos)         /*!< 0x00000008 */

/********************  Bits definition for RTC_TSDR register  *****************/
#define RTC_TSDR_WDU_Pos                     (13U)                             
#define RTC_TSDR_WDU_Msk                     (0x7U << RTC_TSDR_WDU_Pos)        /*!< 0x0000E000 */
#define RTC_TSDR_WDU                         RTC_TSDR_WDU_Msk                  
#define RTC_TSDR_WDU_0                       (0x1U << RTC_TSDR_WDU_Pos)        /*!< 0x00002000 */
#define RTC_TSDR_WDU_1                       (0x2U << RTC_TSDR_WDU_Pos)        /*!< 0x00004000 */
#define RTC_TSDR_WDU_2                       (0x4U << RTC_TSDR_WDU_Pos)        /*!< 0x00008000 */
#define RTC_TSDR_MT_Pos                      (12U)                             
#define RTC_TSDR_MT_Msk                      (0x1U << RTC_TSDR_MT_Pos)         /*!< 0x00001000 */
#define RTC_TSDR_MT                          RTC_TSDR_MT_Msk                   
#define RTC_TSDR_MU_Pos                      (8U)                              
#define RTC_TSDR_MU_Msk                      (0xFU << RTC_TSDR_MU_Pos)         /*!< 0x00000F00 */
#define RTC_TSDR_MU                          RTC_TSDR_MU_Msk                   
#define RTC_TSDR_MU_0                        (0x1U << RTC_TSDR_MU_Pos)         /*!< 0x00000100 */
#define RTC_TSDR_MU_1                        (0x2U << RTC_TSDR_MU_Pos)         /*!< 0x00000200 */
#define RTC_TSDR_MU_2                        (0x4U << RTC_TSDR_MU_Pos)         /*!< 0x00000400 */
#define RTC_TSDR_MU_3                        (0x8U << RTC_TSDR_MU_Pos)         /*!< 0x00000800 */
#define RTC_TSDR_DT_Pos                      (4U)                              
#define RTC_TSDR_DT_Msk                      (0x3U << RTC_TSDR_DT_Pos)         /*!< 0x00000030 */
#define RTC_TSDR_DT                          RTC_TSDR_DT_Msk                   
#define RTC_TSDR_DT_0                        (0x1U << RTC_TSDR_DT_Pos)         /*!< 0x00000010 */
#define RTC_TSDR_DT_1                        (0x2U << RTC_TSDR_DT_Pos)         /*!< 0x00000020 */
#define RTC_TSDR_DU_Pos                      (0U)                              
#define RTC_TSDR_DU_Msk                      (0xFU << RTC_TSDR_DU_Pos)         /*!< 0x0000000F */
#define RTC_TSDR_DU                          RTC_TSDR_DU_Msk                   
#define RTC_TSDR_DU_0                        (0x1U << RTC_TSDR_DU_Pos)         /*!< 0x00000001 */
#define RTC_TSDR_DU_1                        (0x2U << RTC_TSDR_DU_Pos)         /*!< 0x00000002 */
#define RTC_TSDR_DU_2                        (0x4U << RTC_TSDR_DU_Pos)         /*!< 0x00000004 */
#define RTC_TSDR_DU_3                        (0x8U << RTC_TSDR_DU_Pos)         /*!< 0x00000008 */

/********************  Bits definition for RTC_TSSSR register  ****************/
#define RTC_TSSSR_SS_Pos                     (0U)                              
#define RTC_TSSSR_SS_Msk                     (0xFFFFU << RTC_TSSSR_SS_Pos)     /*!< 0x0000FFFF */
#define RTC_TSSSR_SS                         RTC_TSSSR_SS_Msk                  

/********************  Bits definition for RTC_CAL register  *****************/
#define RTC_CALR_CALP_Pos                    (15U)                             
#define RTC_CALR_CALP_Msk                    (0x1U << RTC_CALR_CALP_Pos)       /*!< 0x00008000 */
#define RTC_CALR_CALP                        RTC_CALR_CALP_Msk                 
#define RTC_CALR_CALW8_Pos                   (14U)                             
#define RTC_CALR_CALW8_Msk                   (0x1U << RTC_CALR_CALW8_Pos)      /*!< 0x00004000 */
#define RTC_CALR_CALW8                       RTC_CALR_CALW8_Msk                
#define RTC_CALR_CALW16_Pos                  (13U)                             
#define RTC_CALR_CALW16_Msk                  (0x1U << RTC_CALR_CALW16_Pos)     /*!< 0x00002000 */
#define RTC_CALR_CALW16                      RTC_CALR_CALW16_Msk               
#define RTC_CALR_CALM_Pos                    (0U)                              
#define RTC_CALR_CALM_Msk                    (0x1FFU << RTC_CALR_CALM_Pos)     /*!< 0x000001FF */
#define RTC_CALR_CALM                        RTC_CALR_CALM_Msk                 
#define RTC_CALR_CALM_0                      (0x001U << RTC_CALR_CALM_Pos)     /*!< 0x00000001 */
#define RTC_CALR_CALM_1                      (0x002U << RTC_CALR_CALM_Pos)     /*!< 0x00000002 */
#define RTC_CALR_CALM_2                      (0x004U << RTC_CALR_CALM_Pos)     /*!< 0x00000004 */
#define RTC_CALR_CALM_3                      (0x008U << RTC_CALR_CALM_Pos)     /*!< 0x00000008 */
#define RTC_CALR_CALM_4                      (0x010U << RTC_CALR_CALM_Pos)     /*!< 0x00000010 */
#define RTC_CALR_CALM_5                      (0x020U << RTC_CALR_CALM_Pos)     /*!< 0x00000020 */
#define RTC_CALR_CALM_6                      (0x040U << RTC_CALR_CALM_Pos)     /*!< 0x00000040 */
#define RTC_CALR_CALM_7                      (0x080U << RTC_CALR_CALM_Pos)     /*!< 0x00000080 */
#define RTC_CALR_CALM_8                      (0x100U << RTC_CALR_CALM_Pos)     /*!< 0x00000100 */

/********************  Bits definition for RTC_TAFCR register  ****************/
#define RTC_TAFCR_ALARMOUTTYPE_Pos           (18U)                             
#define RTC_TAFCR_ALARMOUTTYPE_Msk           (0x1U << RTC_TAFCR_ALARMOUTTYPE_Pos) /*!< 0x00040000 */
#define RTC_TAFCR_ALARMOUTTYPE               RTC_TAFCR_ALARMOUTTYPE_Msk        
#define RTC_TAFCR_TAMPPUDIS_Pos              (15U)                             
#define RTC_TAFCR_TAMPPUDIS_Msk              (0x1U << RTC_TAFCR_TAMPPUDIS_Pos) /*!< 0x00008000 */
#define RTC_TAFCR_TAMPPUDIS                  RTC_TAFCR_TAMPPUDIS_Msk           
#define RTC_TAFCR_TAMPPRCH_Pos               (13U)                             
#define RTC_TAFCR_TAMPPRCH_Msk               (0x3U << RTC_TAFCR_TAMPPRCH_Pos)  /*!< 0x00006000 */
#define RTC_TAFCR_TAMPPRCH                   RTC_TAFCR_TAMPPRCH_Msk            
#define RTC_TAFCR_TAMPPRCH_0                 (0x1U << RTC_TAFCR_TAMPPRCH_Pos)  /*!< 0x00002000 */
#define RTC_TAFCR_TAMPPRCH_1                 (0x2U << RTC_TAFCR_TAMPPRCH_Pos)  /*!< 0x00004000 */
#define RTC_TAFCR_TAMPFLT_Pos                (11U)                             
#define RTC_TAFCR_TAMPFLT_Msk                (0x3U << RTC_TAFCR_TAMPFLT_Pos)   /*!< 0x00001800 */
#define RTC_TAFCR_TAMPFLT                    RTC_TAFCR_TAMPFLT_Msk             
#define RTC_TAFCR_TAMPFLT_0                  (0x1U << RTC_TAFCR_TAMPFLT_Pos)   /*!< 0x00000800 */
#define RTC_TAFCR_TAMPFLT_1                  (0x2U << RTC_TAFCR_TAMPFLT_Pos)   /*!< 0x00001000 */
#define RTC_TAFCR_TAMPFREQ_Pos               (8U)                              
#define RTC_TAFCR_TAMPFREQ_Msk               (0x7U << RTC_TAFCR_TAMPFREQ_Pos)  /*!< 0x00000700 */
#define RTC_TAFCR_TAMPFREQ                   RTC_TAFCR_TAMPFREQ_Msk            
#define RTC_TAFCR_TAMPFREQ_0                 (0x1U << RTC_TAFCR_TAMPFREQ_Pos)  /*!< 0x00000100 */
#define RTC_TAFCR_TAMPFREQ_1                 (0x2U << RTC_TAFCR_TAMPFREQ_Pos)  /*!< 0x00000200 */
#define RTC_TAFCR_TAMPFREQ_2                 (0x4U << RTC_TAFCR_TAMPFREQ_Pos)  /*!< 0x00000400 */
#define RTC_TAFCR_TAMPTS_Pos                 (7U)                              
#define RTC_TAFCR_TAMPTS_Msk                 (0x1U << RTC_TAFCR_TAMPTS_Pos)    /*!< 0x00000080 */
#define RTC_TAFCR_TAMPTS                     RTC_TAFCR_TAMPTS_Msk              
#define RTC_TAFCR_TAMP3TRG_Pos               (6U)                              
#define RTC_TAFCR_TAMP3TRG_Msk               (0x1U << RTC_TAFCR_TAMP3TRG_Pos)  /*!< 0x00000040 */
#define RTC_TAFCR_TAMP3TRG                   RTC_TAFCR_TAMP3TRG_Msk            
#define RTC_TAFCR_TAMP3E_Pos                 (5U)                              
#define RTC_TAFCR_TAMP3E_Msk                 (0x1U << RTC_TAFCR_TAMP3E_Pos)    /*!< 0x00000020 */
#define RTC_TAFCR_TAMP3E                     RTC_TAFCR_TAMP3E_Msk              
#define RTC_TAFCR_TAMP2TRG_Pos               (4U)                              
#define RTC_TAFCR_TAMP2TRG_Msk               (0x1U << RTC_TAFCR_TAMP2TRG_Pos)  /*!< 0x00000010 */
#define RTC_TAFCR_TAMP2TRG                   RTC_TAFCR_TAMP2TRG_Msk            
#define RTC_TAFCR_TAMP2E_Pos                 (3U)                              
#define RTC_TAFCR_TAMP2E_Msk                 (0x1U << RTC_TAFCR_TAMP2E_Pos)    /*!< 0x00000008 */
#define RTC_TAFCR_TAMP2E                     RTC_TAFCR_TAMP2E_Msk              
#define RTC_TAFCR_TAMPIE_Pos                 (2U)                              
#define RTC_TAFCR_TAMPIE_Msk                 (0x1U << RTC_TAFCR_TAMPIE_Pos)    /*!< 0x00000004 */
#define RTC_TAFCR_TAMPIE                     RTC_TAFCR_TAMPIE_Msk              
#define RTC_TAFCR_TAMP1TRG_Pos               (1U)                              
#define RTC_TAFCR_TAMP1TRG_Msk               (0x1U << RTC_TAFCR_TAMP1TRG_Pos)  /*!< 0x00000002 */
#define RTC_TAFCR_TAMP1TRG                   RTC_TAFCR_TAMP1TRG_Msk            
#define RTC_TAFCR_TAMP1E_Pos                 (0U)                              
#define RTC_TAFCR_TAMP1E_Msk                 (0x1U << RTC_TAFCR_TAMP1E_Pos)    /*!< 0x00000001 */
#define RTC_TAFCR_TAMP1E                     RTC_TAFCR_TAMP1E_Msk              

/********************  Bits definition for RTC_ALRMASSR register  *************/
#define RTC_ALRMASSR_MASKSS_Pos              (24U)                             
#define RTC_ALRMASSR_MASKSS_Msk              (0xFU << RTC_ALRMASSR_MASKSS_Pos) /*!< 0x0F000000 */
#define RTC_ALRMASSR_MASKSS                  RTC_ALRMASSR_MASKSS_Msk           
#define RTC_ALRMASSR_MASKSS_0                (0x1U << RTC_ALRMASSR_MASKSS_Pos) /*!< 0x01000000 */
#define RTC_ALRMASSR_MASKSS_1                (0x2U << RTC_ALRMASSR_MASKSS_Pos) /*!< 0x02000000 */
#define RTC_ALRMASSR_MASKSS_2                (0x4U << RTC_ALRMASSR_MASKSS_Pos) /*!< 0x04000000 */
#define RTC_ALRMASSR_MASKSS_3                (0x8U << RTC_ALRMASSR_MASKSS_Pos) /*!< 0x08000000 */
#define RTC_ALRMASSR_SS_Pos                  (0U)                              
#define RTC_ALRMASSR_SS_Msk                  (0x7FFFU << RTC_ALRMASSR_SS_Pos)  /*!< 0x00007FFF */
#define RTC_ALRMASSR_SS                      RTC_ALRMASSR_SS_Msk               

/********************  Bits definition for RTC_ALRMBSSR register  *************/
#define RTC_ALRMBSSR_MASKSS_Pos              (24U)                             
#define RTC_ALRMBSSR_MASKSS_Msk              (0xFU << RTC_ALRMBSSR_MASKSS_Pos) /*!< 0x0F000000 */
#define RTC_ALRMBSSR_MASKSS                  RTC_ALRMBSSR_MASKSS_Msk           
#define RTC_ALRMBSSR_MASKSS_0                (0x1U << RTC_ALRMBSSR_MASKSS_Pos) /*!< 0x01000000 */
#define RTC_ALRMBSSR_MASKSS_1                (0x2U << RTC_ALRMBSSR_MASKSS_Pos) /*!< 0x02000000 */
#define RTC_ALRMBSSR_MASKSS_2                (0x4U << RTC_ALRMBSSR_MASKSS_Pos) /*!< 0x04000000 */
#define RTC_ALRMBSSR_MASKSS_3                (0x8U << RTC_ALRMBSSR_MASKSS_Pos) /*!< 0x08000000 */
#define RTC_ALRMBSSR_SS_Pos                  (0U)                              
#define RTC_ALRMBSSR_SS_Msk                  (0x7FFFU << RTC_ALRMBSSR_SS_Pos)  /*!< 0x00007FFF */
#define RTC_ALRMBSSR_SS                      RTC_ALRMBSSR_SS_Msk               

/********************  Bits definition for RTC_BKP0R register  ****************/
#define RTC_BKP0R_Pos                        (0U)                              
#define RTC_BKP0R_Msk                        (0xFFFFFFFFU << RTC_BKP0R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP0R                            RTC_BKP0R_Msk                     

/********************  Bits definition for RTC_BKP1R register  ****************/
#define RTC_BKP1R_Pos                        (0U)                              
#define RTC_BKP1R_Msk                        (0xFFFFFFFFU << RTC_BKP1R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP1R                            RTC_BKP1R_Msk                     

/********************  Bits definition for RTC_BKP2R register  ****************/
#define RTC_BKP2R_Pos                        (0U)                              
#define RTC_BKP2R_Msk                        (0xFFFFFFFFU << RTC_BKP2R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP2R                            RTC_BKP2R_Msk                     

/********************  Bits definition for RTC_BKP3R register  ****************/
#define RTC_BKP3R_Pos                        (0U)                              
#define RTC_BKP3R_Msk                        (0xFFFFFFFFU << RTC_BKP3R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP3R                            RTC_BKP3R_Msk                     

/********************  Bits definition for RTC_BKP4R register  ****************/
#define RTC_BKP4R_Pos                        (0U)                              
#define RTC_BKP4R_Msk                        (0xFFFFFFFFU << RTC_BKP4R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP4R                            RTC_BKP4R_Msk                     

/********************  Bits definition for RTC_BKP5R register  ****************/
#define RTC_BKP5R_Pos                        (0U)                              
#define RTC_BKP5R_Msk                        (0xFFFFFFFFU << RTC_BKP5R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP5R                            RTC_BKP5R_Msk                     

/********************  Bits definition for RTC_BKP6R register  ****************/
#define RTC_BKP6R_Pos                        (0U)                              
#define RTC_BKP6R_Msk                        (0xFFFFFFFFU << RTC_BKP6R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP6R                            RTC_BKP6R_Msk                     

/********************  Bits definition for RTC_BKP7R register  ****************/
#define RTC_BKP7R_Pos                        (0U)                              
#define RTC_BKP7R_Msk                        (0xFFFFFFFFU << RTC_BKP7R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP7R                            RTC_BKP7R_Msk                     

/********************  Bits definition for RTC_BKP8R register  ****************/
#define RTC_BKP8R_Pos                        (0U)                              
#define RTC_BKP8R_Msk                        (0xFFFFFFFFU << RTC_BKP8R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP8R                            RTC_BKP8R_Msk                     

/********************  Bits definition for RTC_BKP9R register  ****************/
#define RTC_BKP9R_Pos                        (0U)                              
#define RTC_BKP9R_Msk                        (0xFFFFFFFFU << RTC_BKP9R_Pos)    /*!< 0xFFFFFFFF */
#define RTC_BKP9R                            RTC_BKP9R_Msk                     

/********************  Bits definition for RTC_BKP10R register  ***************/
#define RTC_BKP10R_Pos                       (0U)                              
#define RTC_BKP10R_Msk                       (0xFFFFFFFFU << RTC_BKP10R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP10R                           RTC_BKP10R_Msk                    

/********************  Bits definition for RTC_BKP11R register  ***************/
#define RTC_BKP11R_Pos                       (0U)                              
#define RTC_BKP11R_Msk                       (0xFFFFFFFFU << RTC_BKP11R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP11R                           RTC_BKP11R_Msk                    

/********************  Bits definition for RTC_BKP12R register  ***************/
#define RTC_BKP12R_Pos                       (0U)                              
#define RTC_BKP12R_Msk                       (0xFFFFFFFFU << RTC_BKP12R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP12R                           RTC_BKP12R_Msk                    

/********************  Bits definition for RTC_BKP13R register  ***************/
#define RTC_BKP13R_Pos                       (0U)                              
#define RTC_BKP13R_Msk                       (0xFFFFFFFFU << RTC_BKP13R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP13R                           RTC_BKP13R_Msk                    

/********************  Bits definition for RTC_BKP14R register  ***************/
#define RTC_BKP14R_Pos                       (0U)                              
#define RTC_BKP14R_Msk                       (0xFFFFFFFFU << RTC_BKP14R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP14R                           RTC_BKP14R_Msk                    

/********************  Bits definition for RTC_BKP15R register  ***************/
#define RTC_BKP15R_Pos                       (0U)                              
#define RTC_BKP15R_Msk                       (0xFFFFFFFFU << RTC_BKP15R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP15R                           RTC_BKP15R_Msk                    

/********************  Bits definition for RTC_BKP16R register  ***************/
#define RTC_BKP16R_Pos                       (0U)                              
#define RTC_BKP16R_Msk                       (0xFFFFFFFFU << RTC_BKP16R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP16R                           RTC_BKP16R_Msk                    

/********************  Bits definition for RTC_BKP17R register  ***************/
#define RTC_BKP17R_Pos                       (0U)                              
#define RTC_BKP17R_Msk                       (0xFFFFFFFFU << RTC_BKP17R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP17R                           RTC_BKP17R_Msk                    

/********************  Bits definition for RTC_BKP18R register  ***************/
#define RTC_BKP18R_Pos                       (0U)                              
#define RTC_BKP18R_Msk                       (0xFFFFFFFFU << RTC_BKP18R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP18R                           RTC_BKP18R_Msk                    

/********************  Bits definition for RTC_BKP19R register  ***************/
#define RTC_BKP19R_Pos                       (0U)                              
#define RTC_BKP19R_Msk                       (0xFFFFFFFFU << RTC_BKP19R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP19R                           RTC_BKP19R_Msk                    

/********************  Bits definition for RTC_BKP20R register  ***************/
#define RTC_BKP20R_Pos                       (0U)                              
#define RTC_BKP20R_Msk                       (0xFFFFFFFFU << RTC_BKP20R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP20R                           RTC_BKP20R_Msk                    

/********************  Bits definition for RTC_BKP21R register  ***************/
#define RTC_BKP21R_Pos                       (0U)                              
#define RTC_BKP21R_Msk                       (0xFFFFFFFFU << RTC_BKP21R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP21R                           RTC_BKP21R_Msk                    

/********************  Bits definition for RTC_BKP22R register  ***************/
#define RTC_BKP22R_Pos                       (0U)                              
#define RTC_BKP22R_Msk                       (0xFFFFFFFFU << RTC_BKP22R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP22R                           RTC_BKP22R_Msk                    

/********************  Bits definition for RTC_BKP23R register  ***************/
#define RTC_BKP23R_Pos                       (0U)                              
#define RTC_BKP23R_Msk                       (0xFFFFFFFFU << RTC_BKP23R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP23R                           RTC_BKP23R_Msk                    

/********************  Bits definition for RTC_BKP24R register  ***************/
#define RTC_BKP24R_Pos                       (0U)                              
#define RTC_BKP24R_Msk                       (0xFFFFFFFFU << RTC_BKP24R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP24R                           RTC_BKP24R_Msk                    

/********************  Bits definition for RTC_BKP25R register  ***************/
#define RTC_BKP25R_Pos                       (0U)                              
#define RTC_BKP25R_Msk                       (0xFFFFFFFFU << RTC_BKP25R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP25R                           RTC_BKP25R_Msk                    

/********************  Bits definition for RTC_BKP26R register  ***************/
#define RTC_BKP26R_Pos                       (0U)                              
#define RTC_BKP26R_Msk                       (0xFFFFFFFFU << RTC_BKP26R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP26R                           RTC_BKP26R_Msk                    

/********************  Bits definition for RTC_BKP27R register  ***************/
#define RTC_BKP27R_Pos                       (0U)                              
#define RTC_BKP27R_Msk                       (0xFFFFFFFFU << RTC_BKP27R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP27R                           RTC_BKP27R_Msk                    

/********************  Bits definition for RTC_BKP28R register  ***************/
#define RTC_BKP28R_Pos                       (0U)                              
#define RTC_BKP28R_Msk                       (0xFFFFFFFFU << RTC_BKP28R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP28R                           RTC_BKP28R_Msk                    

/********************  Bits definition for RTC_BKP29R register  ***************/
#define RTC_BKP29R_Pos                       (0U)                              
#define RTC_BKP29R_Msk                       (0xFFFFFFFFU << RTC_BKP29R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP29R                           RTC_BKP29R_Msk                    

/********************  Bits definition for RTC_BKP30R register  ***************/
#define RTC_BKP30R_Pos                       (0U)                              
#define RTC_BKP30R_Msk                       (0xFFFFFFFFU << RTC_BKP30R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP30R                           RTC_BKP30R_Msk                    

/********************  Bits definition for RTC_BKP31R register  ***************/
#define RTC_BKP31R_Pos                       (0U)                              
#define RTC_BKP31R_Msk                       (0xFFFFFFFFU << RTC_BKP31R_Pos)   /*!< 0xFFFFFFFF */
#define RTC_BKP31R                           RTC_BKP31R_Msk                    

/******************** Number of backup registers ******************************/
#define RTC_BKP_NUMBER 32

/******************************************************************************/
/*                                                                            */
/*                          SD host Interface                                 */
/*                                                                            */
/******************************************************************************/

/******************  Bit definition for SDIO_POWER register  ******************/
#define SDIO_POWER_PWRCTRL_Pos              (0U)                               
#define SDIO_POWER_PWRCTRL_Msk              (0x3U << SDIO_POWER_PWRCTRL_Pos)   /*!< 0x00000003 */
#define SDIO_POWER_PWRCTRL                  SDIO_POWER_PWRCTRL_Msk             /*!< PWRCTRL[1:0] bits (Power supply control bits) */
#define SDIO_POWER_PWRCTRL_0                (0x1U << SDIO_POWER_PWRCTRL_Pos)   /*!< 0x00000001 */
#define SDIO_POWER_PWRCTRL_1                (0x2U << SDIO_POWER_PWRCTRL_Pos)   /*!< 0x00000002 */

/******************  Bit definition for SDIO_CLKCR register  ******************/
#define SDIO_CLKCR_CLKDIV_Pos               (0U)                               
#define SDIO_CLKCR_CLKDIV_Msk               (0xFFU << SDIO_CLKCR_CLKDIV_Pos)   /*!< 0x000000FF */
#define SDIO_CLKCR_CLKDIV                   SDIO_CLKCR_CLKDIV_Msk              /*!< Clock divide factor */
#define SDIO_CLKCR_CLKEN_Pos                (8U)                               
#define SDIO_CLKCR_CLKEN_Msk                (0x1U << SDIO_CLKCR_CLKEN_Pos)     /*!< 0x00000100 */
#define SDIO_CLKCR_CLKEN                    SDIO_CLKCR_CLKEN_Msk               /*!< Clock enable bit */
#define SDIO_CLKCR_PWRSAV_Pos               (9U)                               
#define SDIO_CLKCR_PWRSAV_Msk               (0x1U << SDIO_CLKCR_PWRSAV_Pos)    /*!< 0x00000200 */
#define SDIO_CLKCR_PWRSAV                   SDIO_CLKCR_PWRSAV_Msk              /*!< Power saving configuration bit */
#define SDIO_CLKCR_BYPASS_Pos               (10U)                              
#define SDIO_CLKCR_BYPASS_Msk               (0x1U << SDIO_CLKCR_BYPASS_Pos)    /*!< 0x00000400 */
#define SDIO_CLKCR_BYPASS                   SDIO_CLKCR_BYPASS_Msk              /*!< Clock divider bypass enable bit */

#define SDIO_CLKCR_WIDBUS_Pos               (11U)                              
#define SDIO_CLKCR_WIDBUS_Msk               (0x3U << SDIO_CLKCR_WIDBUS_Pos)    /*!< 0x00001800 */
#define SDIO_CLKCR_WIDBUS                   SDIO_CLKCR_WIDBUS_Msk              /*!< WIDBUS[1:0] bits (Wide bus mode enable bit) */
#define SDIO_CLKCR_WIDBUS_0                 (0x1U << SDIO_CLKCR_WIDBUS_Pos)    /*!< 0x00000800 */
#define SDIO_CLKCR_WIDBUS_1                 (0x2U << SDIO_CLKCR_WIDBUS_Pos)    /*!< 0x00001000 */

#define SDIO_CLKCR_NEGEDGE_Pos              (13U)                              
#define SDIO_CLKCR_NEGEDGE_Msk              (0x1U << SDIO_CLKCR_NEGEDGE_Pos)   /*!< 0x00002000 */
#define SDIO_CLKCR_NEGEDGE                  SDIO_CLKCR_NEGEDGE_Msk             /*!< SDIO_CK dephasing selection bit */
#define SDIO_CLKCR_HWFC_EN_Pos              (14U)                              
#define SDIO_CLKCR_HWFC_EN_Msk              (0x1U << SDIO_CLKCR_HWFC_EN_Pos)   /*!< 0x00004000 */
#define SDIO_CLKCR_HWFC_EN                  SDIO_CLKCR_HWFC_EN_Msk             /*!< HW Flow Control enable */

/*******************  Bit definition for SDIO_ARG register  *******************/
#define SDIO_ARG_CMDARG_Pos                 (0U)                               
#define SDIO_ARG_CMDARG_Msk                 (0xFFFFFFFFU << SDIO_ARG_CMDARG_Pos) /*!< 0xFFFFFFFF */
#define SDIO_ARG_CMDARG                     SDIO_ARG_CMDARG_Msk                /*!< Command argument */

/*******************  Bit definition for SDIO_CMD register  *******************/
#define SDIO_CMD_CMDINDEX_Pos               (0U)                               
#define SDIO_CMD_CMDINDEX_Msk               (0x3FU << SDIO_CMD_CMDINDEX_Pos)   /*!< 0x0000003F */
#define SDIO_CMD_CMDINDEX                   SDIO_CMD_CMDINDEX_Msk              /*!< Command Index */

#define SDIO_CMD_WAITRESP_Pos               (6U)                               
#define SDIO_CMD_WAITRESP_Msk               (0x3U << SDIO_CMD_WAITRESP_Pos)    /*!< 0x000000C0 */
#define SDIO_CMD_WAITRESP                   SDIO_CMD_WAITRESP_Msk              /*!< WAITRESP[1:0] bits (Wait for response bits) */
#define SDIO_CMD_WAITRESP_0                 (0x1U << SDIO_CMD_WAITRESP_Pos)    /*!< 0x00000040 */
#define SDIO_CMD_WAITRESP_1                 (0x2U << SDIO_CMD_WAITRESP_Pos)    /*!< 0x00000080 */

#define SDIO_CMD_WAITINT_Pos                (8U)                               
#define SDIO_CMD_WAITINT_Msk                (0x1U << SDIO_CMD_WAITINT_Pos)     /*!< 0x00000100 */
#define SDIO_CMD_WAITINT                    SDIO_CMD_WAITINT_Msk               /*!< CPSM Waits for Interrupt Request */
#define SDIO_CMD_WAITPEND_Pos               (9U)                               
#define SDIO_CMD_WAITPEND_Msk               (0x1U << SDIO_CMD_WAITPEND_Pos)    /*!< 0x00000200 */
#define SDIO_CMD_WAITPEND                   SDIO_CMD_WAITPEND_Msk              /*!< CPSM Waits for ends of data transfer (CmdPend internal signal) */
#define SDIO_CMD_CPSMEN_Pos                 (10U)                              
#define SDIO_CMD_CPSMEN_Msk                 (0x1U << SDIO_CMD_CPSMEN_Pos)      /*!< 0x00000400 */
#define SDIO_CMD_CPSMEN                     SDIO_CMD_CPSMEN_Msk                /*!< Command path state machine (CPSM) Enable bit */
#define SDIO_CMD_SDIOSUSPEND_Pos            (11U)                              
#define SDIO_CMD_SDIOSUSPEND_Msk            (0x1U << SDIO_CMD_SDIOSUSPEND_Pos) /*!< 0x00000800 */
#define SDIO_CMD_SDIOSUSPEND                SDIO_CMD_SDIOSUSPEND_Msk           /*!< SD I/O suspend command */
#define SDIO_CMD_ENCMDCOMPL_Pos             (12U)                              
#define SDIO_CMD_ENCMDCOMPL_Msk             (0x1U << SDIO_CMD_ENCMDCOMPL_Pos)  /*!< 0x00001000 */
#define SDIO_CMD_ENCMDCOMPL                 SDIO_CMD_ENCMDCOMPL_Msk            /*!< Enable CMD completion */
#define SDIO_CMD_NIEN_Pos                   (13U)                              
#define SDIO_CMD_NIEN_Msk                   (0x1U << SDIO_CMD_NIEN_Pos)        /*!< 0x00002000 */
#define SDIO_CMD_NIEN                       SDIO_CMD_NIEN_Msk                  /*!< Not Interrupt Enable */
#define SDIO_CMD_CEATACMD_Pos               (14U)                              
#define SDIO_CMD_CEATACMD_Msk               (0x1U << SDIO_CMD_CEATACMD_Pos)    /*!< 0x00004000 */
#define SDIO_CMD_CEATACMD                   SDIO_CMD_CEATACMD_Msk              /*!< CE-ATA command */

/*****************  Bit definition for SDIO_RESPCMD register  *****************/
#define SDIO_RESPCMD_RESPCMD_Pos            (0U)                               
#define SDIO_RESPCMD_RESPCMD_Msk            (0x3FU << SDIO_RESPCMD_RESPCMD_Pos) /*!< 0x0000003F */
#define SDIO_RESPCMD_RESPCMD                SDIO_RESPCMD_RESPCMD_Msk           /*!< Response command index */

/******************  Bit definition for SDIO_RESP0 register  ******************/
#define SDIO_RESP0_CARDSTATUS0_Pos          (0U)                               
#define SDIO_RESP0_CARDSTATUS0_Msk          (0xFFFFFFFFU << SDIO_RESP0_CARDSTATUS0_Pos) /*!< 0xFFFFFFFF */
#define SDIO_RESP0_CARDSTATUS0              SDIO_RESP0_CARDSTATUS0_Msk         /*!< Card Status */

/******************  Bit definition for SDIO_RESP1 register  ******************/
#define SDIO_RESP1_CARDSTATUS1_Pos          (0U)                               
#define SDIO_RESP1_CARDSTATUS1_Msk          (0xFFFFFFFFU << SDIO_RESP1_CARDSTATUS1_Pos) /*!< 0xFFFFFFFF */
#define SDIO_RESP1_CARDSTATUS1              SDIO_RESP1_CARDSTATUS1_Msk         /*!< Card Status */

/******************  Bit definition for SDIO_RESP2 register  ******************/
#define SDIO_RESP2_CARDSTATUS2_Pos          (0U)                               
#define SDIO_RESP2_CARDSTATUS2_Msk          (0xFFFFFFFFU << SDIO_RESP2_CARDSTATUS2_Pos) /*!< 0xFFFFFFFF */
#define SDIO_RESP2_CARDSTATUS2              SDIO_RESP2_CARDSTATUS2_Msk         /*!< Card Status */

/******************  Bit definition for SDIO_RESP3 register  ******************/
#define SDIO_RESP3_CARDSTATUS3_Pos          (0U)                               
#define SDIO_RESP3_CARDSTATUS3_Msk          (0xFFFFFFFFU << SDIO_RESP3_CARDSTATUS3_Pos) /*!< 0xFFFFFFFF */
#define SDIO_RESP3_CARDSTATUS3              SDIO_RESP3_CARDSTATUS3_Msk         /*!< Card Status */

/******************  Bit definition for SDIO_RESP4 register  ******************/
#define SDIO_RESP4_CARDSTATUS4_Pos          (0U)                               
#define SDIO_RESP4_CARDSTATUS4_Msk          (0xFFFFFFFFU << SDIO_RESP4_CARDSTATUS4_Pos) /*!< 0xFFFFFFFF */
#define SDIO_RESP4_CARDSTATUS4              SDIO_RESP4_CARDSTATUS4_Msk         /*!< Card Status */

/******************  Bit definition for SDIO_DTIMER register  *****************/
#define SDIO_DTIMER_DATATIME_Pos            (0U)                               
#define SDIO_DTIMER_DATATIME_Msk            (0xFFFFFFFFU << SDIO_DTIMER_DATATIME_Pos) /*!< 0xFFFFFFFF */
#define SDIO_DTIMER_DATATIME                SDIO_DTIMER_DATATIME_Msk           /*!< Data timeout period. */

/******************  Bit definition for SDIO_DLEN register  *******************/
#define SDIO_DLEN_DATALENGTH_Pos            (0U)                               
#define SDIO_DLEN_DATALENGTH_Msk            (0x1FFFFFFU << SDIO_DLEN_DATALENGTH_Pos) /*!< 0x01FFFFFF */
#define SDIO_DLEN_DATALENGTH                SDIO_DLEN_DATALENGTH_Msk           /*!< Data length value */

/******************  Bit definition for SDIO_DCTRL register  ******************/
#define SDIO_DCTRL_DTEN_Pos                 (0U)                               
#define SDIO_DCTRL_DTEN_Msk                 (0x1U << SDIO_DCTRL_DTEN_Pos)      /*!< 0x00000001 */
#define SDIO_DCTRL_DTEN                     SDIO_DCTRL_DTEN_Msk                /*!< Data transfer enabled bit */
#define SDIO_DCTRL_DTDIR_Pos                (1U)                               
#define SDIO_DCTRL_DTDIR_Msk                (0x1U << SDIO_DCTRL_DTDIR_Pos)     /*!< 0x00000002 */
#define SDIO_DCTRL_DTDIR                    SDIO_DCTRL_DTDIR_Msk               /*!< Data transfer direction selection */
#define SDIO_DCTRL_DTMODE_Pos               (2U)                               
#define SDIO_DCTRL_DTMODE_Msk               (0x1U << SDIO_DCTRL_DTMODE_Pos)    /*!< 0x00000004 */
#define SDIO_DCTRL_DTMODE                   SDIO_DCTRL_DTMODE_Msk              /*!< Data transfer mode selection */
#define SDIO_DCTRL_DMAEN_Pos                (3U)                               
#define SDIO_DCTRL_DMAEN_Msk                (0x1U << SDIO_DCTRL_DMAEN_Pos)     /*!< 0x00000008 */
#define SDIO_DCTRL_DMAEN                    SDIO_DCTRL_DMAEN_Msk               /*!< DMA enabled bit */

#define SDIO_DCTRL_DBLOCKSIZE_Pos           (4U)                               
#define SDIO_DCTRL_DBLOCKSIZE_Msk           (0xFU << SDIO_DCTRL_DBLOCKSIZE_Pos) /*!< 0x000000F0 */
#define SDIO_DCTRL_DBLOCKSIZE               SDIO_DCTRL_DBLOCKSIZE_Msk          /*!< DBLOCKSIZE[3:0] bits (Data block size) */
#define SDIO_DCTRL_DBLOCKSIZE_0             (0x1U << SDIO_DCTRL_DBLOCKSIZE_Pos) /*!< 0x00000010 */
#define SDIO_DCTRL_DBLOCKSIZE_1             (0x2U << SDIO_DCTRL_DBLOCKSIZE_Pos) /*!< 0x00000020 */
#define SDIO_DCTRL_DBLOCKSIZE_2             (0x4U << SDIO_DCTRL_DBLOCKSIZE_Pos) /*!< 0x00000040 */
#define SDIO_DCTRL_DBLOCKSIZE_3             (0x8U << SDIO_DCTRL_DBLOCKSIZE_Pos) /*!< 0x00000080 */

#define SDIO_DCTRL_RWSTART_Pos              (8U)                               
#define SDIO_DCTRL_RWSTART_Msk              (0x1U << SDIO_DCTRL_RWSTART_Pos)   /*!< 0x00000100 */
#define SDIO_DCTRL_RWSTART                  SDIO_DCTRL_RWSTART_Msk             /*!< Read wait start */
#define SDIO_DCTRL_RWSTOP_Pos               (9U)                               
#define SDIO_DCTRL_RWSTOP_Msk               (0x1U << SDIO_DCTRL_RWSTOP_Pos)    /*!< 0x00000200 */
#define SDIO_DCTRL_RWSTOP                   SDIO_DCTRL_RWSTOP_Msk              /*!< Read wait stop */
#define SDIO_DCTRL_RWMOD_Pos                (10U)                              
#define SDIO_DCTRL_RWMOD_Msk                (0x1U << SDIO_DCTRL_RWMOD_Pos)     /*!< 0x00000400 */
#define SDIO_DCTRL_RWMOD                    SDIO_DCTRL_RWMOD_Msk               /*!< Read wait mode */
#define SDIO_DCTRL_SDIOEN_Pos               (11U)                              
#define SDIO_DCTRL_SDIOEN_Msk               (0x1U << SDIO_DCTRL_SDIOEN_Pos)    /*!< 0x00000800 */
#define SDIO_DCTRL_SDIOEN                   SDIO_DCTRL_SDIOEN_Msk              /*!< SD I/O enable functions */

/******************  Bit definition for SDIO_DCOUNT register  *****************/
#define SDIO_DCOUNT_DATACOUNT_Pos           (0U)                               
#define SDIO_DCOUNT_DATACOUNT_Msk           (0x1FFFFFFU << SDIO_DCOUNT_DATACOUNT_Pos) /*!< 0x01FFFFFF */
#define SDIO_DCOUNT_DATACOUNT               SDIO_DCOUNT_DATACOUNT_Msk          /*!< Data count value */

/******************  Bit definition for SDIO_STA register  ********************/
#define SDIO_STA_CCRCFAIL_Pos               (0U)                               
#define SDIO_STA_CCRCFAIL_Msk               (0x1U << SDIO_STA_CCRCFAIL_Pos)    /*!< 0x00000001 */
#define SDIO_STA_CCRCFAIL                   SDIO_STA_CCRCFAIL_Msk              /*!< Command response received (CRC check failed) */
#define SDIO_STA_DCRCFAIL_Pos               (1U)                               
#define SDIO_STA_DCRCFAIL_Msk               (0x1U << SDIO_STA_DCRCFAIL_Pos)    /*!< 0x00000002 */
#define SDIO_STA_DCRCFAIL                   SDIO_STA_DCRCFAIL_Msk              /*!< Data block sent/received (CRC check failed) */
#define SDIO_STA_CTIMEOUT_Pos               (2U)                               
#define SDIO_STA_CTIMEOUT_Msk               (0x1U << SDIO_STA_CTIMEOUT_Pos)    /*!< 0x00000004 */
#define SDIO_STA_CTIMEOUT                   SDIO_STA_CTIMEOUT_Msk              /*!< Command response timeout */
#define SDIO_STA_DTIMEOUT_Pos               (3U)                               
#define SDIO_STA_DTIMEOUT_Msk               (0x1U << SDIO_STA_DTIMEOUT_Pos)    /*!< 0x00000008 */
#define SDIO_STA_DTIMEOUT                   SDIO_STA_DTIMEOUT_Msk              /*!< Data timeout */
#define SDIO_STA_TXUNDERR_Pos               (4U)                               
#define SDIO_STA_TXUNDERR_Msk               (0x1U << SDIO_STA_TXUNDERR_Pos)    /*!< 0x00000010 */
#define SDIO_STA_TXUNDERR                   SDIO_STA_TXUNDERR_Msk              /*!< Transmit FIFO underrun error */
#define SDIO_STA_RXOVERR_Pos                (5U)                               
#define SDIO_STA_RXOVERR_Msk                (0x1U << SDIO_STA_RXOVERR_Pos)     /*!< 0x00000020 */
#define SDIO_STA_RXOVERR                    SDIO_STA_RXOVERR_Msk               /*!< Received FIFO overrun error */
#define SDIO_STA_CMDREND_Pos                (6U)                               
#define SDIO_STA_CMDREND_Msk                (0x1U << SDIO_STA_CMDREND_Pos)     /*!< 0x00000040 */
#define SDIO_STA_CMDREND                    SDIO_STA_CMDREND_Msk               /*!< Command response received (CRC check passed) */
#define SDIO_STA_CMDSENT_Pos                (7U)                               
#define SDIO_STA_CMDSENT_Msk                (0x1U << SDIO_STA_CMDSENT_Pos)     /*!< 0x00000080 */
#define SDIO_STA_CMDSENT                    SDIO_STA_CMDSENT_Msk               /*!< Command sent (no response required) */
#define SDIO_STA_DATAEND_Pos                (8U)                               
#define SDIO_STA_DATAEND_Msk                (0x1U << SDIO_STA_DATAEND_Pos)     /*!< 0x00000100 */
#define SDIO_STA_DATAEND                    SDIO_STA_DATAEND_Msk               /*!< Data end (data counter, SDIDCOUNT, is zero) */
#define SDIO_STA_STBITERR_Pos               (9U)                               
#define SDIO_STA_STBITERR_Msk               (0x1U << SDIO_STA_STBITERR_Pos)    /*!< 0x00000200 */
#define SDIO_STA_STBITERR                   SDIO_STA_STBITERR_Msk              /*!< Start bit not detected on all data signals in wide bus mode */
#define SDIO_STA_DBCKEND_Pos                (10U)                              
#define SDIO_STA_DBCKEND_Msk                (0x1U << SDIO_STA_DBCKEND_Pos)     /*!< 0x00000400 */
#define SDIO_STA_DBCKEND                    SDIO_STA_DBCKEND_Msk               /*!< Data block sent/received (CRC check passed) */
#define SDIO_STA_CMDACT_Pos                 (11U)                              
#define SDIO_STA_CMDACT_Msk                 (0x1U << SDIO_STA_CMDACT_Pos)      /*!< 0x00000800 */
#define SDIO_STA_CMDACT                     SDIO_STA_CMDACT_Msk                /*!< Command transfer in progress */
#define SDIO_STA_TXACT_Pos                  (12U)                              
#define SDIO_STA_TXACT_Msk                  (0x1U << SDIO_STA_TXACT_Pos)       /*!< 0x00001000 */
#define SDIO_STA_TXACT                      SDIO_STA_TXACT_Msk                 /*!< Data transmit in progress */
#define SDIO_STA_RXACT_Pos                  (13U)                              
#define SDIO_STA_RXACT_Msk                  (0x1U << SDIO_STA_RXACT_Pos)       /*!< 0x00002000 */
#define SDIO_STA_RXACT                      SDIO_STA_RXACT_Msk                 /*!< Data receive in progress */
#define SDIO_STA_TXFIFOHE_Pos               (14U)                              
#define SDIO_STA_TXFIFOHE_Msk               (0x1U << SDIO_STA_TXFIFOHE_Pos)    /*!< 0x00004000 */
#define SDIO_STA_TXFIFOHE                   SDIO_STA_TXFIFOHE_Msk              /*!< Transmit FIFO Half Empty: at least 8 words can be written into the FIFO */
#define SDIO_STA_RXFIFOHF_Pos               (15U)                              
#define SDIO_STA_RXFIFOHF_Msk               (0x1U << SDIO_STA_RXFIFOHF_Pos)    /*!< 0x00008000 */
#define SDIO_STA_RXFIFOHF                   SDIO_STA_RXFIFOHF_Msk              /*!< Receive FIFO Half Full: there are at least 8 words in the FIFO */
#define SDIO_STA_TXFIFOF_Pos                (16U)                              
#define SDIO_STA_TXFIFOF_Msk                (0x1U << SDIO_STA_TXFIFOF_Pos)     /*!< 0x00010000 */
#define SDIO_STA_TXFIFOF                    SDIO_STA_TXFIFOF_Msk               /*!< Transmit FIFO full */
#define SDIO_STA_RXFIFOF_Pos                (17U)                              
#define SDIO_STA_RXFIFOF_Msk                (0x1U << SDIO_STA_RXFIFOF_Pos)     /*!< 0x00020000 */
#define SDIO_STA_RXFIFOF                    SDIO_STA_RXFIFOF_Msk               /*!< Receive FIFO full */
#define SDIO_STA_TXFIFOE_Pos                (18U)                              
#define SDIO_STA_TXFIFOE_Msk                (0x1U << SDIO_STA_TXFIFOE_Pos)     /*!< 0x00040000 */
#define SDIO_STA_TXFIFOE                    SDIO_STA_TXFIFOE_Msk               /*!< Transmit FIFO empty */
#define SDIO_STA_RXFIFOE_Pos                (19U)                              
#define SDIO_STA_RXFIFOE_Msk                (0x1U << SDIO_STA_RXFIFOE_Pos)     /*!< 0x00080000 */
#define SDIO_STA_RXFIFOE                    SDIO_STA_RXFIFOE_Msk               /*!< Receive FIFO empty */
#define SDIO_STA_TXDAVL_Pos                 (20U)                              
#define SDIO_STA_TXDAVL_Msk                 (0x1U << SDIO_STA_TXDAVL_Pos)      /*!< 0x00100000 */
#define SDIO_STA_TXDAVL                     SDIO_STA_TXDAVL_Msk                /*!< Data available in transmit FIFO */
#define SDIO_STA_RXDAVL_Pos                 (21U)                              
#define SDIO_STA_RXDAVL_Msk                 (0x1U << SDIO_STA_RXDAVL_Pos)      /*!< 0x00200000 */
#define SDIO_STA_RXDAVL                     SDIO_STA_RXDAVL_Msk                /*!< Data available in receive FIFO */
#define SDIO_STA_SDIOIT_Pos                 (22U)                              
#define SDIO_STA_SDIOIT_Msk                 (0x1U << SDIO_STA_SDIOIT_Pos)      /*!< 0x00400000 */
#define SDIO_STA_SDIOIT                     SDIO_STA_SDIOIT_Msk                /*!< SDIO interrupt received */
#define SDIO_STA_CEATAEND_Pos               (23U)                              
#define SDIO_STA_CEATAEND_Msk               (0x1U << SDIO_STA_CEATAEND_Pos)    /*!< 0x00800000 */
#define SDIO_STA_CEATAEND                   SDIO_STA_CEATAEND_Msk              /*!< CE-ATA command completion signal received for CMD61 */

/*******************  Bit definition for SDIO_ICR register  *******************/
#define SDIO_ICR_CCRCFAILC_Pos              (0U)                               
#define SDIO_ICR_CCRCFAILC_Msk              (0x1U << SDIO_ICR_CCRCFAILC_Pos)   /*!< 0x00000001 */
#define SDIO_ICR_CCRCFAILC                  SDIO_ICR_CCRCFAILC_Msk             /*!< CCRCFAIL flag clear bit */
#define SDIO_ICR_DCRCFAILC_Pos              (1U)                               
#define SDIO_ICR_DCRCFAILC_Msk              (0x1U << SDIO_ICR_DCRCFAILC_Pos)   /*!< 0x00000002 */
#define SDIO_ICR_DCRCFAILC                  SDIO_ICR_DCRCFAILC_Msk             /*!< DCRCFAIL flag clear bit */
#define SDIO_ICR_CTIMEOUTC_Pos              (2U)                               
#define SDIO_ICR_CTIMEOUTC_Msk              (0x1U << SDIO_ICR_CTIMEOUTC_Pos)   /*!< 0x00000004 */
#define SDIO_ICR_CTIMEOUTC                  SDIO_ICR_CTIMEOUTC_Msk             /*!< CTIMEOUT flag clear bit */
#define SDIO_ICR_DTIMEOUTC_Pos              (3U)                               
#define SDIO_ICR_DTIMEOUTC_Msk              (0x1U << SDIO_ICR_DTIMEOUTC_Pos)   /*!< 0x00000008 */
#define SDIO_ICR_DTIMEOUTC                  SDIO_ICR_DTIMEOUTC_Msk             /*!< DTIMEOUT flag clear bit */
#define SDIO_ICR_TXUNDERRC_Pos              (4U)                               
#define SDIO_ICR_TXUNDERRC_Msk              (0x1U << SDIO_ICR_TXUNDERRC_Pos)   /*!< 0x00000010 */
#define SDIO_ICR_TXUNDERRC                  SDIO_ICR_TXUNDERRC_Msk             /*!< TXUNDERR flag clear bit */
#define SDIO_ICR_RXOVERRC_Pos               (5U)                               
#define SDIO_ICR_RXOVERRC_Msk               (0x1U << SDIO_ICR_RXOVERRC_Pos)    /*!< 0x00000020 */
#define SDIO_ICR_RXOVERRC                   SDIO_ICR_RXOVERRC_Msk              /*!< RXOVERR flag clear bit */
#define SDIO_ICR_CMDRENDC_Pos               (6U)                               
#define SDIO_ICR_CMDRENDC_Msk               (0x1U << SDIO_ICR_CMDRENDC_Pos)    /*!< 0x00000040 */
#define SDIO_ICR_CMDRENDC                   SDIO_ICR_CMDRENDC_Msk              /*!< CMDREND flag clear bit */
#define SDIO_ICR_CMDSENTC_Pos               (7U)                               
#define SDIO_ICR_CMDSENTC_Msk               (0x1U << SDIO_ICR_CMDSENTC_Pos)    /*!< 0x00000080 */
#define SDIO_ICR_CMDSENTC                   SDIO_ICR_CMDSENTC_Msk              /*!< CMDSENT flag clear bit */
#define SDIO_ICR_DATAENDC_Pos               (8U)                               
#define SDIO_ICR_DATAENDC_Msk               (0x1U << SDIO_ICR_DATAENDC_Pos)    /*!< 0x00000100 */
#define SDIO_ICR_DATAENDC                   SDIO_ICR_DATAENDC_Msk              /*!< DATAEND flag clear bit */
#define SDIO_ICR_STBITERRC_Pos              (9U)                               
#define SDIO_ICR_STBITERRC_Msk              (0x1U << SDIO_ICR_STBITERRC_Pos)   /*!< 0x00000200 */
#define SDIO_ICR_STBITERRC                  SDIO_ICR_STBITERRC_Msk             /*!< STBITERR flag clear bit */
#define SDIO_ICR_DBCKENDC_Pos               (10U)                              
#define SDIO_ICR_DBCKENDC_Msk               (0x1U << SDIO_ICR_DBCKENDC_Pos)    /*!< 0x00000400 */
#define SDIO_ICR_DBCKENDC                   SDIO_ICR_DBCKENDC_Msk              /*!< DBCKEND flag clear bit */
#define SDIO_ICR_SDIOITC_Pos                (22U)                              
#define SDIO_ICR_SDIOITC_Msk                (0x1U << SDIO_ICR_SDIOITC_Pos)     /*!< 0x00400000 */
#define SDIO_ICR_SDIOITC                    SDIO_ICR_SDIOITC_Msk               /*!< SDIOIT flag clear bit */
#define SDIO_ICR_CEATAENDC_Pos              (23U)                              
#define SDIO_ICR_CEATAENDC_Msk              (0x1U << SDIO_ICR_CEATAENDC_Pos)   /*!< 0x00800000 */
#define SDIO_ICR_CEATAENDC                  SDIO_ICR_CEATAENDC_Msk             /*!< CEATAEND flag clear bit */

/******************  Bit definition for SDIO_MASK register  *******************/
#define SDIO_MASK_CCRCFAILIE_Pos            (0U)                               
#define SDIO_MASK_CCRCFAILIE_Msk            (0x1U << SDIO_MASK_CCRCFAILIE_Pos) /*!< 0x00000001 */
#define SDIO_MASK_CCRCFAILIE                SDIO_MASK_CCRCFAILIE_Msk           /*!< Command CRC Fail Interrupt Enable */
#define SDIO_MASK_DCRCFAILIE_Pos            (1U)                               
#define SDIO_MASK_DCRCFAILIE_Msk            (0x1U << SDIO_MASK_DCRCFAILIE_Pos) /*!< 0x00000002 */
#define SDIO_MASK_DCRCFAILIE                SDIO_MASK_DCRCFAILIE_Msk           /*!< Data CRC Fail Interrupt Enable */
#define SDIO_MASK_CTIMEOUTIE_Pos            (2U)                               
#define SDIO_MASK_CTIMEOUTIE_Msk            (0x1U << SDIO_MASK_CTIMEOUTIE_Pos) /*!< 0x00000004 */
#define SDIO_MASK_CTIMEOUTIE                SDIO_MASK_CTIMEOUTIE_Msk           /*!< Command TimeOut Interrupt Enable */
#define SDIO_MASK_DTIMEOUTIE_Pos            (3U)                               
#define SDIO_MASK_DTIMEOUTIE_Msk            (0x1U << SDIO_MASK_DTIMEOUTIE_Pos) /*!< 0x00000008 */
#define SDIO_MASK_DTIMEOUTIE                SDIO_MASK_DTIMEOUTIE_Msk           /*!< Data TimeOut Interrupt Enable */
#define SDIO_MASK_TXUNDERRIE_Pos            (4U)                               
#define SDIO_MASK_TXUNDERRIE_Msk            (0x1U << SDIO_MASK_TXUNDERRIE_Pos) /*!< 0x00000010 */
#define SDIO_MASK_TXUNDERRIE                SDIO_MASK_TXUNDERRIE_Msk           /*!< Tx FIFO UnderRun Error Interrupt Enable */
#define SDIO_MASK_RXOVERRIE_Pos             (5U)                               
#define SDIO_MASK_RXOVERRIE_Msk             (0x1U << SDIO_MASK_RXOVERRIE_Pos)  /*!< 0x00000020 */
#define SDIO_MASK_RXOVERRIE                 SDIO_MASK_RXOVERRIE_Msk            /*!< Rx FIFO OverRun Error Interrupt Enable */
#define SDIO_MASK_CMDRENDIE_Pos             (6U)                               
#define SDIO_MASK_CMDRENDIE_Msk             (0x1U << SDIO_MASK_CMDRENDIE_Pos)  /*!< 0x00000040 */
#define SDIO_MASK_CMDRENDIE                 SDIO_MASK_CMDRENDIE_Msk            /*!< Command Response Received Interrupt Enable */
#define SDIO_MASK_CMDSENTIE_Pos             (7U)                               
#define SDIO_MASK_CMDSENTIE_Msk             (0x1U << SDIO_MASK_CMDSENTIE_Pos)  /*!< 0x00000080 */
#define SDIO_MASK_CMDSENTIE                 SDIO_MASK_CMDSENTIE_Msk            /*!< Command Sent Interrupt Enable */
#define SDIO_MASK_DATAENDIE_Pos             (8U)                               
#define SDIO_MASK_DATAENDIE_Msk             (0x1U << SDIO_MASK_DATAENDIE_Pos)  /*!< 0x00000100 */
#define SDIO_MASK_DATAENDIE                 SDIO_MASK_DATAENDIE_Msk            /*!< Data End Interrupt Enable */
#define SDIO_MASK_STBITERRIE_Pos            (9U)                               
#define SDIO_MASK_STBITERRIE_Msk            (0x1U << SDIO_MASK_STBITERRIE_Pos) /*!< 0x00000200 */
#define SDIO_MASK_STBITERRIE                SDIO_MASK_STBITERRIE_Msk           /*!< Start Bit Error Interrupt Enable */
#define SDIO_MASK_DBCKENDIE_Pos             (10U)                              
#define SDIO_MASK_DBCKENDIE_Msk             (0x1U << SDIO_MASK_DBCKENDIE_Pos)  /*!< 0x00000400 */
#define SDIO_MASK_DBCKENDIE                 SDIO_MASK_DBCKENDIE_Msk            /*!< Data Block End Interrupt Enable */
#define SDIO_MASK_CMDACTIE_Pos              (11U)                              
#define SDIO_MASK_CMDACTIE_Msk              (0x1U << SDIO_MASK_CMDACTIE_Pos)   /*!< 0x00000800 */
#define SDIO_MASK_CMDACTIE                  SDIO_MASK_CMDACTIE_Msk             /*!< Command Acting Interrupt Enable */
#define SDIO_MASK_TXACTIE_Pos               (12U)                              
#define SDIO_MASK_TXACTIE_Msk               (0x1U << SDIO_MASK_TXACTIE_Pos)    /*!< 0x00001000 */
#define SDIO_MASK_TXACTIE                   SDIO_MASK_TXACTIE_Msk              /*!< Data Transmit Acting Interrupt Enable */
#define SDIO_MASK_RXACTIE_Pos               (13U)                              
#define SDIO_MASK_RXACTIE_Msk               (0x1U << SDIO_MASK_RXACTIE_Pos)    /*!< 0x00002000 */
#define SDIO_MASK_RXACTIE                   SDIO_MASK_RXACTIE_Msk              /*!< Data receive acting interrupt enabled */
#define SDIO_MASK_TXFIFOHEIE_Pos            (14U)                              
#define SDIO_MASK_TXFIFOHEIE_Msk            (0x1U << SDIO_MASK_TXFIFOHEIE_Pos) /*!< 0x00004000 */
#define SDIO_MASK_TXFIFOHEIE                SDIO_MASK_TXFIFOHEIE_Msk           /*!< Tx FIFO Half Empty interrupt Enable */
#define SDIO_MASK_RXFIFOHFIE_Pos            (15U)                              
#define SDIO_MASK_RXFIFOHFIE_Msk            (0x1U << SDIO_MASK_RXFIFOHFIE_Pos) /*!< 0x00008000 */
#define SDIO_MASK_RXFIFOHFIE                SDIO_MASK_RXFIFOHFIE_Msk           /*!< Rx FIFO Half Full interrupt Enable */
#define SDIO_MASK_TXFIFOFIE_Pos             (16U)                              
#define SDIO_MASK_TXFIFOFIE_Msk             (0x1U << SDIO_MASK_TXFIFOFIE_Pos)  /*!< 0x00010000 */
#define SDIO_MASK_TXFIFOFIE                 SDIO_MASK_TXFIFOFIE_Msk            /*!< Tx FIFO Full interrupt Enable */
#define SDIO_MASK_RXFIFOFIE_Pos             (17U)                              
#define SDIO_MASK_RXFIFOFIE_Msk             (0x1U << SDIO_MASK_RXFIFOFIE_Pos)  /*!< 0x00020000 */
#define SDIO_MASK_RXFIFOFIE                 SDIO_MASK_RXFIFOFIE_Msk            /*!< Rx FIFO Full interrupt Enable */
#define SDIO_MASK_TXFIFOEIE_Pos             (18U)                              
#define SDIO_MASK_TXFIFOEIE_Msk             (0x1U << SDIO_MASK_TXFIFOEIE_Pos)  /*!< 0x00040000 */
#define SDIO_MASK_TXFIFOEIE                 SDIO_MASK_TXFIFOEIE_Msk            /*!< Tx FIFO Empty interrupt Enable */
#define SDIO_MASK_RXFIFOEIE_Pos             (19U)                              
#define SDIO_MASK_RXFIFOEIE_Msk             (0x1U << SDIO_MASK_RXFIFOEIE_Pos)  /*!< 0x00080000 */
#define SDIO_MASK_RXFIFOEIE                 SDIO_MASK_RXFIFOEIE_Msk            /*!< Rx FIFO Empty interrupt Enable */
#define SDIO_MASK_TXDAVLIE_Pos              (20U)                              
#define SDIO_MASK_TXDAVLIE_Msk              (0x1U << SDIO_MASK_TXDAVLIE_Pos)   /*!< 0x00100000 */
#define SDIO_MASK_TXDAVLIE                  SDIO_MASK_TXDAVLIE_Msk             /*!< Data available in Tx FIFO interrupt Enable */
#define SDIO_MASK_RXDAVLIE_Pos              (21U)                              
#define SDIO_MASK_RXDAVLIE_Msk              (0x1U << SDIO_MASK_RXDAVLIE_Pos)   /*!< 0x00200000 */
#define SDIO_MASK_RXDAVLIE                  SDIO_MASK_RXDAVLIE_Msk             /*!< Data available in Rx FIFO interrupt Enable */
#define SDIO_MASK_SDIOITIE_Pos              (22U)                              
#define SDIO_MASK_SDIOITIE_Msk              (0x1U << SDIO_MASK_SDIOITIE_Pos)   /*!< 0x00400000 */
#define SDIO_MASK_SDIOITIE                  SDIO_MASK_SDIOITIE_Msk             /*!< SDIO Mode Interrupt Received interrupt Enable */
#define SDIO_MASK_CEATAENDIE_Pos            (23U)                              
#define SDIO_MASK_CEATAENDIE_Msk            (0x1U << SDIO_MASK_CEATAENDIE_Pos) /*!< 0x00800000 */
#define SDIO_MASK_CEATAENDIE                SDIO_MASK_CEATAENDIE_Msk           /*!< CE-ATA command completion signal received Interrupt Enable */

/*****************  Bit definition for SDIO_FIFOCNT register  *****************/
#define SDIO_FIFOCNT_FIFOCOUNT_Pos          (0U)                               
#define SDIO_FIFOCNT_FIFOCOUNT_Msk          (0xFFFFFFU << SDIO_FIFOCNT_FIFOCOUNT_Pos) /*!< 0x00FFFFFF */
#define SDIO_FIFOCNT_FIFOCOUNT              SDIO_FIFOCNT_FIFOCOUNT_Msk         /*!< Remaining number of words to be written to or read from the FIFO */

/******************  Bit definition for SDIO_FIFO register  *******************/
#define SDIO_FIFO_FIFODATA_Pos              (0U)                               
#define SDIO_FIFO_FIFODATA_Msk              (0xFFFFFFFFU << SDIO_FIFO_FIFODATA_Pos) /*!< 0xFFFFFFFF */
#define SDIO_FIFO_FIFODATA                  SDIO_FIFO_FIFODATA_Msk             /*!< Receive and transmit FIFO data */

/******************************************************************************/
/*                                                                            */
/*                     Serial Peripheral Interface (SPI)                      */
/*                                                                            */
/******************************************************************************/

/*
 * @brief Specific device feature definitions (not present on all devices in the STM32F3 serie)
 */
#define SPI_I2S_SUPPORT    

/*******************  Bit definition for SPI_CR1 register  ********************/
#define SPI_CR1_CPHA_Pos                    (0U)                               
#define SPI_CR1_CPHA_Msk                    (0x1U << SPI_CR1_CPHA_Pos)         /*!< 0x00000001 */
#define SPI_CR1_CPHA                        SPI_CR1_CPHA_Msk                   /*!< Clock Phase */
#define SPI_CR1_CPOL_Pos                    (1U)                               
#define SPI_CR1_CPOL_Msk                    (0x1U << SPI_CR1_CPOL_Pos)         /*!< 0x00000002 */
#define SPI_CR1_CPOL                        SPI_CR1_CPOL_Msk                   /*!< Clock Polarity */
#define SPI_CR1_MSTR_Pos                    (2U)                               
#define SPI_CR1_MSTR_Msk                    (0x1U << SPI_CR1_MSTR_Pos)         /*!< 0x00000004 */
#define SPI_CR1_MSTR                        SPI_CR1_MSTR_Msk                   /*!< Master Selection */

#define SPI_CR1_BR_Pos                      (3U)                               
#define SPI_CR1_BR_Msk                      (0x7U << SPI_CR1_BR_Pos)           /*!< 0x00000038 */
#define SPI_CR1_BR                          SPI_CR1_BR_Msk                     /*!< BR[2:0] bits (Baud Rate Control) */
#define SPI_CR1_BR_0                        (0x1U << SPI_CR1_BR_Pos)           /*!< 0x00000008 */
#define SPI_CR1_BR_1                        (0x2U << SPI_CR1_BR_Pos)           /*!< 0x00000010 */
#define SPI_CR1_BR_2                        (0x4U << SPI_CR1_BR_Pos)           /*!< 0x00000020 */

#define SPI_CR1_SPE_Pos                     (6U)                               
#define SPI_CR1_SPE_Msk                     (0x1U << SPI_CR1_SPE_Pos)          /*!< 0x00000040 */
#define SPI_CR1_SPE                         SPI_CR1_SPE_Msk                    /*!< SPI Enable */
#define SPI_CR1_LSBFIRST_Pos                (7U)                               
#define SPI_CR1_LSBFIRST_Msk                (0x1U << SPI_CR1_LSBFIRST_Pos)     /*!< 0x00000080 */
#define SPI_CR1_LSBFIRST                    SPI_CR1_LSBFIRST_Msk               /*!< Frame Format */
#define SPI_CR1_SSI_Pos                     (8U)                               
#define SPI_CR1_SSI_Msk                     (0x1U << SPI_CR1_SSI_Pos)          /*!< 0x00000100 */
#define SPI_CR1_SSI                         SPI_CR1_SSI_Msk                    /*!< Internal slave select */
#define SPI_CR1_SSM_Pos                     (9U)                               
#define SPI_CR1_SSM_Msk                     (0x1U << SPI_CR1_SSM_Pos)          /*!< 0x00000200 */
#define SPI_CR1_SSM                         SPI_CR1_SSM_Msk                    /*!< Software slave management */
#define SPI_CR1_RXONLY_Pos                  (10U)                              
#define SPI_CR1_RXONLY_Msk                  (0x1U << SPI_CR1_RXONLY_Pos)       /*!< 0x00000400 */
#define SPI_CR1_RXONLY                      SPI_CR1_RXONLY_Msk                 /*!< Receive only */
#define SPI_CR1_DFF_Pos                     (11U)                              
#define SPI_CR1_DFF_Msk                     (0x1U << SPI_CR1_DFF_Pos)          /*!< 0x00000800 */
#define SPI_CR1_DFF                         SPI_CR1_DFF_Msk                    /*!< Data Frame Format */
#define SPI_CR1_CRCNEXT_Pos                 (12U)                              
#define SPI_CR1_CRCNEXT_Msk                 (0x1U << SPI_CR1_CRCNEXT_Pos)      /*!< 0x00001000 */
#define SPI_CR1_CRCNEXT                     SPI_CR1_CRCNEXT_Msk                /*!< Transmit CRC next */
#define SPI_CR1_CRCEN_Pos                   (13U)                              
#define SPI_CR1_CRCEN_Msk                   (0x1U << SPI_CR1_CRCEN_Pos)        /*!< 0x00002000 */
#define SPI_CR1_CRCEN                       SPI_CR1_CRCEN_Msk                  /*!< Hardware CRC calculation enable */
#define SPI_CR1_BIDIOE_Pos                  (14U)                              
#define SPI_CR1_BIDIOE_Msk                  (0x1U << SPI_CR1_BIDIOE_Pos)       /*!< 0x00004000 */
#define SPI_CR1_BIDIOE                      SPI_CR1_BIDIOE_Msk                 /*!< Output enable in bidirectional mode */
#define SPI_CR1_BIDIMODE_Pos                (15U)                              
#define SPI_CR1_BIDIMODE_Msk                (0x1U << SPI_CR1_BIDIMODE_Pos)     /*!< 0x00008000 */
#define SPI_CR1_BIDIMODE                    SPI_CR1_BIDIMODE_Msk               /*!< Bidirectional data mode enable */

/*******************  Bit definition for SPI_CR2 register  ********************/
#define SPI_CR2_RXDMAEN_Pos                 (0U)                               
#define SPI_CR2_RXDMAEN_Msk                 (0x1U << SPI_CR2_RXDMAEN_Pos)      /*!< 0x00000001 */
#define SPI_CR2_RXDMAEN                     SPI_CR2_RXDMAEN_Msk                /*!< Rx Buffer DMA Enable */
#define SPI_CR2_TXDMAEN_Pos                 (1U)                               
#define SPI_CR2_TXDMAEN_Msk                 (0x1U << SPI_CR2_TXDMAEN_Pos)      /*!< 0x00000002 */
#define SPI_CR2_TXDMAEN                     SPI_CR2_TXDMAEN_Msk                /*!< Tx Buffer DMA Enable */
#define SPI_CR2_SSOE_Pos                    (2U)                               
#define SPI_CR2_SSOE_Msk                    (0x1U << SPI_CR2_SSOE_Pos)         /*!< 0x00000004 */
#define SPI_CR2_SSOE                        SPI_CR2_SSOE_Msk                   /*!< SS Output Enable */
#define SPI_CR2_FRF_Pos                     (4U)                               
#define SPI_CR2_FRF_Msk                     (0x1U << SPI_CR2_FRF_Pos)          /*!< 0x00000010 */
#define SPI_CR2_FRF                         SPI_CR2_FRF_Msk                    /*!< Frame format */
#define SPI_CR2_ERRIE_Pos                   (5U)                               
#define SPI_CR2_ERRIE_Msk                   (0x1U << SPI_CR2_ERRIE_Pos)        /*!< 0x00000020 */
#define SPI_CR2_ERRIE                       SPI_CR2_ERRIE_Msk                  /*!< Error Interrupt Enable */
#define SPI_CR2_RXNEIE_Pos                  (6U)                               
#define SPI_CR2_RXNEIE_Msk                  (0x1U << SPI_CR2_RXNEIE_Pos)       /*!< 0x00000040 */
#define SPI_CR2_RXNEIE                      SPI_CR2_RXNEIE_Msk                 /*!< RX buffer Not Empty Interrupt Enable */
#define SPI_CR2_TXEIE_Pos                   (7U)                               
#define SPI_CR2_TXEIE_Msk                   (0x1U << SPI_CR2_TXEIE_Pos)        /*!< 0x00000080 */
#define SPI_CR2_TXEIE                       SPI_CR2_TXEIE_Msk                  /*!< Tx buffer Empty Interrupt Enable */

/********************  Bit definition for SPI_SR register  ********************/
#define SPI_SR_RXNE_Pos                     (0U)                               
#define SPI_SR_RXNE_Msk                     (0x1U << SPI_SR_RXNE_Pos)          /*!< 0x00000001 */
#define SPI_SR_RXNE                         SPI_SR_RXNE_Msk                    /*!< Receive buffer Not Empty */
#define SPI_SR_TXE_Pos                      (1U)                               
#define SPI_SR_TXE_Msk                      (0x1U << SPI_SR_TXE_Pos)           /*!< 0x00000002 */
#define SPI_SR_TXE                          SPI_SR_TXE_Msk                     /*!< Transmit buffer Empty */
#define SPI_SR_CHSIDE_Pos                   (2U)                               
#define SPI_SR_CHSIDE_Msk                   (0x1U << SPI_SR_CHSIDE_Pos)        /*!< 0x00000004 */
#define SPI_SR_CHSIDE                       SPI_SR_CHSIDE_Msk                  /*!< Channel side */
#define SPI_SR_UDR_Pos                      (3U)                               
#define SPI_SR_UDR_Msk                      (0x1U << SPI_SR_UDR_Pos)           /*!< 0x00000008 */
#define SPI_SR_UDR                          SPI_SR_UDR_Msk                     /*!< Underrun flag */
#define SPI_SR_CRCERR_Pos                   (4U)                               
#define SPI_SR_CRCERR_Msk                   (0x1U << SPI_SR_CRCERR_Pos)        /*!< 0x00000010 */
#define SPI_SR_CRCERR                       SPI_SR_CRCERR_Msk                  /*!< CRC Error flag */
#define SPI_SR_MODF_Pos                     (5U)                               
#define SPI_SR_MODF_Msk                     (0x1U << SPI_SR_MODF_Pos)          /*!< 0x00000020 */
#define SPI_SR_MODF                         SPI_SR_MODF_Msk                    /*!< Mode fault */
#define SPI_SR_OVR_Pos                      (6U)                               
#define SPI_SR_OVR_Msk                      (0x1U << SPI_SR_OVR_Pos)           /*!< 0x00000040 */
#define SPI_SR_OVR                          SPI_SR_OVR_Msk                     /*!< Overrun flag */
#define SPI_SR_BSY_Pos                      (7U)                               
#define SPI_SR_BSY_Msk                      (0x1U << SPI_SR_BSY_Pos)           /*!< 0x00000080 */
#define SPI_SR_BSY                          SPI_SR_BSY_Msk                     /*!< Busy flag */
#define SPI_SR_FRE_Pos                      (8U)                               
#define SPI_SR_FRE_Msk                      (0x1U << SPI_SR_FRE_Pos)           /*!< 0x00000100 */
#define SPI_SR_FRE                          SPI_SR_FRE_Msk                     /*!<Frame format error flag  */

/********************  Bit definition for SPI_DR register  ********************/
#define SPI_DR_DR_Pos                       (0U)                               
#define SPI_DR_DR_Msk                       (0xFFFFU << SPI_DR_DR_Pos)         /*!< 0x0000FFFF */
#define SPI_DR_DR                           SPI_DR_DR_Msk                      /*!< Data Register */

/*******************  Bit definition for SPI_CRCPR register  ******************/
#define SPI_CRCPR_CRCPOLY_Pos               (0U)                               
#define SPI_CRCPR_CRCPOLY_Msk               (0xFFFFU << SPI_CRCPR_CRCPOLY_Pos) /*!< 0x0000FFFF */
#define SPI_CRCPR_CRCPOLY                   SPI_CRCPR_CRCPOLY_Msk              /*!< CRC polynomial register */

/******************  Bit definition for SPI_RXCRCR register  ******************/
#define SPI_RXCRCR_RXCRC_Pos                (0U)                               
#define SPI_RXCRCR_RXCRC_Msk                (0xFFFFU << SPI_RXCRCR_RXCRC_Pos)  /*!< 0x0000FFFF */
#define SPI_RXCRCR_RXCRC                    SPI_RXCRCR_RXCRC_Msk               /*!< Rx CRC Register */

/******************  Bit definition for SPI_TXCRCR register  ******************/
#define SPI_TXCRCR_TXCRC_Pos                (0U)                               
#define SPI_TXCRCR_TXCRC_Msk                (0xFFFFU << SPI_TXCRCR_TXCRC_Pos)  /*!< 0x0000FFFF */
#define SPI_TXCRCR_TXCRC                    SPI_TXCRCR_TXCRC_Msk               /*!< Tx CRC Register */

/******************  Bit definition for SPI_I2SCFGR register  *****************/
#define SPI_I2SCFGR_CHLEN_Pos               (0U)                               
#define SPI_I2SCFGR_CHLEN_Msk               (0x1U << SPI_I2SCFGR_CHLEN_Pos)    /*!< 0x00000001 */
#define SPI_I2SCFGR_CHLEN                   SPI_I2SCFGR_CHLEN_Msk              /*!<Channel length (number of bits per audio channel) */

#define SPI_I2SCFGR_DATLEN_Pos              (1U)                               
#define SPI_I2SCFGR_DATLEN_Msk              (0x3U << SPI_I2SCFGR_DATLEN_Pos)   /*!< 0x00000006 */
#define SPI_I2SCFGR_DATLEN                  SPI_I2SCFGR_DATLEN_Msk             /*!<DATLEN[1:0] bits (Data length to be transferred) */
#define SPI_I2SCFGR_DATLEN_0                (0x1U << SPI_I2SCFGR_DATLEN_Pos)   /*!< 0x00000002 */
#define SPI_I2SCFGR_DATLEN_1                (0x2U << SPI_I2SCFGR_DATLEN_Pos)   /*!< 0x00000004 */

#define SPI_I2SCFGR_CKPOL_Pos               (3U)                               
#define SPI_I2SCFGR_CKPOL_Msk               (0x1U << SPI_I2SCFGR_CKPOL_Pos)    /*!< 0x00000008 */
#define SPI_I2SCFGR_CKPOL                   SPI_I2SCFGR_CKPOL_Msk              /*!<steady state clock polarity */

#define SPI_I2SCFGR_I2SSTD_Pos              (4U)                               
#define SPI_I2SCFGR_I2SSTD_Msk              (0x3U << SPI_I2SCFGR_I2SSTD_Pos)   /*!< 0x00000030 */
#define SPI_I2SCFGR_I2SSTD                  SPI_I2SCFGR_I2SSTD_Msk             /*!<I2SSTD[1:0] bits (I2S standard selection) */
#define SPI_I2SCFGR_I2SSTD_0                (0x1U << SPI_I2SCFGR_I2SSTD_Pos)   /*!< 0x00000010 */
#define SPI_I2SCFGR_I2SSTD_1                (0x2U << SPI_I2SCFGR_I2SSTD_Pos)   /*!< 0x00000020 */

#define SPI_I2SCFGR_PCMSYNC_Pos             (7U)                               
#define SPI_I2SCFGR_PCMSYNC_Msk             (0x1U << SPI_I2SCFGR_PCMSYNC_Pos)  /*!< 0x00000080 */
#define SPI_I2SCFGR_PCMSYNC                 SPI_I2SCFGR_PCMSYNC_Msk            /*!<PCM frame synchronization */

#define SPI_I2SCFGR_I2SCFG_Pos              (8U)                               
#define SPI_I2SCFGR_I2SCFG_Msk              (0x3U << SPI_I2SCFGR_I2SCFG_Pos)   /*!< 0x00000300 */
#define SPI_I2SCFGR_I2SCFG                  SPI_I2SCFGR_I2SCFG_Msk             /*!<I2SCFG[1:0] bits (I2S configuration mode) */
#define SPI_I2SCFGR_I2SCFG_0                (0x1U << SPI_I2SCFGR_I2SCFG_Pos)   /*!< 0x00000100 */
#define SPI_I2SCFGR_I2SCFG_1                (0x2U << SPI_I2SCFGR_I2SCFG_Pos)   /*!< 0x00000200 */

#define SPI_I2SCFGR_I2SE_Pos                (10U)                              
#define SPI_I2SCFGR_I2SE_Msk                (0x1U << SPI_I2SCFGR_I2SE_Pos)     /*!< 0x00000400 */
#define SPI_I2SCFGR_I2SE                    SPI_I2SCFGR_I2SE_Msk               /*!<I2S Enable */
#define SPI_I2SCFGR_I2SMOD_Pos              (11U)                              
#define SPI_I2SCFGR_I2SMOD_Msk              (0x1U << SPI_I2SCFGR_I2SMOD_Pos)   /*!< 0x00000800 */
#define SPI_I2SCFGR_I2SMOD                  SPI_I2SCFGR_I2SMOD_Msk             /*!<I2S mode selection */

/******************  Bit definition for SPI_I2SPR register  *******************/
#define SPI_I2SPR_I2SDIV_Pos                (0U)                               
#define SPI_I2SPR_I2SDIV_Msk                (0xFFU << SPI_I2SPR_I2SDIV_Pos)    /*!< 0x000000FF */
#define SPI_I2SPR_I2SDIV                    SPI_I2SPR_I2SDIV_Msk               /*!<I2S Linear prescaler */
#define SPI_I2SPR_ODD_Pos                   (8U)                               
#define SPI_I2SPR_ODD_Msk                   (0x1U << SPI_I2SPR_ODD_Pos)        /*!< 0x00000100 */
#define SPI_I2SPR_ODD                       SPI_I2SPR_ODD_Msk                  /*!<Odd factor for the prescaler */
#define SPI_I2SPR_MCKOE_Pos                 (9U)                               
#define SPI_I2SPR_MCKOE_Msk                 (0x1U << SPI_I2SPR_MCKOE_Pos)      /*!< 0x00000200 */
#define SPI_I2SPR_MCKOE                     SPI_I2SPR_MCKOE_Msk                /*!<Master Clock Output Enable */

/******************************************************************************/
/*                                                                            */
/*                       System Configuration (SYSCFG)                        */
/*                                                                            */
/******************************************************************************/
/*****************  Bit definition for SYSCFG_MEMRMP register  ****************/
#define SYSCFG_MEMRMP_MEM_MODE_Pos      (0U)                                   
#define SYSCFG_MEMRMP_MEM_MODE_Msk      (0x3U << SYSCFG_MEMRMP_MEM_MODE_Pos)   /*!< 0x00000003 */
#define SYSCFG_MEMRMP_MEM_MODE          SYSCFG_MEMRMP_MEM_MODE_Msk             /*!< SYSCFG_Memory Remap Config */
#define SYSCFG_MEMRMP_MEM_MODE_0        (0x1U << SYSCFG_MEMRMP_MEM_MODE_Pos)   /*!< 0x00000001 */
#define SYSCFG_MEMRMP_MEM_MODE_1        (0x2U << SYSCFG_MEMRMP_MEM_MODE_Pos)   /*!< 0x00000002 */
#define SYSCFG_MEMRMP_BOOT_MODE_Pos     (8U)                                   
#define SYSCFG_MEMRMP_BOOT_MODE_Msk     (0x3U << SYSCFG_MEMRMP_BOOT_MODE_Pos)  /*!< 0x00000300 */
#define SYSCFG_MEMRMP_BOOT_MODE         SYSCFG_MEMRMP_BOOT_MODE_Msk            /*!< Boot mode Config */
#define SYSCFG_MEMRMP_BOOT_MODE_0       (0x1U << SYSCFG_MEMRMP_BOOT_MODE_Pos)  /*!< 0x00000100 */
#define SYSCFG_MEMRMP_BOOT_MODE_1       (0x2U << SYSCFG_MEMRMP_BOOT_MODE_Pos)  /*!< 0x00000200 */

/*****************  Bit definition for SYSCFG_PMC register  *******************/
#define SYSCFG_PMC_USB_PU_Pos           (0U)                                   
#define SYSCFG_PMC_USB_PU_Msk           (0x1U << SYSCFG_PMC_USB_PU_Pos)        /*!< 0x00000001 */
#define SYSCFG_PMC_USB_PU               SYSCFG_PMC_USB_PU_Msk                  /*!< SYSCFG PMC */
#define SYSCFG_PMC_LCD_CAPA_Pos         (1U)                                   
#define SYSCFG_PMC_LCD_CAPA_Msk         (0x1FU << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x0000003E */
#define SYSCFG_PMC_LCD_CAPA             SYSCFG_PMC_LCD_CAPA_Msk                /*!< LCD_CAPA decoupling capacitance connection */
#define SYSCFG_PMC_LCD_CAPA_0           (0x01U << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x00000002 */
#define SYSCFG_PMC_LCD_CAPA_1           (0x02U << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x00000004 */
#define SYSCFG_PMC_LCD_CAPA_2           (0x04U << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x00000008 */
#define SYSCFG_PMC_LCD_CAPA_3           (0x08U << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x00000010 */
#define SYSCFG_PMC_LCD_CAPA_4           (0x10U << SYSCFG_PMC_LCD_CAPA_Pos)     /*!< 0x00000020 */

/*****************  Bit definition for SYSCFG_EXTICR1 register  ***************/
#define SYSCFG_EXTICR1_EXTI0_Pos        (0U)                                   
#define SYSCFG_EXTICR1_EXTI0_Msk        (0xFU << SYSCFG_EXTICR1_EXTI0_Pos)     /*!< 0x0000000F */
#define SYSCFG_EXTICR1_EXTI0            SYSCFG_EXTICR1_EXTI0_Msk               /*!< EXTI 0 configuration */
#define SYSCFG_EXTICR1_EXTI1_Pos        (4U)                                   
#define SYSCFG_EXTICR1_EXTI1_Msk        (0xFU << SYSCFG_EXTICR1_EXTI1_Pos)     /*!< 0x000000F0 */
#define SYSCFG_EXTICR1_EXTI1            SYSCFG_EXTICR1_EXTI1_Msk               /*!< EXTI 1 configuration */
#define SYSCFG_EXTICR1_EXTI2_Pos        (8U)                                   
#define SYSCFG_EXTICR1_EXTI2_Msk        (0xFU << SYSCFG_EXTICR1_EXTI2_Pos)     /*!< 0x00000F00 */
#define SYSCFG_EXTICR1_EXTI2            SYSCFG_EXTICR1_EXTI2_Msk               /*!< EXTI 2 configuration */
#define SYSCFG_EXTICR1_EXTI3_Pos        (12U)                                  
#define SYSCFG_EXTICR1_EXTI3_Msk        (0xFU << SYSCFG_EXTICR1_EXTI3_Pos)     /*!< 0x0000F000 */
#define SYSCFG_EXTICR1_EXTI3            SYSCFG_EXTICR1_EXTI3_Msk               /*!< EXTI 3 configuration */

/** 
  * @brief  EXTI0 configuration  
  */ 
#define SYSCFG_EXTICR1_EXTI0_PA         (0x00000000U)                          /*!< PA[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PB         (0x00000001U)                          /*!< PB[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PC         (0x00000002U)                          /*!< PC[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PD         (0x00000003U)                          /*!< PD[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PE         (0x00000004U)                          /*!< PE[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PH         (0x00000005U)                          /*!< PH[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PF         (0x00000006U)                          /*!< PF[0] pin */
#define SYSCFG_EXTICR1_EXTI0_PG         (0x00000007U)                          /*!< PG[0] pin */

/** 
  * @brief  EXTI1 configuration  
  */ 
#define SYSCFG_EXTICR1_EXTI1_PA         (0x00000000U)                          /*!< PA[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PB         (0x00000010U)                          /*!< PB[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PC         (0x00000020U)                          /*!< PC[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PD         (0x00000030U)                          /*!< PD[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PE         (0x00000040U)                          /*!< PE[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PH         (0x00000050U)                          /*!< PH[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PF         (0x00000060U)                          /*!< PF[1] pin */
#define SYSCFG_EXTICR1_EXTI1_PG         (0x00000070U)                          /*!< PG[1] pin */

/** 
  * @brief  EXTI2 configuration  
  */ 
#define SYSCFG_EXTICR1_EXTI2_PA         (0x00000000U)                          /*!< PA[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PB         (0x00000100U)                          /*!< PB[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PC         (0x00000200U)                          /*!< PC[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PD         (0x00000300U)                          /*!< PD[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PE         (0x00000400U)                          /*!< PE[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PH         (0x00000500U)                          /*!< PH[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PF         (0x00000600U)                          /*!< PF[2] pin */
#define SYSCFG_EXTICR1_EXTI2_PG         (0x00000700U)                          /*!< PG[2] pin */

/** 
  * @brief  EXTI3 configuration  
  */ 
#define SYSCFG_EXTICR1_EXTI3_PA         (0x00000000U)                          /*!< PA[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PB         (0x00001000U)                          /*!< PB[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PC         (0x00002000U)                          /*!< PC[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PD         (0x00003000U)                          /*!< PD[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PE         (0x00004000U)                          /*!< PE[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PF         (0x00003000U)                          /*!< PF[3] pin */
#define SYSCFG_EXTICR1_EXTI3_PG         (0x00004000U)                          /*!< PG[3] pin */

/*****************  Bit definition for SYSCFG_EXTICR2 register  *****************/
#define SYSCFG_EXTICR2_EXTI4_Pos        (0U)                                   
#define SYSCFG_EXTICR2_EXTI4_Msk        (0xFU << SYSCFG_EXTICR2_EXTI4_Pos)     /*!< 0x0000000F */
#define SYSCFG_EXTICR2_EXTI4            SYSCFG_EXTICR2_EXTI4_Msk               /*!< EXTI 4 configuration */
#define SYSCFG_EXTICR2_EXTI5_Pos        (4U)                                   
#define SYSCFG_EXTICR2_EXTI5_Msk        (0xFU << SYSCFG_EXTICR2_EXTI5_Pos)     /*!< 0x000000F0 */
#define SYSCFG_EXTICR2_EXTI5            SYSCFG_EXTICR2_EXTI5_Msk               /*!< EXTI 5 configuration */
#define SYSCFG_EXTICR2_EXTI6_Pos        (8U)                                   
#define SYSCFG_EXTICR2_EXTI6_Msk        (0xFU << SYSCFG_EXTICR2_EXTI6_Pos)     /*!< 0x00000F00 */
#define SYSCFG_EXTICR2_EXTI6            SYSCFG_EXTICR2_EXTI6_Msk               /*!< EXTI 6 configuration */
#define SYSCFG_EXTICR2_EXTI7_Pos        (12U)                                  
#define SYSCFG_EXTICR2_EXTI7_Msk        (0xFU << SYSCFG_EXTICR2_EXTI7_Pos)     /*!< 0x0000F000 */
#define SYSCFG_EXTICR2_EXTI7            SYSCFG_EXTICR2_EXTI7_Msk               /*!< EXTI 7 configuration */

/** 
  * @brief  EXTI4 configuration  
  */ 
#define SYSCFG_EXTICR2_EXTI4_PA         (0x00000000U)                          /*!< PA[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PB         (0x00000001U)                          /*!< PB[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PC         (0x00000002U)                          /*!< PC[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PD         (0x00000003U)                          /*!< PD[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PE         (0x00000004U)                          /*!< PE[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PF         (0x00000006U)                          /*!< PF[4] pin */
#define SYSCFG_EXTICR2_EXTI4_PG         (0x00000007U)                          /*!< PG[4] pin */

/** 
  * @brief  EXTI5 configuration  
  */ 
#define SYSCFG_EXTICR2_EXTI5_PA         (0x00000000U)                          /*!< PA[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PB         (0x00000010U)                          /*!< PB[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PC         (0x00000020U)                          /*!< PC[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PD         (0x00000030U)                          /*!< PD[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PE         (0x00000040U)                          /*!< PE[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PF         (0x00000060U)                          /*!< PF[5] pin */
#define SYSCFG_EXTICR2_EXTI5_PG         (0x00000070U)                          /*!< PG[5] pin */

/** 
  * @brief  EXTI6 configuration  
  */ 
#define SYSCFG_EXTICR2_EXTI6_PA         (0x00000000U)                          /*!< PA[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PB         (0x00000100U)                          /*!< PB[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PC         (0x00000200U)                          /*!< PC[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PD         (0x00000300U)                          /*!< PD[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PE         (0x00000400U)                          /*!< PE[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PF         (0x00000600U)                          /*!< PF[6] pin */
#define SYSCFG_EXTICR2_EXTI6_PG         (0x00000700U)                          /*!< PG[6] pin */

/** 
  * @brief  EXTI7 configuration  
  */ 
#define SYSCFG_EXTICR2_EXTI7_PA         (0x00000000U)                          /*!< PA[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PB         (0x00001000U)                          /*!< PB[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PC         (0x00002000U)                          /*!< PC[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PD         (0x00003000U)                          /*!< PD[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PE         (0x00004000U)                          /*!< PE[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PF         (0x00006000U)                          /*!< PF[7] pin */
#define SYSCFG_EXTICR2_EXTI7_PG         (0x00007000U)                          /*!< PG[7] pin */

/*****************  Bit definition for SYSCFG_EXTICR3 register  *****************/
#define SYSCFG_EXTICR3_EXTI8_Pos        (0U)                                   
#define SYSCFG_EXTICR3_EXTI8_Msk        (0xFU << SYSCFG_EXTICR3_EXTI8_Pos)     /*!< 0x0000000F */
#define SYSCFG_EXTICR3_EXTI8            SYSCFG_EXTICR3_EXTI8_Msk               /*!< EXTI 8 configuration */
#define SYSCFG_EXTICR3_EXTI9_Pos        (4U)                                   
#define SYSCFG_EXTICR3_EXTI9_Msk        (0xFU << SYSCFG_EXTICR3_EXTI9_Pos)     /*!< 0x000000F0 */
#define SYSCFG_EXTICR3_EXTI9            SYSCFG_EXTICR3_EXTI9_Msk               /*!< EXTI 9 configuration */
#define SYSCFG_EXTICR3_EXTI10_Pos       (8U)                                   
#define SYSCFG_EXTICR3_EXTI10_Msk       (0xFU << SYSCFG_EXTICR3_EXTI10_Pos)    /*!< 0x00000F00 */
#define SYSCFG_EXTICR3_EXTI10           SYSCFG_EXTICR3_EXTI10_Msk              /*!< EXTI 10 configuration */
#define SYSCFG_EXTICR3_EXTI11_Pos       (12U)                                  
#define SYSCFG_EXTICR3_EXTI11_Msk       (0xFU << SYSCFG_EXTICR3_EXTI11_Pos)    /*!< 0x0000F000 */
#define SYSCFG_EXTICR3_EXTI11           SYSCFG_EXTICR3_EXTI11_Msk              /*!< EXTI 11 configuration */

/** 
  * @brief  EXTI8 configuration  
  */ 
#define SYSCFG_EXTICR3_EXTI8_PA         (0x00000000U)                          /*!< PA[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PB         (0x00000001U)                          /*!< PB[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PC         (0x00000002U)                          /*!< PC[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PD         (0x00000003U)                          /*!< PD[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PE         (0x00000004U)                          /*!< PE[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PF         (0x00000006U)                          /*!< PF[8] pin */
#define SYSCFG_EXTICR3_EXTI8_PG         (0x00000007U)                          /*!< PG[8] pin */

/** 
  * @brief  EXTI9 configuration  
  */ 
#define SYSCFG_EXTICR3_EXTI9_PA         (0x00000000U)                          /*!< PA[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PB         (0x00000010U)                          /*!< PB[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PC         (0x00000020U)                          /*!< PC[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PD         (0x00000030U)                          /*!< PD[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PE         (0x00000040U)                          /*!< PE[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PF         (0x00000060U)                          /*!< PF[9] pin */
#define SYSCFG_EXTICR3_EXTI9_PG         (0x00000070U)                          /*!< PG[9] pin */

/** 
  * @brief  EXTI10 configuration  
  */ 
#define SYSCFG_EXTICR3_EXTI10_PA        (0x00000000U)                          /*!< PA[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PB        (0x00000100U)                          /*!< PB[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PC        (0x00000200U)                          /*!< PC[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PD        (0x00000300U)                          /*!< PD[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PE        (0x00000400U)                          /*!< PE[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PF        (0x00000600U)                          /*!< PF[10] pin */
#define SYSCFG_EXTICR3_EXTI10_PG        (0x00000700U)                          /*!< PG[10] pin */

/** 
  * @brief  EXTI11 configuration  
  */ 
#define SYSCFG_EXTICR3_EXTI11_PA        (0x00000000U)                          /*!< PA[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PB        (0x00001000U)                          /*!< PB[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PC        (0x00002000U)                          /*!< PC[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PD        (0x00003000U)                          /*!< PD[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PE        (0x00004000U)                          /*!< PE[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PF        (0x00006000U)                          /*!< PF[11] pin */
#define SYSCFG_EXTICR3_EXTI11_PG        (0x00007000U)                          /*!< PG[11] pin */

/*****************  Bit definition for SYSCFG_EXTICR4 register  *****************/
#define SYSCFG_EXTICR4_EXTI12_Pos       (0U)                                   
#define SYSCFG_EXTICR4_EXTI12_Msk       (0xFU << SYSCFG_EXTICR4_EXTI12_Pos)    /*!< 0x0000000F */
#define SYSCFG_EXTICR4_EXTI12           SYSCFG_EXTICR4_EXTI12_Msk              /*!< EXTI 12 configuration */
#define SYSCFG_EXTICR4_EXTI13_Pos       (4U)                                   
#define SYSCFG_EXTICR4_EXTI13_Msk       (0xFU << SYSCFG_EXTICR4_EXTI13_Pos)    /*!< 0x000000F0 */
#define SYSCFG_EXTICR4_EXTI13           SYSCFG_EXTICR4_EXTI13_Msk              /*!< EXTI 13 configuration */
#define SYSCFG_EXTICR4_EXTI14_Pos       (8U)                                   
#define SYSCFG_EXTICR4_EXTI14_Msk       (0xFU << SYSCFG_EXTICR4_EXTI14_Pos)    /*!< 0x00000F00 */
#define SYSCFG_EXTICR4_EXTI14           SYSCFG_EXTICR4_EXTI14_Msk              /*!< EXTI 14 configuration */
#define SYSCFG_EXTICR4_EXTI15_Pos       (12U)                                  
#define SYSCFG_EXTICR4_EXTI15_Msk       (0xFU << SYSCFG_EXTICR4_EXTI15_Pos)    /*!< 0x0000F000 */
#define SYSCFG_EXTICR4_EXTI15           SYSCFG_EXTICR4_EXTI15_Msk              /*!< EXTI 15 configuration */

/** 
  * @brief  EXTI12 configuration  
  */ 
#define SYSCFG_EXTICR4_EXTI12_PA        (0x00000000U)                          /*!< PA[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PB        (0x00000001U)                          /*!< PB[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PC        (0x00000002U)                          /*!< PC[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PD        (0x00000003U)                          /*!< PD[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PE        (0x00000004U)                          /*!< PE[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PF        (0x00000006U)                          /*!< PF[12] pin */
#define SYSCFG_EXTICR4_EXTI12_PG        (0x00000007U)                          /*!< PG[12] pin */

/** 
  * @brief  EXTI13 configuration  
  */ 
#define SYSCFG_EXTICR4_EXTI13_PA        (0x00000000U)                          /*!< PA[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PB        (0x00000010U)                          /*!< PB[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PC        (0x00000020U)                          /*!< PC[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PD        (0x00000030U)                          /*!< PD[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PE        (0x00000040U)                          /*!< PE[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PF        (0x00000060U)                          /*!< PF[13] pin */
#define SYSCFG_EXTICR4_EXTI13_PG        (0x00000070U)                          /*!< PG[13] pin */

/** 
  * @brief  EXTI14 configuration  
  */ 
#define SYSCFG_EXTICR4_EXTI14_PA        (0x00000000U)                          /*!< PA[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PB        (0x00000100U)                          /*!< PB[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PC        (0x00000200U)                          /*!< PC[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PD        (0x00000300U)                          /*!< PD[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PE        (0x00000400U)                          /*!< PE[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PF        (0x00000600U)                          /*!< PF[14] pin */
#define SYSCFG_EXTICR4_EXTI14_PG        (0x00000700U)                          /*!< PG[14] pin */

/** 
  * @brief  EXTI15 configuration  
  */ 
#define SYSCFG_EXTICR4_EXTI15_PA        (0x00000000U)                          /*!< PA[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PB        (0x00001000U)                          /*!< PB[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PC        (0x00002000U)                          /*!< PC[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PD        (0x00003000U)                          /*!< PD[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PE        (0x00004000U)                          /*!< PE[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PF        (0x00006000U)                          /*!< PF[15] pin */
#define SYSCFG_EXTICR4_EXTI15_PG        (0x00007000U)                          /*!< PG[15] pin */
 
/******************************************************************************/
/*                                                                            */
/*                       Routing Interface (RI)                               */
/*                                                                            */
/******************************************************************************/

/********************  Bit definition for RI_ICR register  ********************/
#define RI_ICR_IC1OS_Pos                (0U)                                   
#define RI_ICR_IC1OS_Msk                (0xFU << RI_ICR_IC1OS_Pos)             /*!< 0x0000000F */
#define RI_ICR_IC1OS                    RI_ICR_IC1OS_Msk                       /*!< IC1OS[3:0] bits (Input Capture 1 select bits) */
#define RI_ICR_IC1OS_0                  (0x1U << RI_ICR_IC1OS_Pos)             /*!< 0x00000001 */
#define RI_ICR_IC1OS_1                  (0x2U << RI_ICR_IC1OS_Pos)             /*!< 0x00000002 */
#define RI_ICR_IC1OS_2                  (0x4U << RI_ICR_IC1OS_Pos)             /*!< 0x00000004 */
#define RI_ICR_IC1OS_3                  (0x8U << RI_ICR_IC1OS_Pos)             /*!< 0x00000008 */

#define RI_ICR_IC2OS_Pos                (4U)                                   
#define RI_ICR_IC2OS_Msk                (0xFU << RI_ICR_IC2OS_Pos)             /*!< 0x000000F0 */
#define RI_ICR_IC2OS                    RI_ICR_IC2OS_Msk                       /*!< IC2OS[3:0] bits (Input Capture 2 select bits) */
#define RI_ICR_IC2OS_0                  (0x1U << RI_ICR_IC2OS_Pos)             /*!< 0x00000010 */
#define RI_ICR_IC2OS_1                  (0x2U << RI_ICR_IC2OS_Pos)             /*!< 0x00000020 */
#define RI_ICR_IC2OS_2                  (0x4U << RI_ICR_IC2OS_Pos)             /*!< 0x00000040 */
#define RI_ICR_IC2OS_3                  (0x8U << RI_ICR_IC2OS_Pos)             /*!< 0x00000080 */

#define RI_ICR_IC3OS_Pos                (8U)                                   
#define RI_ICR_IC3OS_Msk                (0xFU << RI_ICR_IC3OS_Pos)             /*!< 0x00000F00 */
#define RI_ICR_IC3OS                    RI_ICR_IC3OS_Msk                       /*!< IC3OS[3:0] bits (Input Capture 3 select bits) */
#define RI_ICR_IC3OS_0                  (0x1U << RI_ICR_IC3OS_Pos)             /*!< 0x00000100 */
#define RI_ICR_IC3OS_1                  (0x2U << RI_ICR_IC3OS_Pos)             /*!< 0x00000200 */
#define RI_ICR_IC3OS_2                  (0x4U << RI_ICR_IC3OS_Pos)             /*!< 0x00000400 */
#define RI_ICR_IC3OS_3                  (0x8U << RI_ICR_IC3OS_Pos)             /*!< 0x00000800 */

#define RI_ICR_IC4OS_Pos                (12U)                                  
#define RI_ICR_IC4OS_Msk                (0xFU << RI_ICR_IC4OS_Pos)             /*!< 0x0000F000 */
#define RI_ICR_IC4OS                    RI_ICR_IC4OS_Msk                       /*!< IC4OS[3:0] bits (Input Capture 4 select bits) */
#define RI_ICR_IC4OS_0                  (0x1U << RI_ICR_IC4OS_Pos)             /*!< 0x00001000 */
#define RI_ICR_IC4OS_1                  (0x2U << RI_ICR_IC4OS_Pos)             /*!< 0x00002000 */
#define RI_ICR_IC4OS_2                  (0x4U << RI_ICR_IC4OS_Pos)             /*!< 0x00004000 */
#define RI_ICR_IC4OS_3                  (0x8U << RI_ICR_IC4OS_Pos)             /*!< 0x00008000 */

#define RI_ICR_TIM_Pos                  (16U)                                  
#define RI_ICR_TIM_Msk                  (0x3U << RI_ICR_TIM_Pos)               /*!< 0x00030000 */
#define RI_ICR_TIM                      RI_ICR_TIM_Msk                         /*!< TIM[3:0] bits (Timers select bits) */
#define RI_ICR_TIM_0                    (0x1U << RI_ICR_TIM_Pos)               /*!< 0x00010000 */
#define RI_ICR_TIM_1                    (0x2U << RI_ICR_TIM_Pos)               /*!< 0x00020000 */

#define RI_ICR_IC1_Pos                  (18U)                                  
#define RI_ICR_IC1_Msk                  (0x1U << RI_ICR_IC1_Pos)               /*!< 0x00040000 */
#define RI_ICR_IC1                      RI_ICR_IC1_Msk                         /*!< Input capture 1 */
#define RI_ICR_IC2_Pos                  (19U)                                  
#define RI_ICR_IC2_Msk                  (0x1U << RI_ICR_IC2_Pos)               /*!< 0x00080000 */
#define RI_ICR_IC2                      RI_ICR_IC2_Msk                         /*!< Input capture 2 */
#define RI_ICR_IC3_Pos                  (20U)                                  
#define RI_ICR_IC3_Msk                  (0x1U << RI_ICR_IC3_Pos)               /*!< 0x00100000 */
#define RI_ICR_IC3                      RI_ICR_IC3_Msk                         /*!< Input capture 3 */
#define RI_ICR_IC4_Pos                  (21U)                                  
#define RI_ICR_IC4_Msk                  (0x1U << RI_ICR_IC4_Pos)               /*!< 0x00200000 */
#define RI_ICR_IC4                      RI_ICR_IC4_Msk                         /*!< Input capture 4 */

/********************  Bit definition for RI_ASCR1 register  ********************/
#define RI_ASCR1_CH_Pos                 (0U)                                   
#define RI_ASCR1_CH_Msk                 (0x7BFDFFFFU << RI_ASCR1_CH_Pos)       /*!< 0x7BFDFFFF */
#define RI_ASCR1_CH                     RI_ASCR1_CH_Msk                        /*!< AS_CH[25:18] & AS_CH[15:0] bits ( Analog switches selection bits) */
#define RI_ASCR1_CH_0                   (0x00000001U)                          /*!< Bit 0 */
#define RI_ASCR1_CH_1                   (0x00000002U)                          /*!< Bit 1 */
#define RI_ASCR1_CH_2                   (0x00000004U)                          /*!< Bit 2 */
#define RI_ASCR1_CH_3                   (0x00000008U)                          /*!< Bit 3 */
#define RI_ASCR1_CH_4                   (0x00000010U)                          /*!< Bit 4 */
#define RI_ASCR1_CH_5                   (0x00000020U)                          /*!< Bit 5 */
#define RI_ASCR1_CH_6                   (0x00000040U)                          /*!< Bit 6 */
#define RI_ASCR1_CH_7                   (0x00000080U)                          /*!< Bit 7 */
#define RI_ASCR1_CH_8                   (0x00000100U)                          /*!< Bit 8 */
#define RI_ASCR1_CH_9                   (0x00000200U)                          /*!< Bit 9 */
#define RI_ASCR1_CH_10                  (0x00000400U)                          /*!< Bit 10 */
#define RI_ASCR1_CH_11                  (0x00000800U)                          /*!< Bit 11 */
#define RI_ASCR1_CH_12                  (0x00001000U)                          /*!< Bit 12 */
#define RI_ASCR1_CH_13                  (0x00002000U)                          /*!< Bit 13 */
#define RI_ASCR1_CH_14                  (0x00004000U)                          /*!< Bit 14 */
#define RI_ASCR1_CH_15                  (0x00008000U)                          /*!< Bit 15 */
#define RI_ASCR1_CH_31                  (0x00010000U)                          /*!< Bit 16 */
#define RI_ASCR1_CH_18                  (0x00040000U)                          /*!< Bit 18 */
#define RI_ASCR1_CH_19                  (0x00080000U)                          /*!< Bit 19 */
#define RI_ASCR1_CH_20                  (0x00100000U)                          /*!< Bit 20 */
#define RI_ASCR1_CH_21                  (0x00200000U)                          /*!< Bit 21 */
#define RI_ASCR1_CH_22                  (0x00400000U)                          /*!< Bit 22 */
#define RI_ASCR1_CH_23                  (0x00800000U)                          /*!< Bit 23 */
#define RI_ASCR1_CH_24                  (0x01000000U)                          /*!< Bit 24 */
#define RI_ASCR1_CH_25                  (0x02000000U)                          /*!< Bit 25 */
#define RI_ASCR1_VCOMP_Pos              (26U)                                  
#define RI_ASCR1_VCOMP_Msk              (0x1U << RI_ASCR1_VCOMP_Pos)           /*!< 0x04000000 */
#define RI_ASCR1_VCOMP                  RI_ASCR1_VCOMP_Msk                     /*!< ADC analog switch selection for internal node to COMP1 */
#define RI_ASCR1_CH_27                  (0x08000000U)                          /*!< Bit 27 */
#define RI_ASCR1_CH_28                  (0x10000000U)                          /*!< Bit 28 */
#define RI_ASCR1_CH_29                  (0x20000000U)                          /*!< Bit 29 */
#define RI_ASCR1_CH_30                  (0x40000000U)                          /*!< Bit 30 */
#define RI_ASCR1_SCM_Pos                (31U)                                  
#define RI_ASCR1_SCM_Msk                (0x1U << RI_ASCR1_SCM_Pos)             /*!< 0x80000000 */
#define RI_ASCR1_SCM                    RI_ASCR1_SCM_Msk                       /*!< I/O Switch control mode */

/********************  Bit definition for RI_ASCR2 register  ********************/
#define RI_ASCR2_GR10_1                 (0x00000001U)                          /*!< GR10-1 selection bit */
#define RI_ASCR2_GR10_2                 (0x00000002U)                          /*!< GR10-2 selection bit */
#define RI_ASCR2_GR10_3                 (0x00000004U)                          /*!< GR10-3 selection bit */
#define RI_ASCR2_GR10_4                 (0x00000008U)                          /*!< GR10-4 selection bit */
#define RI_ASCR2_GR6_Pos                (4U)                                   
#define RI_ASCR2_GR6_Msk                (0x1800003U << RI_ASCR2_GR6_Pos)       /*!< 0x18000030 */
#define RI_ASCR2_GR6                    RI_ASCR2_GR6_Msk                       /*!< GR6 selection bits */
#define RI_ASCR2_GR6_1                  (0x0000001U << RI_ASCR2_GR6_Pos)       /*!< 0x00000010 */
#define RI_ASCR2_GR6_2                  (0x0000002U << RI_ASCR2_GR6_Pos)       /*!< 0x00000020 */
#define RI_ASCR2_GR6_3                  (0x0800000U << RI_ASCR2_GR6_Pos)       /*!< 0x08000000 */
#define RI_ASCR2_GR6_4                  (0x1000000U << RI_ASCR2_GR6_Pos)       /*!< 0x10000000 */
#define RI_ASCR2_GR5_1                  (0x00000040U)                          /*!< GR5-1 selection bit */
#define RI_ASCR2_GR5_2                  (0x00000080U)                          /*!< GR5-2 selection bit */
#define RI_ASCR2_GR5_3                  (0x00000100U)                          /*!< GR5-3 selection bit */
#define RI_ASCR2_GR4_1                  (0x00000200U)                          /*!< GR4-1 selection bit */
#define RI_ASCR2_GR4_2                  (0x00000400U)                          /*!< GR4-2 selection bit */
#define RI_ASCR2_GR4_3                  (0x00000800U)                          /*!< GR4-3 selection bit */
#define RI_ASCR2_GR4_4                  (0x00008000U)                          /*!< GR4-4 selection bit */
#define RI_ASCR2_CH0b_Pos               (16U)                                  
#define RI_ASCR2_CH0b_Msk               (0x1U << RI_ASCR2_CH0b_Pos)            /*!< 0x00010000 */
#define RI_ASCR2_CH0b                   RI_ASCR2_CH0b_Msk                      /*!< CH0b selection bit */
#define RI_ASCR2_CH1b_Pos               (17U)                                  
#define RI_ASCR2_CH1b_Msk               (0x1U << RI_ASCR2_CH1b_Pos)            /*!< 0x00020000 */
#define RI_ASCR2_CH1b                   RI_ASCR2_CH1b_Msk                      /*!< CH1b selection bit */
#define RI_ASCR2_CH2b_Pos               (18U)                                  
#define RI_ASCR2_CH2b_Msk               (0x1U << RI_ASCR2_CH2b_Pos)            /*!< 0x00040000 */
#define RI_ASCR2_CH2b                   RI_ASCR2_CH2b_Msk                      /*!< CH2b selection bit */
#define RI_ASCR2_CH3b_Pos               (19U)                                  
#define RI_ASCR2_CH3b_Msk               (0x1U << RI_ASCR2_CH3b_Pos)            /*!< 0x00080000 */
#define RI_ASCR2_CH3b                   RI_ASCR2_CH3b_Msk                      /*!< CH3b selection bit */
#define RI_ASCR2_CH6b_Pos               (20U)                                  
#define RI_ASCR2_CH6b_Msk               (0x1U << RI_ASCR2_CH6b_Pos)            /*!< 0x00100000 */
#define RI_ASCR2_CH6b                   RI_ASCR2_CH6b_Msk                      /*!< CH6b selection bit */
#define RI_ASCR2_CH7b_Pos               (21U)                                  
#define RI_ASCR2_CH7b_Msk               (0x1U << RI_ASCR2_CH7b_Pos)            /*!< 0x00200000 */
#define RI_ASCR2_CH7b                   RI_ASCR2_CH7b_Msk                      /*!< CH7b selection bit */
#define RI_ASCR2_CH8b_Pos               (22U)                                  
#define RI_ASCR2_CH8b_Msk               (0x1U << RI_ASCR2_CH8b_Pos)            /*!< 0x00400000 */
#define RI_ASCR2_CH8b                   RI_ASCR2_CH8b_Msk                      /*!< CH8b selection bit */
#define RI_ASCR2_CH9b_Pos               (23U)                                  
#define RI_ASCR2_CH9b_Msk               (0x1U << RI_ASCR2_CH9b_Pos)            /*!< 0x00800000 */
#define RI_ASCR2_CH9b                   RI_ASCR2_CH9b_Msk                      /*!< CH9b selection bit */
#define RI_ASCR2_CH10b_Pos              (24U)                                  
#define RI_ASCR2_CH10b_Msk              (0x1U << RI_ASCR2_CH10b_Pos)           /*!< 0x01000000 */
#define RI_ASCR2_CH10b                  RI_ASCR2_CH10b_Msk                     /*!< CH10b selection bit */
#define RI_ASCR2_CH11b_Pos              (25U)                                  
#define RI_ASCR2_CH11b_Msk              (0x1U << RI_ASCR2_CH11b_Pos)           /*!< 0x02000000 */
#define RI_ASCR2_CH11b                  RI_ASCR2_CH11b_Msk                     /*!< CH11b selection bit */
#define RI_ASCR2_CH12b_Pos              (26U)                                  
#define RI_ASCR2_CH12b_Msk              (0x1U << RI_ASCR2_CH12b_Pos)           /*!< 0x04000000 */
#define RI_ASCR2_CH12b                  RI_ASCR2_CH12b_Msk                     /*!< CH12b selection bit */

/********************  Bit definition for RI_HYSCR1 register  ********************/
#define RI_HYSCR1_PA_Pos                (0U)                                   
#define RI_HYSCR1_PA_Msk                (0xFFFFU << RI_HYSCR1_PA_Pos)          /*!< 0x0000FFFF */
#define RI_HYSCR1_PA                    RI_HYSCR1_PA_Msk                       /*!< PA[15:0] Port A Hysteresis selection */
#define RI_HYSCR1_PA_0                  (0x0001U << RI_HYSCR1_PA_Pos)          /*!< 0x00000001 */
#define RI_HYSCR1_PA_1                  (0x0002U << RI_HYSCR1_PA_Pos)          /*!< 0x00000002 */
#define RI_HYSCR1_PA_2                  (0x0004U << RI_HYSCR1_PA_Pos)          /*!< 0x00000004 */
#define RI_HYSCR1_PA_3                  (0x0008U << RI_HYSCR1_PA_Pos)          /*!< 0x00000008 */
#define RI_HYSCR1_PA_4                  (0x0010U << RI_HYSCR1_PA_Pos)          /*!< 0x00000010 */
#define RI_HYSCR1_PA_5                  (0x0020U << RI_HYSCR1_PA_Pos)          /*!< 0x00000020 */
#define RI_HYSCR1_PA_6                  (0x0040U << RI_HYSCR1_PA_Pos)          /*!< 0x00000040 */
#define RI_HYSCR1_PA_7                  (0x0080U << RI_HYSCR1_PA_Pos)          /*!< 0x00000080 */
#define RI_HYSCR1_PA_8                  (0x0100U << RI_HYSCR1_PA_Pos)          /*!< 0x00000100 */
#define RI_HYSCR1_PA_9                  (0x0200U << RI_HYSCR1_PA_Pos)          /*!< 0x00000200 */
#define RI_HYSCR1_PA_10                 (0x0400U << RI_HYSCR1_PA_Pos)          /*!< 0x00000400 */
#define RI_HYSCR1_PA_11                 (0x0800U << RI_HYSCR1_PA_Pos)          /*!< 0x00000800 */
#define RI_HYSCR1_PA_12                 (0x1000U << RI_HYSCR1_PA_Pos)          /*!< 0x00001000 */
#define RI_HYSCR1_PA_13                 (0x2000U << RI_HYSCR1_PA_Pos)          /*!< 0x00002000 */
#define RI_HYSCR1_PA_14                 (0x4000U << RI_HYSCR1_PA_Pos)          /*!< 0x00004000 */
#define RI_HYSCR1_PA_15                 (0x8000U << RI_HYSCR1_PA_Pos)          /*!< 0x00008000 */

#define RI_HYSCR1_PB_Pos                (16U)                                  
#define RI_HYSCR1_PB_Msk                (0xFFFFU << RI_HYSCR1_PB_Pos)          /*!< 0xFFFF0000 */
#define RI_HYSCR1_PB                    RI_HYSCR1_PB_Msk                       /*!< PB[15:0] Port B Hysteresis selection */
#define RI_HYSCR1_PB_0                  (0x0001U << RI_HYSCR1_PB_Pos)          /*!< 0x00010000 */
#define RI_HYSCR1_PB_1                  (0x0002U << RI_HYSCR1_PB_Pos)          /*!< 0x00020000 */
#define RI_HYSCR1_PB_2                  (0x0004U << RI_HYSCR1_PB_Pos)          /*!< 0x00040000 */
#define RI_HYSCR1_PB_3                  (0x0008U << RI_HYSCR1_PB_Pos)          /*!< 0x00080000 */
#define RI_HYSCR1_PB_4                  (0x0010U << RI_HYSCR1_PB_Pos)          /*!< 0x00100000 */
#define RI_HYSCR1_PB_5                  (0x0020U << RI_HYSCR1_PB_Pos)          /*!< 0x00200000 */
#define RI_HYSCR1_PB_6                  (0x0040U << RI_HYSCR1_PB_Pos)          /*!< 0x00400000 */
#define RI_HYSCR1_PB_7                  (0x0080U << RI_HYSCR1_PB_Pos)          /*!< 0x00800000 */
#define RI_HYSCR1_PB_8                  (0x0100U << RI_HYSCR1_PB_Pos)          /*!< 0x01000000 */
#define RI_HYSCR1_PB_9                  (0x0200U << RI_HYSCR1_PB_Pos)          /*!< 0x02000000 */
#define RI_HYSCR1_PB_10                 (0x0400U << RI_HYSCR1_PB_Pos)          /*!< 0x04000000 */
#define RI_HYSCR1_PB_11                 (0x0800U << RI_HYSCR1_PB_Pos)          /*!< 0x08000000 */
#define RI_HYSCR1_PB_12                 (0x1000U << RI_HYSCR1_PB_Pos)          /*!< 0x10000000 */
#define RI_HYSCR1_PB_13                 (0x2000U << RI_HYSCR1_PB_Pos)          /*!< 0x20000000 */
#define RI_HYSCR1_PB_14                 (0x4000U << RI_HYSCR1_PB_Pos)          /*!< 0x40000000 */
#define RI_HYSCR1_PB_15                 (0x8000U << RI_HYSCR1_PB_Pos)          /*!< 0x80000000 */

/********************  Bit definition for RI_HYSCR2 register  ********************/
#define RI_HYSCR2_PC_Pos                (0U)                                   
#define RI_HYSCR2_PC_Msk                (0xFFFFU << RI_HYSCR2_PC_Pos)          /*!< 0x0000FFFF */
#define RI_HYSCR2_PC                    RI_HYSCR2_PC_Msk                       /*!< PC[15:0] Port C Hysteresis selection */
#define RI_HYSCR2_PC_0                  (0x0001U << RI_HYSCR2_PC_Pos)          /*!< 0x00000001 */
#define RI_HYSCR2_PC_1                  (0x0002U << RI_HYSCR2_PC_Pos)          /*!< 0x00000002 */
#define RI_HYSCR2_PC_2                  (0x0004U << RI_HYSCR2_PC_Pos)          /*!< 0x00000004 */
#define RI_HYSCR2_PC_3                  (0x0008U << RI_HYSCR2_PC_Pos)          /*!< 0x00000008 */
#define RI_HYSCR2_PC_4                  (0x0010U << RI_HYSCR2_PC_Pos)          /*!< 0x00000010 */
#define RI_HYSCR2_PC_5                  (0x0020U << RI_HYSCR2_PC_Pos)          /*!< 0x00000020 */
#define RI_HYSCR2_PC_6                  (0x0040U << RI_HYSCR2_PC_Pos)          /*!< 0x00000040 */
#define RI_HYSCR2_PC_7                  (0x0080U << RI_HYSCR2_PC_Pos)          /*!< 0x00000080 */
#define RI_HYSCR2_PC_8                  (0x0100U << RI_HYSCR2_PC_Pos)          /*!< 0x00000100 */
#define RI_HYSCR2_PC_9                  (0x0200U << RI_HYSCR2_PC_Pos)          /*!< 0x00000200 */
#define RI_HYSCR2_PC_10                 (0x0400U << RI_HYSCR2_PC_Pos)          /*!< 0x00000400 */
#define RI_HYSCR2_PC_11                 (0x0800U << RI_HYSCR2_PC_Pos)          /*!< 0x00000800 */
#define RI_HYSCR2_PC_12                 (0x1000U << RI_HYSCR2_PC_Pos)          /*!< 0x00001000 */
#define RI_HYSCR2_PC_13                 (0x2000U << RI_HYSCR2_PC_Pos)          /*!< 0x00002000 */
#define RI_HYSCR2_PC_14                 (0x4000U << RI_HYSCR2_PC_Pos)          /*!< 0x00004000 */
#define RI_HYSCR2_PC_15                 (0x8000U << RI_HYSCR2_PC_Pos)          /*!< 0x00008000 */

#define RI_HYSCR2_PD_Pos                (16U)                                  
#define RI_HYSCR2_PD_Msk                (0xFFFFU << RI_HYSCR2_PD_Pos)          /*!< 0xFFFF0000 */
#define RI_HYSCR2_PD                    RI_HYSCR2_PD_Msk                       /*!< PD[15:0] Port D Hysteresis selection */
#define RI_HYSCR2_PD_0                  (0x0001U << RI_HYSCR2_PD_Pos)          /*!< 0x00010000 */
#define RI_HYSCR2_PD_1                  (0x0002U << RI_HYSCR2_PD_Pos)          /*!< 0x00020000 */
#define RI_HYSCR2_PD_2                  (0x0004U << RI_HYSCR2_PD_Pos)          /*!< 0x00040000 */
#define RI_HYSCR2_PD_3                  (0x0008U << RI_HYSCR2_PD_Pos)          /*!< 0x00080000 */
#define RI_HYSCR2_PD_4                  (0x0010U << RI_HYSCR2_PD_Pos)          /*!< 0x00100000 */
#define RI_HYSCR2_PD_5                  (0x0020U << RI_HYSCR2_PD_Pos)          /*!< 0x00200000 */
#define RI_HYSCR2_PD_6                  (0x0040U << RI_HYSCR2_PD_Pos)          /*!< 0x00400000 */
#define RI_HYSCR2_PD_7                  (0x0080U << RI_HYSCR2_PD_Pos)          /*!< 0x00800000 */
#define RI_HYSCR2_PD_8                  (0x0100U << RI_HYSCR2_PD_Pos)          /*!< 0x01000000 */
#define RI_HYSCR2_PD_9                  (0x0200U << RI_HYSCR2_PD_Pos)          /*!< 0x02000000 */
#define RI_HYSCR2_PD_10                 (0x0400U << RI_HYSCR2_PD_Pos)          /*!< 0x04000000 */
#define RI_HYSCR2_PD_11                 (0x0800U << RI_HYSCR2_PD_Pos)          /*!< 0x08000000 */
#define RI_HYSCR2_PD_12                 (0x1000U << RI_HYSCR2_PD_Pos)          /*!< 0x10000000 */
#define RI_HYSCR2_PD_13                 (0x2000U << RI_HYSCR2_PD_Pos)          /*!< 0x20000000 */
#define RI_HYSCR2_PD_14                 (0x4000U << RI_HYSCR2_PD_Pos)          /*!< 0x40000000 */
#define RI_HYSCR2_PD_15                 (0x8000U << RI_HYSCR2_PD_Pos)          /*!< 0x80000000 */

/********************  Bit definition for RI_HYSCR3 register  ********************/
#define RI_HYSCR3_PE_Pos                (0U)                                   
#define RI_HYSCR3_PE_Msk                (0xFFFFU << RI_HYSCR3_PE_Pos)          /*!< 0x0000FFFF */
#define RI_HYSCR3_PE                    RI_HYSCR3_PE_Msk                       /*!< PE[15:0] Port E Hysteresis selection */
#define RI_HYSCR3_PE_0                  (0x0001U << RI_HYSCR3_PE_Pos)          /*!< 0x00000001 */
#define RI_HYSCR3_PE_1                  (0x0002U << RI_HYSCR3_PE_Pos)          /*!< 0x00000002 */
#define RI_HYSCR3_PE_2                  (0x0004U << RI_HYSCR3_PE_Pos)          /*!< 0x00000004 */
#define RI_HYSCR3_PE_3                  (0x0008U << RI_HYSCR3_PE_Pos)          /*!< 0x00000008 */
#define RI_HYSCR3_PE_4                  (0x0010U << RI_HYSCR3_PE_Pos)          /*!< 0x00000010 */
#define RI_HYSCR3_PE_5                  (0x0020U << RI_HYSCR3_PE_Pos)          /*!< 0x00000020 */
#define RI_HYSCR3_PE_6                  (0x0040U << RI_HYSCR3_PE_Pos)          /*!< 0x00000040 */
#define RI_HYSCR3_PE_7                  (0x0080U << RI_HYSCR3_PE_Pos)          /*!< 0x00000080 */
#define RI_HYSCR3_PE_8                  (0x0100U << RI_HYSCR3_PE_Pos)          /*!< 0x00000100 */
#define RI_HYSCR3_PE_9                  (0x0200U << RI_HYSCR3_PE_Pos)          /*!< 0x00000200 */
#define RI_HYSCR3_PE_10                 (0x0400U << RI_HYSCR3_PE_Pos)          /*!< 0x00000400 */
#define RI_HYSCR3_PE_11                 (0x0800U << RI_HYSCR3_PE_Pos)          /*!< 0x00000800 */
#define RI_HYSCR3_PE_12                 (0x1000U << RI_HYSCR3_PE_Pos)          /*!< 0x00001000 */
#define RI_HYSCR3_PE_13                 (0x2000U << RI_HYSCR3_PE_Pos)          /*!< 0x00002000 */
#define RI_HYSCR3_PE_14                 (0x4000U << RI_HYSCR3_PE_Pos)          /*!< 0x00004000 */
#define RI_HYSCR3_PE_15                 (0x8000U << RI_HYSCR3_PE_Pos)          /*!< 0x00008000 */
#define RI_HYSCR3_PF_Pos                (16U)                                  
#define RI_HYSCR3_PF_Msk                (0xFFFFU << RI_HYSCR3_PF_Pos)          /*!< 0xFFFF0000 */
#define RI_HYSCR3_PF                    RI_HYSCR3_PF_Msk                       /*!< PF[15:0] Port F Hysteresis selection */
#define RI_HYSCR3_PF_0                  (0x0001U << RI_HYSCR3_PF_Pos)          /*!< 0x00010000 */
#define RI_HYSCR3_PF_1                  (0x0002U << RI_HYSCR3_PF_Pos)          /*!< 0x00020000 */
#define RI_HYSCR3_PF_2                  (0x0004U << RI_HYSCR3_PF_Pos)          /*!< 0x00040000 */
#define RI_HYSCR3_PF_3                  (0x0008U << RI_HYSCR3_PF_Pos)          /*!< 0x00080000 */
#define RI_HYSCR3_PF_4                  (0x0010U << RI_HYSCR3_PF_Pos)          /*!< 0x00100000 */
#define RI_HYSCR3_PF_5                  (0x0020U << RI_HYSCR3_PF_Pos)          /*!< 0x00200000 */
#define RI_HYSCR3_PF_6                  (0x0040U << RI_HYSCR3_PF_Pos)          /*!< 0x00400000 */
#define RI_HYSCR3_PF_7                  (0x0080U << RI_HYSCR3_PF_Pos)          /*!< 0x00800000 */
#define RI_HYSCR3_PF_8                  (0x0100U << RI_HYSCR3_PF_Pos)          /*!< 0x01000000 */
#define RI_HYSCR3_PF_9                  (0x0200U << RI_HYSCR3_PF_Pos)          /*!< 0x02000000 */
#define RI_HYSCR3_PF_10                 (0x0400U << RI_HYSCR3_PF_Pos)          /*!< 0x04000000 */
#define RI_HYSCR3_PF_11                 (0x0800U << RI_HYSCR3_PF_Pos)          /*!< 0x08000000 */
#define RI_HYSCR3_PF_12                 (0x1000U << RI_HYSCR3_PF_Pos)          /*!< 0x10000000 */
#define RI_HYSCR3_PF_13                 (0x2000U << RI_HYSCR3_PF_Pos)          /*!< 0x20000000 */
#define RI_HYSCR3_PF_14                 (0x4000U << RI_HYSCR3_PF_Pos)          /*!< 0x40000000 */
#define RI_HYSCR3_PF_15                 (0x8000U << RI_HYSCR3_PF_Pos)          /*!< 0x80000000 */

/********************  Bit definition for RI_HYSCR4 register  ********************/
#define RI_HYSCR4_PG_Pos                (0U)                                   
#define RI_HYSCR4_PG_Msk                (0xFFFFU << RI_HYSCR4_PG_Pos)          /*!< 0x0000FFFF */
#define RI_HYSCR4_PG                    RI_HYSCR4_PG_Msk                       /*!< PG[15:0] Port G Hysteresis selection */
#define RI_HYSCR4_PG_0                  (0x0001U << RI_HYSCR4_PG_Pos)          /*!< 0x00000001 */
#define RI_HYSCR4_PG_1                  (0x0002U << RI_HYSCR4_PG_Pos)          /*!< 0x00000002 */
#define RI_HYSCR4_PG_2                  (0x0004U << RI_HYSCR4_PG_Pos)          /*!< 0x00000004 */
#define RI_HYSCR4_PG_3                  (0x0008U << RI_HYSCR4_PG_Pos)          /*!< 0x00000008 */
#define RI_HYSCR4_PG_4                  (0x0010U << RI_HYSCR4_PG_Pos)          /*!< 0x00000010 */
#define RI_HYSCR4_PG_5                  (0x0020U << RI_HYSCR4_PG_Pos)          /*!< 0x00000020 */
#define RI_HYSCR4_PG_6                  (0x0040U << RI_HYSCR4_PG_Pos)          /*!< 0x00000040 */
#define RI_HYSCR4_PG_7                  (0x0080U << RI_HYSCR4_PG_Pos)          /*!< 0x00000080 */
#define RI_HYSCR4_PG_8                  (0x0100U << RI_HYSCR4_PG_Pos)          /*!< 0x00000100 */
#define RI_HYSCR4_PG_9                  (0x0200U << RI_HYSCR4_PG_Pos)          /*!< 0x00000200 */
#define RI_HYSCR4_PG_10                 (0x0400U << RI_HYSCR4_PG_Pos)          /*!< 0x00000400 */
#define RI_HYSCR4_PG_11                 (0x0800U << RI_HYSCR4_PG_Pos)          /*!< 0x00000800 */
#define RI_HYSCR4_PG_12                 (0x1000U << RI_HYSCR4_PG_Pos)          /*!< 0x00001000 */
#define RI_HYSCR4_PG_13                 (0x2000U << RI_HYSCR4_PG_Pos)          /*!< 0x00002000 */
#define RI_HYSCR4_PG_14                 (0x4000U << RI_HYSCR4_PG_Pos)          /*!< 0x00004000 */
#define RI_HYSCR4_PG_15                 (0x8000U << RI_HYSCR4_PG_Pos)          /*!< 0x00008000 */

/********************  Bit definition for RI_ASMR1 register  ********************/
#define RI_ASMR1_PA_Pos                 (0U)                                   
#define RI_ASMR1_PA_Msk                 (0xFFFFU << RI_ASMR1_PA_Pos)           /*!< 0x0000FFFF */
#define RI_ASMR1_PA                     RI_ASMR1_PA_Msk                        /*!< PA[15:0] Port A selection*/
#define RI_ASMR1_PA_0                   (0x0001U << RI_ASMR1_PA_Pos)           /*!< 0x00000001 */
#define RI_ASMR1_PA_1                   (0x0002U << RI_ASMR1_PA_Pos)           /*!< 0x00000002 */
#define RI_ASMR1_PA_2                   (0x0004U << RI_ASMR1_PA_Pos)           /*!< 0x00000004 */
#define RI_ASMR1_PA_3                   (0x0008U << RI_ASMR1_PA_Pos)           /*!< 0x00000008 */
#define RI_ASMR1_PA_4                   (0x0010U << RI_ASMR1_PA_Pos)           /*!< 0x00000010 */
#define RI_ASMR1_PA_5                   (0x0020U << RI_ASMR1_PA_Pos)           /*!< 0x00000020 */
#define RI_ASMR1_PA_6                   (0x0040U << RI_ASMR1_PA_Pos)           /*!< 0x00000040 */
#define RI_ASMR1_PA_7                   (0x0080U << RI_ASMR1_PA_Pos)           /*!< 0x00000080 */
#define RI_ASMR1_PA_8                   (0x0100U << RI_ASMR1_PA_Pos)           /*!< 0x00000100 */
#define RI_ASMR1_PA_9                   (0x0200U << RI_ASMR1_PA_Pos)           /*!< 0x00000200 */
#define RI_ASMR1_PA_10                  (0x0400U << RI_ASMR1_PA_Pos)           /*!< 0x00000400 */
#define RI_ASMR1_PA_11                  (0x0800U << RI_ASMR1_PA_Pos)           /*!< 0x00000800 */
#define RI_ASMR1_PA_12                  (0x1000U << RI_ASMR1_PA_Pos)           /*!< 0x00001000 */
#define RI_ASMR1_PA_13                  (0x2000U << RI_ASMR1_PA_Pos)           /*!< 0x00002000 */
#define RI_ASMR1_PA_14                  (0x4000U << RI_ASMR1_PA_Pos)           /*!< 0x00004000 */
#define RI_ASMR1_PA_15                  (0x8000U << RI_ASMR1_PA_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_CMR1 register  ********************/
#define RI_CMR1_PA_Pos                  (0U)                                   
#define RI_CMR1_PA_Msk                  (0xFFFFU << RI_CMR1_PA_Pos)            /*!< 0x0000FFFF */
#define RI_CMR1_PA                      RI_CMR1_PA_Msk                         /*!< PA[15:0] Port A selection*/
#define RI_CMR1_PA_0                    (0x0001U << RI_CMR1_PA_Pos)            /*!< 0x00000001 */
#define RI_CMR1_PA_1                    (0x0002U << RI_CMR1_PA_Pos)            /*!< 0x00000002 */
#define RI_CMR1_PA_2                    (0x0004U << RI_CMR1_PA_Pos)            /*!< 0x00000004 */
#define RI_CMR1_PA_3                    (0x0008U << RI_CMR1_PA_Pos)            /*!< 0x00000008 */
#define RI_CMR1_PA_4                    (0x0010U << RI_CMR1_PA_Pos)            /*!< 0x00000010 */
#define RI_CMR1_PA_5                    (0x0020U << RI_CMR1_PA_Pos)            /*!< 0x00000020 */
#define RI_CMR1_PA_6                    (0x0040U << RI_CMR1_PA_Pos)            /*!< 0x00000040 */
#define RI_CMR1_PA_7                    (0x0080U << RI_CMR1_PA_Pos)            /*!< 0x00000080 */
#define RI_CMR1_PA_8                    (0x0100U << RI_CMR1_PA_Pos)            /*!< 0x00000100 */
#define RI_CMR1_PA_9                    (0x0200U << RI_CMR1_PA_Pos)            /*!< 0x00000200 */
#define RI_CMR1_PA_10                   (0x0400U << RI_CMR1_PA_Pos)            /*!< 0x00000400 */
#define RI_CMR1_PA_11                   (0x0800U << RI_CMR1_PA_Pos)            /*!< 0x00000800 */
#define RI_CMR1_PA_12                   (0x1000U << RI_CMR1_PA_Pos)            /*!< 0x00001000 */
#define RI_CMR1_PA_13                   (0x2000U << RI_CMR1_PA_Pos)            /*!< 0x00002000 */
#define RI_CMR1_PA_14                   (0x4000U << RI_CMR1_PA_Pos)            /*!< 0x00004000 */
#define RI_CMR1_PA_15                   (0x8000U << RI_CMR1_PA_Pos)            /*!< 0x00008000 */

/********************  Bit definition for RI_CICR1 register  ********************/
#define RI_CICR1_PA_Pos                 (0U)                                   
#define RI_CICR1_PA_Msk                 (0xFFFFU << RI_CICR1_PA_Pos)           /*!< 0x0000FFFF */
#define RI_CICR1_PA                     RI_CICR1_PA_Msk                        /*!< PA[15:0] Port A selection*/
#define RI_CICR1_PA_0                   (0x0001U << RI_CICR1_PA_Pos)           /*!< 0x00000001 */
#define RI_CICR1_PA_1                   (0x0002U << RI_CICR1_PA_Pos)           /*!< 0x00000002 */
#define RI_CICR1_PA_2                   (0x0004U << RI_CICR1_PA_Pos)           /*!< 0x00000004 */
#define RI_CICR1_PA_3                   (0x0008U << RI_CICR1_PA_Pos)           /*!< 0x00000008 */
#define RI_CICR1_PA_4                   (0x0010U << RI_CICR1_PA_Pos)           /*!< 0x00000010 */
#define RI_CICR1_PA_5                   (0x0020U << RI_CICR1_PA_Pos)           /*!< 0x00000020 */
#define RI_CICR1_PA_6                   (0x0040U << RI_CICR1_PA_Pos)           /*!< 0x00000040 */
#define RI_CICR1_PA_7                   (0x0080U << RI_CICR1_PA_Pos)           /*!< 0x00000080 */
#define RI_CICR1_PA_8                   (0x0100U << RI_CICR1_PA_Pos)           /*!< 0x00000100 */
#define RI_CICR1_PA_9                   (0x0200U << RI_CICR1_PA_Pos)           /*!< 0x00000200 */
#define RI_CICR1_PA_10                  (0x0400U << RI_CICR1_PA_Pos)           /*!< 0x00000400 */
#define RI_CICR1_PA_11                  (0x0800U << RI_CICR1_PA_Pos)           /*!< 0x00000800 */
#define RI_CICR1_PA_12                  (0x1000U << RI_CICR1_PA_Pos)           /*!< 0x00001000 */
#define RI_CICR1_PA_13                  (0x2000U << RI_CICR1_PA_Pos)           /*!< 0x00002000 */
#define RI_CICR1_PA_14                  (0x4000U << RI_CICR1_PA_Pos)           /*!< 0x00004000 */
#define RI_CICR1_PA_15                  (0x8000U << RI_CICR1_PA_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_ASMR2 register  ********************/
#define RI_ASMR2_PB_Pos                 (0U)                                   
#define RI_ASMR2_PB_Msk                 (0xFFFFU << RI_ASMR2_PB_Pos)           /*!< 0x0000FFFF */
#define RI_ASMR2_PB                     RI_ASMR2_PB_Msk                        /*!< PB[15:0] Port B selection */
#define RI_ASMR2_PB_0                   (0x0001U << RI_ASMR2_PB_Pos)           /*!< 0x00000001 */
#define RI_ASMR2_PB_1                   (0x0002U << RI_ASMR2_PB_Pos)           /*!< 0x00000002 */
#define RI_ASMR2_PB_2                   (0x0004U << RI_ASMR2_PB_Pos)           /*!< 0x00000004 */
#define RI_ASMR2_PB_3                   (0x0008U << RI_ASMR2_PB_Pos)           /*!< 0x00000008 */
#define RI_ASMR2_PB_4                   (0x0010U << RI_ASMR2_PB_Pos)           /*!< 0x00000010 */
#define RI_ASMR2_PB_5                   (0x0020U << RI_ASMR2_PB_Pos)           /*!< 0x00000020 */
#define RI_ASMR2_PB_6                   (0x0040U << RI_ASMR2_PB_Pos)           /*!< 0x00000040 */
#define RI_ASMR2_PB_7                   (0x0080U << RI_ASMR2_PB_Pos)           /*!< 0x00000080 */
#define RI_ASMR2_PB_8                   (0x0100U << RI_ASMR2_PB_Pos)           /*!< 0x00000100 */
#define RI_ASMR2_PB_9                   (0x0200U << RI_ASMR2_PB_Pos)           /*!< 0x00000200 */
#define RI_ASMR2_PB_10                  (0x0400U << RI_ASMR2_PB_Pos)           /*!< 0x00000400 */
#define RI_ASMR2_PB_11                  (0x0800U << RI_ASMR2_PB_Pos)           /*!< 0x00000800 */
#define RI_ASMR2_PB_12                  (0x1000U << RI_ASMR2_PB_Pos)           /*!< 0x00001000 */
#define RI_ASMR2_PB_13                  (0x2000U << RI_ASMR2_PB_Pos)           /*!< 0x00002000 */
#define RI_ASMR2_PB_14                  (0x4000U << RI_ASMR2_PB_Pos)           /*!< 0x00004000 */
#define RI_ASMR2_PB_15                  (0x8000U << RI_ASMR2_PB_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_CMR2 register  ********************/
#define RI_CMR2_PB_Pos                  (0U)                                   
#define RI_CMR2_PB_Msk                  (0xFFFFU << RI_CMR2_PB_Pos)            /*!< 0x0000FFFF */
#define RI_CMR2_PB                      RI_CMR2_PB_Msk                         /*!< PB[15:0] Port B selection */
#define RI_CMR2_PB_0                    (0x0001U << RI_CMR2_PB_Pos)            /*!< 0x00000001 */
#define RI_CMR2_PB_1                    (0x0002U << RI_CMR2_PB_Pos)            /*!< 0x00000002 */
#define RI_CMR2_PB_2                    (0x0004U << RI_CMR2_PB_Pos)            /*!< 0x00000004 */
#define RI_CMR2_PB_3                    (0x0008U << RI_CMR2_PB_Pos)            /*!< 0x00000008 */
#define RI_CMR2_PB_4                    (0x0010U << RI_CMR2_PB_Pos)            /*!< 0x00000010 */
#define RI_CMR2_PB_5                    (0x0020U << RI_CMR2_PB_Pos)            /*!< 0x00000020 */
#define RI_CMR2_PB_6                    (0x0040U << RI_CMR2_PB_Pos)            /*!< 0x00000040 */
#define RI_CMR2_PB_7                    (0x0080U << RI_CMR2_PB_Pos)            /*!< 0x00000080 */
#define RI_CMR2_PB_8                    (0x0100U << RI_CMR2_PB_Pos)            /*!< 0x00000100 */
#define RI_CMR2_PB_9                    (0x0200U << RI_CMR2_PB_Pos)            /*!< 0x00000200 */
#define RI_CMR2_PB_10                   (0x0400U << RI_CMR2_PB_Pos)            /*!< 0x00000400 */
#define RI_CMR2_PB_11                   (0x0800U << RI_CMR2_PB_Pos)            /*!< 0x00000800 */
#define RI_CMR2_PB_12                   (0x1000U << RI_CMR2_PB_Pos)            /*!< 0x00001000 */
#define RI_CMR2_PB_13                   (0x2000U << RI_CMR2_PB_Pos)            /*!< 0x00002000 */
#define RI_CMR2_PB_14                   (0x4000U << RI_CMR2_PB_Pos)            /*!< 0x00004000 */
#define RI_CMR2_PB_15                   (0x8000U << RI_CMR2_PB_Pos)            /*!< 0x00008000 */

/********************  Bit definition for RI_CICR2 register  ********************/
#define RI_CICR2_PB_Pos                 (0U)                                   
#define RI_CICR2_PB_Msk                 (0xFFFFU << RI_CICR2_PB_Pos)           /*!< 0x0000FFFF */
#define RI_CICR2_PB                     RI_CICR2_PB_Msk                        /*!< PB[15:0] Port B selection */
#define RI_CICR2_PB_0                   (0x0001U << RI_CICR2_PB_Pos)           /*!< 0x00000001 */
#define RI_CICR2_PB_1                   (0x0002U << RI_CICR2_PB_Pos)           /*!< 0x00000002 */
#define RI_CICR2_PB_2                   (0x0004U << RI_CICR2_PB_Pos)           /*!< 0x00000004 */
#define RI_CICR2_PB_3                   (0x0008U << RI_CICR2_PB_Pos)           /*!< 0x00000008 */
#define RI_CICR2_PB_4                   (0x0010U << RI_CICR2_PB_Pos)           /*!< 0x00000010 */
#define RI_CICR2_PB_5                   (0x0020U << RI_CICR2_PB_Pos)           /*!< 0x00000020 */
#define RI_CICR2_PB_6                   (0x0040U << RI_CICR2_PB_Pos)           /*!< 0x00000040 */
#define RI_CICR2_PB_7                   (0x0080U << RI_CICR2_PB_Pos)           /*!< 0x00000080 */
#define RI_CICR2_PB_8                   (0x0100U << RI_CICR2_PB_Pos)           /*!< 0x00000100 */
#define RI_CICR2_PB_9                   (0x0200U << RI_CICR2_PB_Pos)           /*!< 0x00000200 */
#define RI_CICR2_PB_10                  (0x0400U << RI_CICR2_PB_Pos)           /*!< 0x00000400 */
#define RI_CICR2_PB_11                  (0x0800U << RI_CICR2_PB_Pos)           /*!< 0x00000800 */
#define RI_CICR2_PB_12                  (0x1000U << RI_CICR2_PB_Pos)           /*!< 0x00001000 */
#define RI_CICR2_PB_13                  (0x2000U << RI_CICR2_PB_Pos)           /*!< 0x00002000 */
#define RI_CICR2_PB_14                  (0x4000U << RI_CICR2_PB_Pos)           /*!< 0x00004000 */
#define RI_CICR2_PB_15                  (0x8000U << RI_CICR2_PB_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_ASMR3 register  ********************/
#define RI_ASMR3_PC_Pos                 (0U)                                   
#define RI_ASMR3_PC_Msk                 (0xFFFFU << RI_ASMR3_PC_Pos)           /*!< 0x0000FFFF */
#define RI_ASMR3_PC                     RI_ASMR3_PC_Msk                        /*!< PC[15:0] Port C selection */
#define RI_ASMR3_PC_0                   (0x0001U << RI_ASMR3_PC_Pos)           /*!< 0x00000001 */
#define RI_ASMR3_PC_1                   (0x0002U << RI_ASMR3_PC_Pos)           /*!< 0x00000002 */
#define RI_ASMR3_PC_2                   (0x0004U << RI_ASMR3_PC_Pos)           /*!< 0x00000004 */
#define RI_ASMR3_PC_3                   (0x0008U << RI_ASMR3_PC_Pos)           /*!< 0x00000008 */
#define RI_ASMR3_PC_4                   (0x0010U << RI_ASMR3_PC_Pos)           /*!< 0x00000010 */
#define RI_ASMR3_PC_5                   (0x0020U << RI_ASMR3_PC_Pos)           /*!< 0x00000020 */
#define RI_ASMR3_PC_6                   (0x0040U << RI_ASMR3_PC_Pos)           /*!< 0x00000040 */
#define RI_ASMR3_PC_7                   (0x0080U << RI_ASMR3_PC_Pos)           /*!< 0x00000080 */
#define RI_ASMR3_PC_8                   (0x0100U << RI_ASMR3_PC_Pos)           /*!< 0x00000100 */
#define RI_ASMR3_PC_9                   (0x0200U << RI_ASMR3_PC_Pos)           /*!< 0x00000200 */
#define RI_ASMR3_PC_10                  (0x0400U << RI_ASMR3_PC_Pos)           /*!< 0x00000400 */
#define RI_ASMR3_PC_11                  (0x0800U << RI_ASMR3_PC_Pos)           /*!< 0x00000800 */
#define RI_ASMR3_PC_12                  (0x1000U << RI_ASMR3_PC_Pos)           /*!< 0x00001000 */
#define RI_ASMR3_PC_13                  (0x2000U << RI_ASMR3_PC_Pos)           /*!< 0x00002000 */
#define RI_ASMR3_PC_14                  (0x4000U << RI_ASMR3_PC_Pos)           /*!< 0x00004000 */
#define RI_ASMR3_PC_15                  (0x8000U << RI_ASMR3_PC_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_CMR3 register  ********************/
#define RI_CMR3_PC_Pos                  (0U)                                   
#define RI_CMR3_PC_Msk                  (0xFFFFU << RI_CMR3_PC_Pos)            /*!< 0x0000FFFF */
#define RI_CMR3_PC                      RI_CMR3_PC_Msk                         /*!< PC[15:0] Port C selection */
#define RI_CMR3_PC_0                    (0x0001U << RI_CMR3_PC_Pos)            /*!< 0x00000001 */
#define RI_CMR3_PC_1                    (0x0002U << RI_CMR3_PC_Pos)            /*!< 0x00000002 */
#define RI_CMR3_PC_2                    (0x0004U << RI_CMR3_PC_Pos)            /*!< 0x00000004 */
#define RI_CMR3_PC_3                    (0x0008U << RI_CMR3_PC_Pos)            /*!< 0x00000008 */
#define RI_CMR3_PC_4                    (0x0010U << RI_CMR3_PC_Pos)            /*!< 0x00000010 */
#define RI_CMR3_PC_5                    (0x0020U << RI_CMR3_PC_Pos)            /*!< 0x00000020 */
#define RI_CMR3_PC_6                    (0x0040U << RI_CMR3_PC_Pos)            /*!< 0x00000040 */
#define RI_CMR3_PC_7                    (0x0080U << RI_CMR3_PC_Pos)            /*!< 0x00000080 */
#define RI_CMR3_PC_8                    (0x0100U << RI_CMR3_PC_Pos)            /*!< 0x00000100 */
#define RI_CMR3_PC_9                    (0x0200U << RI_CMR3_PC_Pos)            /*!< 0x00000200 */
#define RI_CMR3_PC_10                   (0x0400U << RI_CMR3_PC_Pos)            /*!< 0x00000400 */
#define RI_CMR3_PC_11                   (0x0800U << RI_CMR3_PC_Pos)            /*!< 0x00000800 */
#define RI_CMR3_PC_12                   (0x1000U << RI_CMR3_PC_Pos)            /*!< 0x00001000 */
#define RI_CMR3_PC_13                   (0x2000U << RI_CMR3_PC_Pos)            /*!< 0x00002000 */
#define RI_CMR3_PC_14                   (0x4000U << RI_CMR3_PC_Pos)            /*!< 0x00004000 */
#define RI_CMR3_PC_15                   (0x8000U << RI_CMR3_PC_Pos)            /*!< 0x00008000 */

/********************  Bit definition for RI_CICR3 register  ********************/
#define RI_CICR3_PC_Pos                 (0U)                                   
#define RI_CICR3_PC_Msk                 (0xFFFFU << RI_CICR3_PC_Pos)           /*!< 0x0000FFFF */
#define RI_CICR3_PC                     RI_CICR3_PC_Msk                        /*!< PC[15:0] Port C selection */
#define RI_CICR3_PC_0                   (0x0001U << RI_CICR3_PC_Pos)           /*!< 0x00000001 */
#define RI_CICR3_PC_1                   (0x0002U << RI_CICR3_PC_Pos)           /*!< 0x00000002 */
#define RI_CICR3_PC_2                   (0x0004U << RI_CICR3_PC_Pos)           /*!< 0x00000004 */
#define RI_CICR3_PC_3                   (0x0008U << RI_CICR3_PC_Pos)           /*!< 0x00000008 */
#define RI_CICR3_PC_4                   (0x0010U << RI_CICR3_PC_Pos)           /*!< 0x00000010 */
#define RI_CICR3_PC_5                   (0x0020U << RI_CICR3_PC_Pos)           /*!< 0x00000020 */
#define RI_CICR3_PC_6                   (0x0040U << RI_CICR3_PC_Pos)           /*!< 0x00000040 */
#define RI_CICR3_PC_7                   (0x0080U << RI_CICR3_PC_Pos)           /*!< 0x00000080 */
#define RI_CICR3_PC_8                   (0x0100U << RI_CICR3_PC_Pos)           /*!< 0x00000100 */
#define RI_CICR3_PC_9                   (0x0200U << RI_CICR3_PC_Pos)           /*!< 0x00000200 */
#define RI_CICR3_PC_10                  (0x0400U << RI_CICR3_PC_Pos)           /*!< 0x00000400 */
#define RI_CICR3_PC_11                  (0x0800U << RI_CICR3_PC_Pos)           /*!< 0x00000800 */
#define RI_CICR3_PC_12                  (0x1000U << RI_CICR3_PC_Pos)           /*!< 0x00001000 */
#define RI_CICR3_PC_13                  (0x2000U << RI_CICR3_PC_Pos)           /*!< 0x00002000 */
#define RI_CICR3_PC_14                  (0x4000U << RI_CICR3_PC_Pos)           /*!< 0x00004000 */
#define RI_CICR3_PC_15                  (0x8000U << RI_CICR3_PC_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_ASMR4 register  ********************/
#define RI_ASMR4_PF_Pos                 (0U)                                   
#define RI_ASMR4_PF_Msk                 (0xFFFFU << RI_ASMR4_PF_Pos)           /*!< 0x0000FFFF */
#define RI_ASMR4_PF                     RI_ASMR4_PF_Msk                        /*!< PF[15:0] Port F selection */
#define RI_ASMR4_PF_0                   (0x0001U << RI_ASMR4_PF_Pos)           /*!< 0x00000001 */
#define RI_ASMR4_PF_1                   (0x0002U << RI_ASMR4_PF_Pos)           /*!< 0x00000002 */
#define RI_ASMR4_PF_2                   (0x0004U << RI_ASMR4_PF_Pos)           /*!< 0x00000004 */
#define RI_ASMR4_PF_3                   (0x0008U << RI_ASMR4_PF_Pos)           /*!< 0x00000008 */
#define RI_ASMR4_PF_4                   (0x0010U << RI_ASMR4_PF_Pos)           /*!< 0x00000010 */
#define RI_ASMR4_PF_5                   (0x0020U << RI_ASMR4_PF_Pos)           /*!< 0x00000020 */
#define RI_ASMR4_PF_6                   (0x0040U << RI_ASMR4_PF_Pos)           /*!< 0x00000040 */
#define RI_ASMR4_PF_7                   (0x0080U << RI_ASMR4_PF_Pos)           /*!< 0x00000080 */
#define RI_ASMR4_PF_8                   (0x0100U << RI_ASMR4_PF_Pos)           /*!< 0x00000100 */
#define RI_ASMR4_PF_9                   (0x0200U << RI_ASMR4_PF_Pos)           /*!< 0x00000200 */
#define RI_ASMR4_PF_10                  (0x0400U << RI_ASMR4_PF_Pos)           /*!< 0x00000400 */
#define RI_ASMR4_PF_11                  (0x0800U << RI_ASMR4_PF_Pos)           /*!< 0x00000800 */
#define RI_ASMR4_PF_12                  (0x1000U << RI_ASMR4_PF_Pos)           /*!< 0x00001000 */
#define RI_ASMR4_PF_13                  (0x2000U << RI_ASMR4_PF_Pos)           /*!< 0x00002000 */
#define RI_ASMR4_PF_14                  (0x4000U << RI_ASMR4_PF_Pos)           /*!< 0x00004000 */
#define RI_ASMR4_PF_15                  (0x8000U << RI_ASMR4_PF_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_CMR4 register  ********************/
#define RI_CMR4_PF_Pos                  (0U)                                   
#define RI_CMR4_PF_Msk                  (0xFFFFU << RI_CMR4_PF_Pos)            /*!< 0x0000FFFF */
#define RI_CMR4_PF                      RI_CMR4_PF_Msk                         /*!< PF[15:0] Port F selection */
#define RI_CMR4_PF_0                    (0x0001U << RI_CMR4_PF_Pos)            /*!< 0x00000001 */
#define RI_CMR4_PF_1                    (0x0002U << RI_CMR4_PF_Pos)            /*!< 0x00000002 */
#define RI_CMR4_PF_2                    (0x0004U << RI_CMR4_PF_Pos)            /*!< 0x00000004 */
#define RI_CMR4_PF_3                    (0x0008U << RI_CMR4_PF_Pos)            /*!< 0x00000008 */
#define RI_CMR4_PF_4                    (0x0010U << RI_CMR4_PF_Pos)            /*!< 0x00000010 */
#define RI_CMR4_PF_5                    (0x0020U << RI_CMR4_PF_Pos)            /*!< 0x00000020 */
#define RI_CMR4_PF_6                    (0x0040U << RI_CMR4_PF_Pos)            /*!< 0x00000040 */
#define RI_CMR4_PF_7                    (0x0080U << RI_CMR4_PF_Pos)            /*!< 0x00000080 */
#define RI_CMR4_PF_8                    (0x0100U << RI_CMR4_PF_Pos)            /*!< 0x00000100 */
#define RI_CMR4_PF_9                    (0x0200U << RI_CMR4_PF_Pos)            /*!< 0x00000200 */
#define RI_CMR4_PF_10                   (0x0400U << RI_CMR4_PF_Pos)            /*!< 0x00000400 */
#define RI_CMR4_PF_11                   (0x0800U << RI_CMR4_PF_Pos)            /*!< 0x00000800 */
#define RI_CMR4_PF_12                   (0x1000U << RI_CMR4_PF_Pos)            /*!< 0x00001000 */
#define RI_CMR4_PF_13                   (0x2000U << RI_CMR4_PF_Pos)            /*!< 0x00002000 */
#define RI_CMR4_PF_14                   (0x4000U << RI_CMR4_PF_Pos)            /*!< 0x00004000 */
#define RI_CMR4_PF_15                   (0x8000U << RI_CMR4_PF_Pos)            /*!< 0x00008000 */

/********************  Bit definition for RI_CICR4 register  ********************/
#define RI_CICR4_PF_Pos                 (0U)                                   
#define RI_CICR4_PF_Msk                 (0xFFFFU << RI_CICR4_PF_Pos)           /*!< 0x0000FFFF */
#define RI_CICR4_PF                     RI_CICR4_PF_Msk                        /*!< PF[15:0] Port F selection */
#define RI_CICR4_PF_0                   (0x0001U << RI_CICR4_PF_Pos)           /*!< 0x00000001 */
#define RI_CICR4_PF_1                   (0x0002U << RI_CICR4_PF_Pos)           /*!< 0x00000002 */
#define RI_CICR4_PF_2                   (0x0004U << RI_CICR4_PF_Pos)           /*!< 0x00000004 */
#define RI_CICR4_PF_3                   (0x0008U << RI_CICR4_PF_Pos)           /*!< 0x00000008 */
#define RI_CICR4_PF_4                   (0x0010U << RI_CICR4_PF_Pos)           /*!< 0x00000010 */
#define RI_CICR4_PF_5                   (0x0020U << RI_CICR4_PF_Pos)           /*!< 0x00000020 */
#define RI_CICR4_PF_6                   (0x0040U << RI_CICR4_PF_Pos)           /*!< 0x00000040 */
#define RI_CICR4_PF_7                   (0x0080U << RI_CICR4_PF_Pos)           /*!< 0x00000080 */
#define RI_CICR4_PF_8                   (0x0100U << RI_CICR4_PF_Pos)           /*!< 0x00000100 */
#define RI_CICR4_PF_9                   (0x0200U << RI_CICR4_PF_Pos)           /*!< 0x00000200 */
#define RI_CICR4_PF_10                  (0x0400U << RI_CICR4_PF_Pos)           /*!< 0x00000400 */
#define RI_CICR4_PF_11                  (0x0800U << RI_CICR4_PF_Pos)           /*!< 0x00000800 */
#define RI_CICR4_PF_12                  (0x1000U << RI_CICR4_PF_Pos)           /*!< 0x00001000 */
#define RI_CICR4_PF_13                  (0x2000U << RI_CICR4_PF_Pos)           /*!< 0x00002000 */
#define RI_CICR4_PF_14                  (0x4000U << RI_CICR4_PF_Pos)           /*!< 0x00004000 */
#define RI_CICR4_PF_15                  (0x8000U << RI_CICR4_PF_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_ASMR5 register  ********************/
#define RI_ASMR5_PG_Pos                 (0U)                                   
#define RI_ASMR5_PG_Msk                 (0xFFFFU << RI_ASMR5_PG_Pos)           /*!< 0x0000FFFF */
#define RI_ASMR5_PG                     RI_ASMR5_PG_Msk                        /*!< PG[15:0] Port G selection */
#define RI_ASMR5_PG_0                   (0x0001U << RI_ASMR5_PG_Pos)           /*!< 0x00000001 */
#define RI_ASMR5_PG_1                   (0x0002U << RI_ASMR5_PG_Pos)           /*!< 0x00000002 */
#define RI_ASMR5_PG_2                   (0x0004U << RI_ASMR5_PG_Pos)           /*!< 0x00000004 */
#define RI_ASMR5_PG_3                   (0x0008U << RI_ASMR5_PG_Pos)           /*!< 0x00000008 */
#define RI_ASMR5_PG_4                   (0x0010U << RI_ASMR5_PG_Pos)           /*!< 0x00000010 */
#define RI_ASMR5_PG_5                   (0x0020U << RI_ASMR5_PG_Pos)           /*!< 0x00000020 */
#define RI_ASMR5_PG_6                   (0x0040U << RI_ASMR5_PG_Pos)           /*!< 0x00000040 */
#define RI_ASMR5_PG_7                   (0x0080U << RI_ASMR5_PG_Pos)           /*!< 0x00000080 */
#define RI_ASMR5_PG_8                   (0x0100U << RI_ASMR5_PG_Pos)           /*!< 0x00000100 */
#define RI_ASMR5_PG_9                   (0x0200U << RI_ASMR5_PG_Pos)           /*!< 0x00000200 */
#define RI_ASMR5_PG_10                  (0x0400U << RI_ASMR5_PG_Pos)           /*!< 0x00000400 */
#define RI_ASMR5_PG_11                  (0x0800U << RI_ASMR5_PG_Pos)           /*!< 0x00000800 */
#define RI_ASMR5_PG_12                  (0x1000U << RI_ASMR5_PG_Pos)           /*!< 0x00001000 */
#define RI_ASMR5_PG_13                  (0x2000U << RI_ASMR5_PG_Pos)           /*!< 0x00002000 */
#define RI_ASMR5_PG_14                  (0x4000U << RI_ASMR5_PG_Pos)           /*!< 0x00004000 */
#define RI_ASMR5_PG_15                  (0x8000U << RI_ASMR5_PG_Pos)           /*!< 0x00008000 */

/********************  Bit definition for RI_CMR5 register  ********************/
#define RI_CMR5_PG_Pos                  (0U)                                   
#define RI_CMR5_PG_Msk                  (0xFFFFU << RI_CMR5_PG_Pos)            /*!< 0x0000FFFF */
#define RI_CMR5_PG                      RI_CMR5_PG_Msk                         /*!< PG[15:0] Port G selection */
#define RI_CMR5_PG_0                    (0x0001U << RI_CMR5_PG_Pos)            /*!< 0x00000001 */
#define RI_CMR5_PG_1                    (0x0002U << RI_CMR5_PG_Pos)            /*!< 0x00000002 */
#define RI_CMR5_PG_2                    (0x0004U << RI_CMR5_PG_Pos)            /*!< 0x00000004 */
#define RI_CMR5_PG_3                    (0x0008U << RI_CMR5_PG_Pos)            /*!< 0x00000008 */
#define RI_CMR5_PG_4                    (0x0010U << RI_CMR5_PG_Pos)            /*!< 0x00000010 */
#define RI_CMR5_PG_5                    (0x0020U << RI_CMR5_PG_Pos)            /*!< 0x00000020 */
#define RI_CMR5_PG_6                    (0x0040U << RI_CMR5_PG_Pos)            /*!< 0x00000040 */
#define RI_CMR5_PG_7                    (0x0080U << RI_CMR5_PG_Pos)            /*!< 0x00000080 */
#define RI_CMR5_PG_8                    (0x0100U << RI_CMR5_PG_Pos)            /*!< 0x00000100 */
#define RI_CMR5_PG_9                    (0x0200U << RI_CMR5_PG_Pos)            /*!< 0x00000200 */
#define RI_CMR5_PG_10                   (0x0400U << RI_CMR5_PG_Pos)            /*!< 0x00000400 */
#define RI_CMR5_PG_11                   (0x0800U << RI_CMR5_PG_Pos)            /*!< 0x00000800 */
#define RI_CMR5_PG_12                   (0x1000U << RI_CMR5_PG_Pos)            /*!< 0x00001000 */
#define RI_CMR5_PG_13                   (0x2000U << RI_CMR5_PG_Pos)            /*!< 0x00002000 */
#define RI_CMR5_PG_14                   (0x4000U << RI_CMR5_PG_Pos)            /*!< 0x00004000 */
#define RI_CMR5_PG_15                   (0x8000U << RI_CMR5_PG_Pos)            /*!< 0x00008000 */

/********************  Bit definition for RI_CICR5 register  ********************/
#define RI_CICR5_PG_Pos                 (0U)                                   
#define RI_CICR5_PG_Msk                 (0xFFFFU << RI_CICR5_PG_Pos)           /*!< 0x0000FFFF */
#define RI_CICR5_PG                     RI_CICR5_PG_Msk                        /*!< PG[15:0] Port G selection */
#define RI_CICR5_PG_0                   (0x0001U << RI_CICR5_PG_Pos)           /*!< 0x00000001 */
#define RI_CICR5_PG_1                   (0x0002U << RI_CICR5_PG_Pos)           /*!< 0x00000002 */
#define RI_CICR5_PG_2                   (0x0004U << RI_CICR5_PG_Pos)           /*!< 0x00000004 */
#define RI_CICR5_PG_3                   (0x0008U << RI_CICR5_PG_Pos)           /*!< 0x00000008 */
#define RI_CICR5_PG_4                   (0x0010U << RI_CICR5_PG_Pos)           /*!< 0x00000010 */
#define RI_CICR5_PG_5                   (0x0020U << RI_CICR5_PG_Pos)           /*!< 0x00000020 */
#define RI_CICR5_PG_6                   (0x0040U << RI_CICR5_PG_Pos)           /*!< 0x00000040 */
#define RI_CICR5_PG_7                   (0x0080U << RI_CICR5_PG_Pos)           /*!< 0x00000080 */
#define RI_CICR5_PG_8                   (0x0100U << RI_CICR5_PG_Pos)           /*!< 0x00000100 */
#define RI_CICR5_PG_9                   (0x0200U << RI_CICR5_PG_Pos)           /*!< 0x00000200 */
#define RI_CICR5_PG_10                  (0x0400U << RI_CICR5_PG_Pos)           /*!< 0x00000400 */
#define RI_CICR5_PG_11                  (0x0800U << RI_CICR5_PG_Pos)           /*!< 0x00000800 */
#define RI_CICR5_PG_12                  (0x1000U << RI_CICR5_PG_Pos)           /*!< 0x00001000 */
#define RI_CICR5_PG_13                  (0x2000U << RI_CICR5_PG_Pos)           /*!< 0x00002000 */
#define RI_CICR5_PG_14                  (0x4000U << RI_CICR5_PG_Pos)           /*!< 0x00004000 */
#define RI_CICR5_PG_15                  (0x8000U << RI_CICR5_PG_Pos)           /*!< 0x00008000 */

/******************************************************************************/
/*                                                                            */
/*                               Timers (TIM)                                 */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for TIM_CR1 register  ********************/
#define TIM_CR1_CEN_Pos                     (0U)                               
#define TIM_CR1_CEN_Msk                     (0x1U << TIM_CR1_CEN_Pos)          /*!< 0x00000001 */
#define TIM_CR1_CEN                         TIM_CR1_CEN_Msk                    /*!<Counter enable */
#define TIM_CR1_UDIS_Pos                    (1U)                               
#define TIM_CR1_UDIS_Msk                    (0x1U << TIM_CR1_UDIS_Pos)         /*!< 0x00000002 */
#define TIM_CR1_UDIS                        TIM_CR1_UDIS_Msk                   /*!<Update disable */
#define TIM_CR1_URS_Pos                     (2U)                               
#define TIM_CR1_URS_Msk                     (0x1U << TIM_CR1_URS_Pos)          /*!< 0x00000004 */
#define TIM_CR1_URS                         TIM_CR1_URS_Msk                    /*!<Update request source */
#define TIM_CR1_OPM_Pos                     (3U)                               
#define TIM_CR1_OPM_Msk                     (0x1U << TIM_CR1_OPM_Pos)          /*!< 0x00000008 */
#define TIM_CR1_OPM                         TIM_CR1_OPM_Msk                    /*!<One pulse mode */
#define TIM_CR1_DIR_Pos                     (4U)                               
#define TIM_CR1_DIR_Msk                     (0x1U << TIM_CR1_DIR_Pos)          /*!< 0x00000010 */
#define TIM_CR1_DIR                         TIM_CR1_DIR_Msk                    /*!<Direction */

#define TIM_CR1_CMS_Pos                     (5U)                               
#define TIM_CR1_CMS_Msk                     (0x3U << TIM_CR1_CMS_Pos)          /*!< 0x00000060 */
#define TIM_CR1_CMS                         TIM_CR1_CMS_Msk                    /*!<CMS[1:0] bits (Center-aligned mode selection) */
#define TIM_CR1_CMS_0                       (0x1U << TIM_CR1_CMS_Pos)          /*!< 0x00000020 */
#define TIM_CR1_CMS_1                       (0x2U << TIM_CR1_CMS_Pos)          /*!< 0x00000040 */

#define TIM_CR1_ARPE_Pos                    (7U)                               
#define TIM_CR1_ARPE_Msk                    (0x1U << TIM_CR1_ARPE_Pos)         /*!< 0x00000080 */
#define TIM_CR1_ARPE                        TIM_CR1_ARPE_Msk                   /*!<Auto-reload preload enable */

#define TIM_CR1_CKD_Pos                     (8U)                               
#define TIM_CR1_CKD_Msk                     (0x3U << TIM_CR1_CKD_Pos)          /*!< 0x00000300 */
#define TIM_CR1_CKD                         TIM_CR1_CKD_Msk                    /*!<CKD[1:0] bits (clock division) */
#define TIM_CR1_CKD_0                       (0x1U << TIM_CR1_CKD_Pos)          /*!< 0x00000100 */
#define TIM_CR1_CKD_1                       (0x2U << TIM_CR1_CKD_Pos)          /*!< 0x00000200 */

/*******************  Bit definition for TIM_CR2 register  ********************/
#define TIM_CR2_CCDS_Pos                    (3U)                               
#define TIM_CR2_CCDS_Msk                    (0x1U << TIM_CR2_CCDS_Pos)         /*!< 0x00000008 */
#define TIM_CR2_CCDS                        TIM_CR2_CCDS_Msk                   /*!<Capture/Compare DMA Selection */

#define TIM_CR2_MMS_Pos                     (4U)                               
#define TIM_CR2_MMS_Msk                     (0x7U << TIM_CR2_MMS_Pos)          /*!< 0x00000070 */
#define TIM_CR2_MMS                         TIM_CR2_MMS_Msk                    /*!<MMS[2:0] bits (Master Mode Selection) */
#define TIM_CR2_MMS_0                       (0x1U << TIM_CR2_MMS_Pos)          /*!< 0x00000010 */
#define TIM_CR2_MMS_1                       (0x2U << TIM_CR2_MMS_Pos)          /*!< 0x00000020 */
#define TIM_CR2_MMS_2                       (0x4U << TIM_CR2_MMS_Pos)          /*!< 0x00000040 */

#define TIM_CR2_TI1S_Pos                    (7U)                               
#define TIM_CR2_TI1S_Msk                    (0x1U << TIM_CR2_TI1S_Pos)         /*!< 0x00000080 */
#define TIM_CR2_TI1S                        TIM_CR2_TI1S_Msk                   /*!<TI1 Selection */

/*******************  Bit definition for TIM_SMCR register  *******************/
#define TIM_SMCR_SMS_Pos                    (0U)                               
#define TIM_SMCR_SMS_Msk                    (0x7U << TIM_SMCR_SMS_Pos)         /*!< 0x00000007 */
#define TIM_SMCR_SMS                        TIM_SMCR_SMS_Msk                   /*!<SMS[2:0] bits (Slave mode selection) */
#define TIM_SMCR_SMS_0                      (0x1U << TIM_SMCR_SMS_Pos)         /*!< 0x00000001 */
#define TIM_SMCR_SMS_1                      (0x2U << TIM_SMCR_SMS_Pos)         /*!< 0x00000002 */
#define TIM_SMCR_SMS_2                      (0x4U << TIM_SMCR_SMS_Pos)         /*!< 0x00000004 */

#define TIM_SMCR_OCCS_Pos                   (3U)                               
#define TIM_SMCR_OCCS_Msk                   (0x1U << TIM_SMCR_OCCS_Pos)        /*!< 0x00000008 */
#define TIM_SMCR_OCCS                       TIM_SMCR_OCCS_Msk                  /*!< OCREF clear selection */

#define TIM_SMCR_TS_Pos                     (4U)                               
#define TIM_SMCR_TS_Msk                     (0x7U << TIM_SMCR_TS_Pos)          /*!< 0x00000070 */
#define TIM_SMCR_TS                         TIM_SMCR_TS_Msk                    /*!<TS[2:0] bits (Trigger selection) */
#define TIM_SMCR_TS_0                       (0x1U << TIM_SMCR_TS_Pos)          /*!< 0x00000010 */
#define TIM_SMCR_TS_1                       (0x2U << TIM_SMCR_TS_Pos)          /*!< 0x00000020 */
#define TIM_SMCR_TS_2                       (0x4U << TIM_SMCR_TS_Pos)          /*!< 0x00000040 */

#define TIM_SMCR_MSM_Pos                    (7U)                               
#define TIM_SMCR_MSM_Msk                    (0x1U << TIM_SMCR_MSM_Pos)         /*!< 0x00000080 */
#define TIM_SMCR_MSM                        TIM_SMCR_MSM_Msk                   /*!<Master/slave mode */

#define TIM_SMCR_ETF_Pos                    (8U)                               
#define TIM_SMCR_ETF_Msk                    (0xFU << TIM_SMCR_ETF_Pos)         /*!< 0x00000F00 */
#define TIM_SMCR_ETF                        TIM_SMCR_ETF_Msk                   /*!<ETF[3:0] bits (External trigger filter) */
#define TIM_SMCR_ETF_0                      (0x1U << TIM_SMCR_ETF_Pos)         /*!< 0x00000100 */
#define TIM_SMCR_ETF_1                      (0x2U << TIM_SMCR_ETF_Pos)         /*!< 0x00000200 */
#define TIM_SMCR_ETF_2                      (0x4U << TIM_SMCR_ETF_Pos)         /*!< 0x00000400 */
#define TIM_SMCR_ETF_3                      (0x8U << TIM_SMCR_ETF_Pos)         /*!< 0x00000800 */

#define TIM_SMCR_ETPS_Pos                   (12U)                              
#define TIM_SMCR_ETPS_Msk                   (0x3U << TIM_SMCR_ETPS_Pos)        /*!< 0x00003000 */
#define TIM_SMCR_ETPS                       TIM_SMCR_ETPS_Msk                  /*!<ETPS[1:0] bits (External trigger prescaler) */
#define TIM_SMCR_ETPS_0                     (0x1U << TIM_SMCR_ETPS_Pos)        /*!< 0x00001000 */
#define TIM_SMCR_ETPS_1                     (0x2U << TIM_SMCR_ETPS_Pos)        /*!< 0x00002000 */

#define TIM_SMCR_ECE_Pos                    (14U)                              
#define TIM_SMCR_ECE_Msk                    (0x1U << TIM_SMCR_ECE_Pos)         /*!< 0x00004000 */
#define TIM_SMCR_ECE                        TIM_SMCR_ECE_Msk                   /*!<External clock enable */
#define TIM_SMCR_ETP_Pos                    (15U)                              
#define TIM_SMCR_ETP_Msk                    (0x1U << TIM_SMCR_ETP_Pos)         /*!< 0x00008000 */
#define TIM_SMCR_ETP                        TIM_SMCR_ETP_Msk                   /*!<External trigger polarity */

/*******************  Bit definition for TIM_DIER register  *******************/
#define TIM_DIER_UIE_Pos                    (0U)                               
#define TIM_DIER_UIE_Msk                    (0x1U << TIM_DIER_UIE_Pos)         /*!< 0x00000001 */
#define TIM_DIER_UIE                        TIM_DIER_UIE_Msk                   /*!<Update interrupt enable */
#define TIM_DIER_CC1IE_Pos                  (1U)                               
#define TIM_DIER_CC1IE_Msk                  (0x1U << TIM_DIER_CC1IE_Pos)       /*!< 0x00000002 */
#define TIM_DIER_CC1IE                      TIM_DIER_CC1IE_Msk                 /*!<Capture/Compare 1 interrupt enable */
#define TIM_DIER_CC2IE_Pos                  (2U)                               
#define TIM_DIER_CC2IE_Msk                  (0x1U << TIM_DIER_CC2IE_Pos)       /*!< 0x00000004 */
#define TIM_DIER_CC2IE                      TIM_DIER_CC2IE_Msk                 /*!<Capture/Compare 2 interrupt enable */
#define TIM_DIER_CC3IE_Pos                  (3U)                               
#define TIM_DIER_CC3IE_Msk                  (0x1U << TIM_DIER_CC3IE_Pos)       /*!< 0x00000008 */
#define TIM_DIER_CC3IE                      TIM_DIER_CC3IE_Msk                 /*!<Capture/Compare 3 interrupt enable */
#define TIM_DIER_CC4IE_Pos                  (4U)                               
#define TIM_DIER_CC4IE_Msk                  (0x1U << TIM_DIER_CC4IE_Pos)       /*!< 0x00000010 */
#define TIM_DIER_CC4IE                      TIM_DIER_CC4IE_Msk                 /*!<Capture/Compare 4 interrupt enable */
#define TIM_DIER_TIE_Pos                    (6U)                               
#define TIM_DIER_TIE_Msk                    (0x1U << TIM_DIER_TIE_Pos)         /*!< 0x00000040 */
#define TIM_DIER_TIE                        TIM_DIER_TIE_Msk                   /*!<Trigger interrupt enable */
#define TIM_DIER_UDE_Pos                    (8U)                               
#define TIM_DIER_UDE_Msk                    (0x1U << TIM_DIER_UDE_Pos)         /*!< 0x00000100 */
#define TIM_DIER_UDE                        TIM_DIER_UDE_Msk                   /*!<Update DMA request enable */
#define TIM_DIER_CC1DE_Pos                  (9U)                               
#define TIM_DIER_CC1DE_Msk                  (0x1U << TIM_DIER_CC1DE_Pos)       /*!< 0x00000200 */
#define TIM_DIER_CC1DE                      TIM_DIER_CC1DE_Msk                 /*!<Capture/Compare 1 DMA request enable */
#define TIM_DIER_CC2DE_Pos                  (10U)                              
#define TIM_DIER_CC2DE_Msk                  (0x1U << TIM_DIER_CC2DE_Pos)       /*!< 0x00000400 */
#define TIM_DIER_CC2DE                      TIM_DIER_CC2DE_Msk                 /*!<Capture/Compare 2 DMA request enable */
#define TIM_DIER_CC3DE_Pos                  (11U)                              
#define TIM_DIER_CC3DE_Msk                  (0x1U << TIM_DIER_CC3DE_Pos)       /*!< 0x00000800 */
#define TIM_DIER_CC3DE                      TIM_DIER_CC3DE_Msk                 /*!<Capture/Compare 3 DMA request enable */
#define TIM_DIER_CC4DE_Pos                  (12U)                              
#define TIM_DIER_CC4DE_Msk                  (0x1U << TIM_DIER_CC4DE_Pos)       /*!< 0x00001000 */
#define TIM_DIER_CC4DE                      TIM_DIER_CC4DE_Msk                 /*!<Capture/Compare 4 DMA request enable */
#define TIM_DIER_COMDE                      ((uint16_t)0x2000U)                /*!<COM DMA request enable */
#define TIM_DIER_TDE_Pos                    (14U)                              
#define TIM_DIER_TDE_Msk                    (0x1U << TIM_DIER_TDE_Pos)         /*!< 0x00004000 */
#define TIM_DIER_TDE                        TIM_DIER_TDE_Msk                   /*!<Trigger DMA request enable */

/********************  Bit definition for TIM_SR register  ********************/
#define TIM_SR_UIF_Pos                      (0U)                               
#define TIM_SR_UIF_Msk                      (0x1U << TIM_SR_UIF_Pos)           /*!< 0x00000001 */
#define TIM_SR_UIF                          TIM_SR_UIF_Msk                     /*!<Update interrupt Flag */
#define TIM_SR_CC1IF_Pos                    (1U)                               
#define TIM_SR_CC1IF_Msk                    (0x1U << TIM_SR_CC1IF_Pos)         /*!< 0x00000002 */
#define TIM_SR_CC1IF                        TIM_SR_CC1IF_Msk                   /*!<Capture/Compare 1 interrupt Flag */
#define TIM_SR_CC2IF_Pos                    (2U)                               
#define TIM_SR_CC2IF_Msk                    (0x1U << TIM_SR_CC2IF_Pos)         /*!< 0x00000004 */
#define TIM_SR_CC2IF                        TIM_SR_CC2IF_Msk                   /*!<Capture/Compare 2 interrupt Flag */
#define TIM_SR_CC3IF_Pos                    (3U)                               
#define TIM_SR_CC3IF_Msk                    (0x1U << TIM_SR_CC3IF_Pos)         /*!< 0x00000008 */
#define TIM_SR_CC3IF                        TIM_SR_CC3IF_Msk                   /*!<Capture/Compare 3 interrupt Flag */
#define TIM_SR_CC4IF_Pos                    (4U)                               
#define TIM_SR_CC4IF_Msk                    (0x1U << TIM_SR_CC4IF_Pos)         /*!< 0x00000010 */
#define TIM_SR_CC4IF                        TIM_SR_CC4IF_Msk                   /*!<Capture/Compare 4 interrupt Flag */
#define TIM_SR_TIF_Pos                      (6U)                               
#define TIM_SR_TIF_Msk                      (0x1U << TIM_SR_TIF_Pos)           /*!< 0x00000040 */
#define TIM_SR_TIF                          TIM_SR_TIF_Msk                     /*!<Trigger interrupt Flag */
#define TIM_SR_CC1OF_Pos                    (9U)                               
#define TIM_SR_CC1OF_Msk                    (0x1U << TIM_SR_CC1OF_Pos)         /*!< 0x00000200 */
#define TIM_SR_CC1OF                        TIM_SR_CC1OF_Msk                   /*!<Capture/Compare 1 Overcapture Flag */
#define TIM_SR_CC2OF_Pos                    (10U)                              
#define TIM_SR_CC2OF_Msk                    (0x1U << TIM_SR_CC2OF_Pos)         /*!< 0x00000400 */
#define TIM_SR_CC2OF                        TIM_SR_CC2OF_Msk                   /*!<Capture/Compare 2 Overcapture Flag */
#define TIM_SR_CC3OF_Pos                    (11U)                              
#define TIM_SR_CC3OF_Msk                    (0x1U << TIM_SR_CC3OF_Pos)         /*!< 0x00000800 */
#define TIM_SR_CC3OF                        TIM_SR_CC3OF_Msk                   /*!<Capture/Compare 3 Overcapture Flag */
#define TIM_SR_CC4OF_Pos                    (12U)                              
#define TIM_SR_CC4OF_Msk                    (0x1U << TIM_SR_CC4OF_Pos)         /*!< 0x00001000 */
#define TIM_SR_CC4OF                        TIM_SR_CC4OF_Msk                   /*!<Capture/Compare 4 Overcapture Flag */

/*******************  Bit definition for TIM_EGR register  ********************/
#define TIM_EGR_UG_Pos                      (0U)                               
#define TIM_EGR_UG_Msk                      (0x1U << TIM_EGR_UG_Pos)           /*!< 0x00000001 */
#define TIM_EGR_UG                          TIM_EGR_UG_Msk                     /*!<Update Generation */
#define TIM_EGR_CC1G_Pos                    (1U)                               
#define TIM_EGR_CC1G_Msk                    (0x1U << TIM_EGR_CC1G_Pos)         /*!< 0x00000002 */
#define TIM_EGR_CC1G                        TIM_EGR_CC1G_Msk                   /*!<Capture/Compare 1 Generation */
#define TIM_EGR_CC2G_Pos                    (2U)                               
#define TIM_EGR_CC2G_Msk                    (0x1U << TIM_EGR_CC2G_Pos)         /*!< 0x00000004 */
#define TIM_EGR_CC2G                        TIM_EGR_CC2G_Msk                   /*!<Capture/Compare 2 Generation */
#define TIM_EGR_CC3G_Pos                    (3U)                               
#define TIM_EGR_CC3G_Msk                    (0x1U << TIM_EGR_CC3G_Pos)         /*!< 0x00000008 */
#define TIM_EGR_CC3G                        TIM_EGR_CC3G_Msk                   /*!<Capture/Compare 3 Generation */
#define TIM_EGR_CC4G_Pos                    (4U)                               
#define TIM_EGR_CC4G_Msk                    (0x1U << TIM_EGR_CC4G_Pos)         /*!< 0x00000010 */
#define TIM_EGR_CC4G                        TIM_EGR_CC4G_Msk                   /*!<Capture/Compare 4 Generation */
#define TIM_EGR_TG_Pos                      (6U)                               
#define TIM_EGR_TG_Msk                      (0x1U << TIM_EGR_TG_Pos)           /*!< 0x00000040 */
#define TIM_EGR_TG                          TIM_EGR_TG_Msk                     /*!<Trigger Generation */
                   
/******************  Bit definition for TIM_CCMR1 register  *******************/
#define TIM_CCMR1_CC1S_Pos                  (0U)                               
#define TIM_CCMR1_CC1S_Msk                  (0x3U << TIM_CCMR1_CC1S_Pos)       /*!< 0x00000003 */
#define TIM_CCMR1_CC1S                      TIM_CCMR1_CC1S_Msk                 /*!<CC1S[1:0] bits (Capture/Compare 1 Selection) */
#define TIM_CCMR1_CC1S_0                    (0x1U << TIM_CCMR1_CC1S_Pos)       /*!< 0x00000001 */
#define TIM_CCMR1_CC1S_1                    (0x2U << TIM_CCMR1_CC1S_Pos)       /*!< 0x00000002 */

#define TIM_CCMR1_OC1FE_Pos                 (2U)                               
#define TIM_CCMR1_OC1FE_Msk                 (0x1U << TIM_CCMR1_OC1FE_Pos)      /*!< 0x00000004 */
#define TIM_CCMR1_OC1FE                     TIM_CCMR1_OC1FE_Msk                /*!<Output Compare 1 Fast enable */
#define TIM_CCMR1_OC1PE_Pos                 (3U)                               
#define TIM_CCMR1_OC1PE_Msk                 (0x1U << TIM_CCMR1_OC1PE_Pos)      /*!< 0x00000008 */
#define TIM_CCMR1_OC1PE                     TIM_CCMR1_OC1PE_Msk                /*!<Output Compare 1 Preload enable */

#define TIM_CCMR1_OC1M_Pos                  (4U)                               
#define TIM_CCMR1_OC1M_Msk                  (0x7U << TIM_CCMR1_OC1M_Pos)       /*!< 0x00000070 */
#define TIM_CCMR1_OC1M                      TIM_CCMR1_OC1M_Msk                 /*!<OC1M[2:0] bits (Output Compare 1 Mode) */
#define TIM_CCMR1_OC1M_0                    (0x1U << TIM_CCMR1_OC1M_Pos)       /*!< 0x00000010 */
#define TIM_CCMR1_OC1M_1                    (0x2U << TIM_CCMR1_OC1M_Pos)       /*!< 0x00000020 */
#define TIM_CCMR1_OC1M_2                    (0x4U << TIM_CCMR1_OC1M_Pos)       /*!< 0x00000040 */

#define TIM_CCMR1_OC1CE_Pos                 (7U)                               
#define TIM_CCMR1_OC1CE_Msk                 (0x1U << TIM_CCMR1_OC1CE_Pos)      /*!< 0x00000080 */
#define TIM_CCMR1_OC1CE                     TIM_CCMR1_OC1CE_Msk                /*!<Output Compare 1Clear Enable */

#define TIM_CCMR1_CC2S_Pos                  (8U)                               
#define TIM_CCMR1_CC2S_Msk                  (0x3U << TIM_CCMR1_CC2S_Pos)       /*!< 0x00000300 */
#define TIM_CCMR1_CC2S                      TIM_CCMR1_CC2S_Msk                 /*!<CC2S[1:0] bits (Capture/Compare 2 Selection) */
#define TIM_CCMR1_CC2S_0                    (0x1U << TIM_CCMR1_CC2S_Pos)       /*!< 0x00000100 */
#define TIM_CCMR1_CC2S_1                    (0x2U << TIM_CCMR1_CC2S_Pos)       /*!< 0x00000200 */

#define TIM_CCMR1_OC2FE_Pos                 (10U)                              
#define TIM_CCMR1_OC2FE_Msk                 (0x1U << TIM_CCMR1_OC2FE_Pos)      /*!< 0x00000400 */
#define TIM_CCMR1_OC2FE                     TIM_CCMR1_OC2FE_Msk                /*!<Output Compare 2 Fast enable */
#define TIM_CCMR1_OC2PE_Pos                 (11U)                              
#define TIM_CCMR1_OC2PE_Msk                 (0x1U << TIM_CCMR1_OC2PE_Pos)      /*!< 0x00000800 */
#define TIM_CCMR1_OC2PE                     TIM_CCMR1_OC2PE_Msk                /*!<Output Compare 2 Preload enable */

#define TIM_CCMR1_OC2M_Pos                  (12U)                              
#define TIM_CCMR1_OC2M_Msk                  (0x7U << TIM_CCMR1_OC2M_Pos)       /*!< 0x00007000 */
#define TIM_CCMR1_OC2M                      TIM_CCMR1_OC2M_Msk                 /*!<OC2M[2:0] bits (Output Compare 2 Mode) */
#define TIM_CCMR1_OC2M_0                    (0x1U << TIM_CCMR1_OC2M_Pos)       /*!< 0x00001000 */
#define TIM_CCMR1_OC2M_1                    (0x2U << TIM_CCMR1_OC2M_Pos)       /*!< 0x00002000 */
#define TIM_CCMR1_OC2M_2                    (0x4U << TIM_CCMR1_OC2M_Pos)       /*!< 0x00004000 */

#define TIM_CCMR1_OC2CE_Pos                 (15U)                              
#define TIM_CCMR1_OC2CE_Msk                 (0x1U << TIM_CCMR1_OC2CE_Pos)      /*!< 0x00008000 */
#define TIM_CCMR1_OC2CE                     TIM_CCMR1_OC2CE_Msk                /*!<Output Compare 2 Clear Enable */

/*----------------------------------------------------------------------------*/

#define TIM_CCMR1_IC1PSC_Pos                (2U)                               
#define TIM_CCMR1_IC1PSC_Msk                (0x3U << TIM_CCMR1_IC1PSC_Pos)     /*!< 0x0000000C */
#define TIM_CCMR1_IC1PSC                    TIM_CCMR1_IC1PSC_Msk               /*!<IC1PSC[1:0] bits (Input Capture 1 Prescaler) */
#define TIM_CCMR1_IC1PSC_0                  (0x1U << TIM_CCMR1_IC1PSC_Pos)     /*!< 0x00000004 */
#define TIM_CCMR1_IC1PSC_1                  (0x2U << TIM_CCMR1_IC1PSC_Pos)     /*!< 0x00000008 */

#define TIM_CCMR1_IC1F_Pos                  (4U)                               
#define TIM_CCMR1_IC1F_Msk                  (0xFU << TIM_CCMR1_IC1F_Pos)       /*!< 0x000000F0 */
#define TIM_CCMR1_IC1F                      TIM_CCMR1_IC1F_Msk                 /*!<IC1F[3:0] bits (Input Capture 1 Filter) */
#define TIM_CCMR1_IC1F_0                    (0x1U << TIM_CCMR1_IC1F_Pos)       /*!< 0x00000010 */
#define TIM_CCMR1_IC1F_1                    (0x2U << TIM_CCMR1_IC1F_Pos)       /*!< 0x00000020 */
#define TIM_CCMR1_IC1F_2                    (0x4U << TIM_CCMR1_IC1F_Pos)       /*!< 0x00000040 */
#define TIM_CCMR1_IC1F_3                    (0x8U << TIM_CCMR1_IC1F_Pos)       /*!< 0x00000080 */

#define TIM_CCMR1_IC2PSC_Pos                (10U)                              
#define TIM_CCMR1_IC2PSC_Msk                (0x3U << TIM_CCMR1_IC2PSC_Pos)     /*!< 0x00000C00 */
#define TIM_CCMR1_IC2PSC                    TIM_CCMR1_IC2PSC_Msk               /*!<IC2PSC[1:0] bits (Input Capture 2 Prescaler) */
#define TIM_CCMR1_IC2PSC_0                  (0x1U << TIM_CCMR1_IC2PSC_Pos)     /*!< 0x00000400 */
#define TIM_CCMR1_IC2PSC_1                  (0x2U << TIM_CCMR1_IC2PSC_Pos)     /*!< 0x00000800 */

#define TIM_CCMR1_IC2F_Pos                  (12U)                              
#define TIM_CCMR1_IC2F_Msk                  (0xFU << TIM_CCMR1_IC2F_Pos)       /*!< 0x0000F000 */
#define TIM_CCMR1_IC2F                      TIM_CCMR1_IC2F_Msk                 /*!<IC2F[3:0] bits (Input Capture 2 Filter) */
#define TIM_CCMR1_IC2F_0                    (0x1U << TIM_CCMR1_IC2F_Pos)       /*!< 0x00001000 */
#define TIM_CCMR1_IC2F_1                    (0x2U << TIM_CCMR1_IC2F_Pos)       /*!< 0x00002000 */
#define TIM_CCMR1_IC2F_2                    (0x4U << TIM_CCMR1_IC2F_Pos)       /*!< 0x00004000 */
#define TIM_CCMR1_IC2F_3                    (0x8U << TIM_CCMR1_IC2F_Pos)       /*!< 0x00008000 */

/******************  Bit definition for TIM_CCMR2 register  *******************/
#define TIM_CCMR2_CC3S_Pos                  (0U)                               
#define TIM_CCMR2_CC3S_Msk                  (0x3U << TIM_CCMR2_CC3S_Pos)       /*!< 0x00000003 */
#define TIM_CCMR2_CC3S                      TIM_CCMR2_CC3S_Msk                 /*!<CC3S[1:0] bits (Capture/Compare 3 Selection) */
#define TIM_CCMR2_CC3S_0                    (0x1U << TIM_CCMR2_CC3S_Pos)       /*!< 0x00000001 */
#define TIM_CCMR2_CC3S_1                    (0x2U << TIM_CCMR2_CC3S_Pos)       /*!< 0x00000002 */

#define TIM_CCMR2_OC3FE_Pos                 (2U)                               
#define TIM_CCMR2_OC3FE_Msk                 (0x1U << TIM_CCMR2_OC3FE_Pos)      /*!< 0x00000004 */
#define TIM_CCMR2_OC3FE                     TIM_CCMR2_OC3FE_Msk                /*!<Output Compare 3 Fast enable */
#define TIM_CCMR2_OC3PE_Pos                 (3U)                               
#define TIM_CCMR2_OC3PE_Msk                 (0x1U << TIM_CCMR2_OC3PE_Pos)      /*!< 0x00000008 */
#define TIM_CCMR2_OC3PE                     TIM_CCMR2_OC3PE_Msk                /*!<Output Compare 3 Preload enable */

#define TIM_CCMR2_OC3M_Pos                  (4U)                               
#define TIM_CCMR2_OC3M_Msk                  (0x7U << TIM_CCMR2_OC3M_Pos)       /*!< 0x00000070 */
#define TIM_CCMR2_OC3M                      TIM_CCMR2_OC3M_Msk                 /*!<OC3M[2:0] bits (Output Compare 3 Mode) */
#define TIM_CCMR2_OC3M_0                    (0x1U << TIM_CCMR2_OC3M_Pos)       /*!< 0x00000010 */
#define TIM_CCMR2_OC3M_1                    (0x2U << TIM_CCMR2_OC3M_Pos)       /*!< 0x00000020 */
#define TIM_CCMR2_OC3M_2                    (0x4U << TIM_CCMR2_OC3M_Pos)       /*!< 0x00000040 */

#define TIM_CCMR2_OC3CE_Pos                 (7U)                               
#define TIM_CCMR2_OC3CE_Msk                 (0x1U << TIM_CCMR2_OC3CE_Pos)      /*!< 0x00000080 */
#define TIM_CCMR2_OC3CE                     TIM_CCMR2_OC3CE_Msk                /*!<Output Compare 3 Clear Enable */

#define TIM_CCMR2_CC4S_Pos                  (8U)                               
#define TIM_CCMR2_CC4S_Msk                  (0x3U << TIM_CCMR2_CC4S_Pos)       /*!< 0x00000300 */
#define TIM_CCMR2_CC4S                      TIM_CCMR2_CC4S_Msk                 /*!<CC4S[1:0] bits (Capture/Compare 4 Selection) */
#define TIM_CCMR2_CC4S_0                    (0x1U << TIM_CCMR2_CC4S_Pos)       /*!< 0x00000100 */
#define TIM_CCMR2_CC4S_1                    (0x2U << TIM_CCMR2_CC4S_Pos)       /*!< 0x00000200 */

#define TIM_CCMR2_OC4FE_Pos                 (10U)                              
#define TIM_CCMR2_OC4FE_Msk                 (0x1U << TIM_CCMR2_OC4FE_Pos)      /*!< 0x00000400 */
#define TIM_CCMR2_OC4FE                     TIM_CCMR2_OC4FE_Msk                /*!<Output Compare 4 Fast enable */
#define TIM_CCMR2_OC4PE_Pos                 (11U)                              
#define TIM_CCMR2_OC4PE_Msk                 (0x1U << TIM_CCMR2_OC4PE_Pos)      /*!< 0x00000800 */
#define TIM_CCMR2_OC4PE                     TIM_CCMR2_OC4PE_Msk                /*!<Output Compare 4 Preload enable */

#define TIM_CCMR2_OC4M_Pos                  (12U)                              
#define TIM_CCMR2_OC4M_Msk                  (0x7U << TIM_CCMR2_OC4M_Pos)       /*!< 0x00007000 */
#define TIM_CCMR2_OC4M                      TIM_CCMR2_OC4M_Msk                 /*!<OC4M[2:0] bits (Output Compare 4 Mode) */
#define TIM_CCMR2_OC4M_0                    (0x1U << TIM_CCMR2_OC4M_Pos)       /*!< 0x00001000 */
#define TIM_CCMR2_OC4M_1                    (0x2U << TIM_CCMR2_OC4M_Pos)       /*!< 0x00002000 */
#define TIM_CCMR2_OC4M_2                    (0x4U << TIM_CCMR2_OC4M_Pos)       /*!< 0x00004000 */

#define TIM_CCMR2_OC4CE_Pos                 (15U)                              
#define TIM_CCMR2_OC4CE_Msk                 (0x1U << TIM_CCMR2_OC4CE_Pos)      /*!< 0x00008000 */
#define TIM_CCMR2_OC4CE                     TIM_CCMR2_OC4CE_Msk                /*!<Output Compare 4 Clear Enable */

/*----------------------------------------------------------------------------*/

#define TIM_CCMR2_IC3PSC_Pos                (2U)                               
#define TIM_CCMR2_IC3PSC_Msk                (0x3U << TIM_CCMR2_IC3PSC_Pos)     /*!< 0x0000000C */
#define TIM_CCMR2_IC3PSC                    TIM_CCMR2_IC3PSC_Msk               /*!<IC3PSC[1:0] bits (Input Capture 3 Prescaler) */
#define TIM_CCMR2_IC3PSC_0                  (0x1U << TIM_CCMR2_IC3PSC_Pos)     /*!< 0x00000004 */
#define TIM_CCMR2_IC3PSC_1                  (0x2U << TIM_CCMR2_IC3PSC_Pos)     /*!< 0x00000008 */

#define TIM_CCMR2_IC3F_Pos                  (4U)                               
#define TIM_CCMR2_IC3F_Msk                  (0xFU << TIM_CCMR2_IC3F_Pos)       /*!< 0x000000F0 */
#define TIM_CCMR2_IC3F                      TIM_CCMR2_IC3F_Msk                 /*!<IC3F[3:0] bits (Input Capture 3 Filter) */
#define TIM_CCMR2_IC3F_0                    (0x1U << TIM_CCMR2_IC3F_Pos)       /*!< 0x00000010 */
#define TIM_CCMR2_IC3F_1                    (0x2U << TIM_CCMR2_IC3F_Pos)       /*!< 0x00000020 */
#define TIM_CCMR2_IC3F_2                    (0x4U << TIM_CCMR2_IC3F_Pos)       /*!< 0x00000040 */
#define TIM_CCMR2_IC3F_3                    (0x8U << TIM_CCMR2_IC3F_Pos)       /*!< 0x00000080 */

#define TIM_CCMR2_IC4PSC_Pos                (10U)                              
#define TIM_CCMR2_IC4PSC_Msk                (0x3U << TIM_CCMR2_IC4PSC_Pos)     /*!< 0x00000C00 */
#define TIM_CCMR2_IC4PSC                    TIM_CCMR2_IC4PSC_Msk               /*!<IC4PSC[1:0] bits (Input Capture 4 Prescaler) */
#define TIM_CCMR2_IC4PSC_0                  (0x1U << TIM_CCMR2_IC4PSC_Pos)     /*!< 0x00000400 */
#define TIM_CCMR2_IC4PSC_1                  (0x2U << TIM_CCMR2_IC4PSC_Pos)     /*!< 0x00000800 */

#define TIM_CCMR2_IC4F_Pos                  (12U)                              
#define TIM_CCMR2_IC4F_Msk                  (0xFU << TIM_CCMR2_IC4F_Pos)       /*!< 0x0000F000 */
#define TIM_CCMR2_IC4F                      TIM_CCMR2_IC4F_Msk                 /*!<IC4F[3:0] bits (Input Capture 4 Filter) */
#define TIM_CCMR2_IC4F_0                    (0x1U << TIM_CCMR2_IC4F_Pos)       /*!< 0x00001000 */
#define TIM_CCMR2_IC4F_1                    (0x2U << TIM_CCMR2_IC4F_Pos)       /*!< 0x00002000 */
#define TIM_CCMR2_IC4F_2                    (0x4U << TIM_CCMR2_IC4F_Pos)       /*!< 0x00004000 */
#define TIM_CCMR2_IC4F_3                    (0x8U << TIM_CCMR2_IC4F_Pos)       /*!< 0x00008000 */

/*******************  Bit definition for TIM_CCER register  *******************/
#define TIM_CCER_CC1E_Pos                   (0U)                               
#define TIM_CCER_CC1E_Msk                   (0x1U << TIM_CCER_CC1E_Pos)        /*!< 0x00000001 */
#define TIM_CCER_CC1E                       TIM_CCER_CC1E_Msk                  /*!<Capture/Compare 1 output enable */
#define TIM_CCER_CC1P_Pos                   (1U)                               
#define TIM_CCER_CC1P_Msk                   (0x1U << TIM_CCER_CC1P_Pos)        /*!< 0x00000002 */
#define TIM_CCER_CC1P                       TIM_CCER_CC1P_Msk                  /*!<Capture/Compare 1 output Polarity */
#define TIM_CCER_CC1NP_Pos                  (3U)                               
#define TIM_CCER_CC1NP_Msk                  (0x1U << TIM_CCER_CC1NP_Pos)       /*!< 0x00000008 */
#define TIM_CCER_CC1NP                      TIM_CCER_CC1NP_Msk                 /*!<Capture/Compare 1 Complementary output Polarity */
#define TIM_CCER_CC2E_Pos                   (4U)                               
#define TIM_CCER_CC2E_Msk                   (0x1U << TIM_CCER_CC2E_Pos)        /*!< 0x00000010 */
#define TIM_CCER_CC2E                       TIM_CCER_CC2E_Msk                  /*!<Capture/Compare 2 output enable */
#define TIM_CCER_CC2P_Pos                   (5U)                               
#define TIM_CCER_CC2P_Msk                   (0x1U << TIM_CCER_CC2P_Pos)        /*!< 0x00000020 */
#define TIM_CCER_CC2P                       TIM_CCER_CC2P_Msk                  /*!<Capture/Compare 2 output Polarity */
#define TIM_CCER_CC2NP_Pos                  (7U)                               
#define TIM_CCER_CC2NP_Msk                  (0x1U << TIM_CCER_CC2NP_Pos)       /*!< 0x00000080 */
#define TIM_CCER_CC2NP                      TIM_CCER_CC2NP_Msk                 /*!<Capture/Compare 2 Complementary output Polarity */
#define TIM_CCER_CC3E_Pos                   (8U)                               
#define TIM_CCER_CC3E_Msk                   (0x1U << TIM_CCER_CC3E_Pos)        /*!< 0x00000100 */
#define TIM_CCER_CC3E                       TIM_CCER_CC3E_Msk                  /*!<Capture/Compare 3 output enable */
#define TIM_CCER_CC3P_Pos                   (9U)                               
#define TIM_CCER_CC3P_Msk                   (0x1U << TIM_CCER_CC3P_Pos)        /*!< 0x00000200 */
#define TIM_CCER_CC3P                       TIM_CCER_CC3P_Msk                  /*!<Capture/Compare 3 output Polarity */
#define TIM_CCER_CC3NP_Pos                  (11U)                              
#define TIM_CCER_CC3NP_Msk                  (0x1U << TIM_CCER_CC3NP_Pos)       /*!< 0x00000800 */
#define TIM_CCER_CC3NP                      TIM_CCER_CC3NP_Msk                 /*!<Capture/Compare 3 Complementary output Polarity */
#define TIM_CCER_CC4E_Pos                   (12U)                              
#define TIM_CCER_CC4E_Msk                   (0x1U << TIM_CCER_CC4E_Pos)        /*!< 0x00001000 */
#define TIM_CCER_CC4E                       TIM_CCER_CC4E_Msk                  /*!<Capture/Compare 4 output enable */
#define TIM_CCER_CC4P_Pos                   (13U)                              
#define TIM_CCER_CC4P_Msk                   (0x1U << TIM_CCER_CC4P_Pos)        /*!< 0x00002000 */
#define TIM_CCER_CC4P                       TIM_CCER_CC4P_Msk                  /*!<Capture/Compare 4 output Polarity */
#define TIM_CCER_CC4NP_Pos                  (15U)                              
#define TIM_CCER_CC4NP_Msk                  (0x1U << TIM_CCER_CC4NP_Pos)       /*!< 0x00008000 */
#define TIM_CCER_CC4NP                      TIM_CCER_CC4NP_Msk                 /*!<Capture/Compare 4 Complementary output Polarity */

/*******************  Bit definition for TIM_CNT register  ********************/
#define TIM_CNT_CNT_Pos                     (0U)                               
#define TIM_CNT_CNT_Msk                     (0xFFFFFFFFU << TIM_CNT_CNT_Pos)   /*!< 0xFFFFFFFF */
#define TIM_CNT_CNT                         TIM_CNT_CNT_Msk                    /*!<Counter Value */

/*******************  Bit definition for TIM_PSC register  ********************/
#define TIM_PSC_PSC_Pos                     (0U)                               
#define TIM_PSC_PSC_Msk                     (0xFFFFU << TIM_PSC_PSC_Pos)       /*!< 0x0000FFFF */
#define TIM_PSC_PSC                         TIM_PSC_PSC_Msk                    /*!<Prescaler Value */

/*******************  Bit definition for TIM_ARR register  ********************/
#define TIM_ARR_ARR_Pos                     (0U)                               
#define TIM_ARR_ARR_Msk                     (0xFFFFFFFFU << TIM_ARR_ARR_Pos)   /*!< 0xFFFFFFFF */
#define TIM_ARR_ARR                         TIM_ARR_ARR_Msk                    /*!<actual auto-reload Value */
           
/*******************  Bit definition for TIM_CCR1 register  *******************/
#define TIM_CCR1_CCR1_Pos                   (0U)                               
#define TIM_CCR1_CCR1_Msk                   (0xFFFFU << TIM_CCR1_CCR1_Pos)     /*!< 0x0000FFFF */
#define TIM_CCR1_CCR1                       TIM_CCR1_CCR1_Msk                  /*!<Capture/Compare 1 Value */

/*******************  Bit definition for TIM_CCR2 register  *******************/
#define TIM_CCR2_CCR2_Pos                   (0U)                               
#define TIM_CCR2_CCR2_Msk                   (0xFFFFU << TIM_CCR2_CCR2_Pos)     /*!< 0x0000FFFF */
#define TIM_CCR2_CCR2                       TIM_CCR2_CCR2_Msk                  /*!<Capture/Compare 2 Value */

/*******************  Bit definition for TIM_CCR3 register  *******************/
#define TIM_CCR3_CCR3_Pos                   (0U)                               
#define TIM_CCR3_CCR3_Msk                   (0xFFFFU << TIM_CCR3_CCR3_Pos)     /*!< 0x0000FFFF */
#define TIM_CCR3_CCR3                       TIM_CCR3_CCR3_Msk                  /*!<Capture/Compare 3 Value */

/*******************  Bit definition for TIM_CCR4 register  *******************/
#define TIM_CCR4_CCR4_Pos                   (0U)                               
#define TIM_CCR4_CCR4_Msk                   (0xFFFFU << TIM_CCR4_CCR4_Pos)     /*!< 0x0000FFFF */
#define TIM_CCR4_CCR4                       TIM_CCR4_CCR4_Msk                  /*!<Capture/Compare 4 Value */

/*******************  Bit definition for TIM_DCR register  ********************/
#define TIM_DCR_DBA_Pos                     (0U)                               
#define TIM_DCR_DBA_Msk                     (0x1FU << TIM_DCR_DBA_Pos)         /*!< 0x0000001F */
#define TIM_DCR_DBA                         TIM_DCR_DBA_Msk                    /*!<DBA[4:0] bits (DMA Base Address) */
#define TIM_DCR_DBA_0                       (0x01U << TIM_DCR_DBA_Pos)         /*!< 0x00000001 */
#define TIM_DCR_DBA_1                       (0x02U << TIM_DCR_DBA_Pos)         /*!< 0x00000002 */
#define TIM_DCR_DBA_2                       (0x04U << TIM_DCR_DBA_Pos)         /*!< 0x00000004 */
#define TIM_DCR_DBA_3                       (0x08U << TIM_DCR_DBA_Pos)         /*!< 0x00000008 */
#define TIM_DCR_DBA_4                       (0x10U << TIM_DCR_DBA_Pos)         /*!< 0x00000010 */

#define TIM_DCR_DBL_Pos                     (8U)                               
#define TIM_DCR_DBL_Msk                     (0x1FU << TIM_DCR_DBL_Pos)         /*!< 0x00001F00 */
#define TIM_DCR_DBL                         TIM_DCR_DBL_Msk                    /*!<DBL[4:0] bits (DMA Burst Length) */
#define TIM_DCR_DBL_0                       (0x01U << TIM_DCR_DBL_Pos)         /*!< 0x00000100 */
#define TIM_DCR_DBL_1                       (0x02U << TIM_DCR_DBL_Pos)         /*!< 0x00000200 */
#define TIM_DCR_DBL_2                       (0x04U << TIM_DCR_DBL_Pos)         /*!< 0x00000400 */
#define TIM_DCR_DBL_3                       (0x08U << TIM_DCR_DBL_Pos)         /*!< 0x00000800 */
#define TIM_DCR_DBL_4                       (0x10U << TIM_DCR_DBL_Pos)         /*!< 0x00001000 */

/*******************  Bit definition for TIM_DMAR register  *******************/
#define TIM_DMAR_DMAB_Pos                   (0U)                               
#define TIM_DMAR_DMAB_Msk                   (0xFFFFU << TIM_DMAR_DMAB_Pos)     /*!< 0x0000FFFF */
#define TIM_DMAR_DMAB                       TIM_DMAR_DMAB_Msk                  /*!<DMA register for burst accesses */

/*******************  Bit definition for TIM_OR register  *********************/
#define TIM_OR_TI1RMP_Pos                   (0U)                               
#define TIM_OR_TI1RMP_Msk                   (0x3U << TIM_OR_TI1RMP_Pos)        /*!< 0x00000003 */
#define TIM_OR_TI1RMP                       TIM_OR_TI1RMP_Msk                  /*!<TI1_RMP[1:0] bits (TIM Input 1 remap) */
#define TIM_OR_TI1RMP_0                     (0x1U << TIM_OR_TI1RMP_Pos)        /*!< 0x00000001 */
#define TIM_OR_TI1RMP_1                     (0x2U << TIM_OR_TI1RMP_Pos)        /*!< 0x00000002 */

#define TIM_OR_ETR_RMP_Pos                  (2U)                               
#define TIM_OR_ETR_RMP_Msk                  (0x1U << TIM_OR_ETR_RMP_Pos)       /*!< 0x00000004 */
#define TIM_OR_ETR_RMP                      TIM_OR_ETR_RMP_Msk                 /*!<ETR_RMP bit (TIM10/11 ETR remap)*/
#define TIM_OR_TI1_RMP_RI_Pos               (3U)                               
#define TIM_OR_TI1_RMP_RI_Msk               (0x1U << TIM_OR_TI1_RMP_RI_Pos)    /*!< 0x00000008 */
#define TIM_OR_TI1_RMP_RI                   TIM_OR_TI1_RMP_RI_Msk              /*!<TI1_RMP_RI bit (TIM10/11 Input 1 remap for Routing interface) */

/*----------------------------------------------------------------------------*/
#define TIM9_OR_ITR1_RMP_Pos                (2U)                               
#define TIM9_OR_ITR1_RMP_Msk                (0x1U << TIM9_OR_ITR1_RMP_Pos)     /*!< 0x00000004 */
#define TIM9_OR_ITR1_RMP                    TIM9_OR_ITR1_RMP_Msk               /*!<ITR1_RMP bit (TIM9 Internal trigger 1 remap) */

/*----------------------------------------------------------------------------*/
#define TIM2_OR_ITR1_RMP_Pos                (0U)                               
#define TIM2_OR_ITR1_RMP_Msk                (0x1U << TIM2_OR_ITR1_RMP_Pos)     /*!< 0x00000001 */
#define TIM2_OR_ITR1_RMP                    TIM2_OR_ITR1_RMP_Msk               /*!<ITR1_RMP bit (TIM2 Internal trigger 1 remap) */

/*----------------------------------------------------------------------------*/
#define TIM3_OR_ITR2_RMP_Pos                (0U)                               
#define TIM3_OR_ITR2_RMP_Msk                (0x1U << TIM3_OR_ITR2_RMP_Pos)     /*!< 0x00000001 */
#define TIM3_OR_ITR2_RMP                    TIM3_OR_ITR2_RMP_Msk               /*!<ITR2_RMP bit (TIM3 Internal trigger 2 remap) */

/*----------------------------------------------------------------------------*/

/******************************************************************************/
/*                                                                            */
/*      Universal Synchronous Asynchronous Receiver Transmitter (USART)       */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for USART_SR register  *******************/
#define USART_SR_PE_Pos                     (0U)                               
#define USART_SR_PE_Msk                     (0x1U << USART_SR_PE_Pos)          /*!< 0x00000001 */
#define USART_SR_PE                         USART_SR_PE_Msk                    /*!< Parity Error */
#define USART_SR_FE_Pos                     (1U)                               
#define USART_SR_FE_Msk                     (0x1U << USART_SR_FE_Pos)          /*!< 0x00000002 */
#define USART_SR_FE                         USART_SR_FE_Msk                    /*!< Framing Error */
#define USART_SR_NE_Pos                     (2U)                               
#define USART_SR_NE_Msk                     (0x1U << USART_SR_NE_Pos)          /*!< 0x00000004 */
#define USART_SR_NE                         USART_SR_NE_Msk                    /*!< Noise Error Flag */
#define USART_SR_ORE_Pos                    (3U)                               
#define USART_SR_ORE_Msk                    (0x1U << USART_SR_ORE_Pos)         /*!< 0x00000008 */
#define USART_SR_ORE                        USART_SR_ORE_Msk                   /*!< OverRun Error */
#define USART_SR_IDLE_Pos                   (4U)                               
#define USART_SR_IDLE_Msk                   (0x1U << USART_SR_IDLE_Pos)        /*!< 0x00000010 */
#define USART_SR_IDLE                       USART_SR_IDLE_Msk                  /*!< IDLE line detected */
#define USART_SR_RXNE_Pos                   (5U)                               
#define USART_SR_RXNE_Msk                   (0x1U << USART_SR_RXNE_Pos)        /*!< 0x00000020 */
#define USART_SR_RXNE                       USART_SR_RXNE_Msk                  /*!< Read Data Register Not Empty */
#define USART_SR_TC_Pos                     (6U)                               
#define USART_SR_TC_Msk                     (0x1U << USART_SR_TC_Pos)          /*!< 0x00000040 */
#define USART_SR_TC                         USART_SR_TC_Msk                    /*!< Transmission Complete */
#define USART_SR_TXE_Pos                    (7U)                               
#define USART_SR_TXE_Msk                    (0x1U << USART_SR_TXE_Pos)         /*!< 0x00000080 */
#define USART_SR_TXE                        USART_SR_TXE_Msk                   /*!< Transmit Data Register Empty */
#define USART_SR_LBD_Pos                    (8U)                               
#define USART_SR_LBD_Msk                    (0x1U << USART_SR_LBD_Pos)         /*!< 0x00000100 */
#define USART_SR_LBD                        USART_SR_LBD_Msk                   /*!< LIN Break Detection Flag */
#define USART_SR_CTS_Pos                    (9U)                               
#define USART_SR_CTS_Msk                    (0x1U << USART_SR_CTS_Pos)         /*!< 0x00000200 */
#define USART_SR_CTS                        USART_SR_CTS_Msk                   /*!< CTS Flag */

/*******************  Bit definition for USART_DR register  *******************/
#define USART_DR_DR_Pos                     (0U)                               
#define USART_DR_DR_Msk                     (0x1FFU << USART_DR_DR_Pos)        /*!< 0x000001FF */
#define USART_DR_DR                         USART_DR_DR_Msk                    /*!< Data value */

/******************  Bit definition for USART_BRR register  *******************/
#define USART_BRR_DIV_FRACTION_Pos          (0U)                               
#define USART_BRR_DIV_FRACTION_Msk          (0xFU << USART_BRR_DIV_FRACTION_Pos) /*!< 0x0000000F */
#define USART_BRR_DIV_FRACTION              USART_BRR_DIV_FRACTION_Msk         /*!< Fraction of USARTDIV */
#define USART_BRR_DIV_MANTISSA_Pos          (4U)                               
#define USART_BRR_DIV_MANTISSA_Msk          (0xFFFU << USART_BRR_DIV_MANTISSA_Pos) /*!< 0x0000FFF0 */
#define USART_BRR_DIV_MANTISSA              USART_BRR_DIV_MANTISSA_Msk         /*!< Mantissa of USARTDIV */

/******************  Bit definition for USART_CR1 register  *******************/
#define USART_CR1_SBK_Pos                   (0U)                               
#define USART_CR1_SBK_Msk                   (0x1U << USART_CR1_SBK_Pos)        /*!< 0x00000001 */
#define USART_CR1_SBK                       USART_CR1_SBK_Msk                  /*!< Send Break */
#define USART_CR1_RWU_Pos                   (1U)                               
#define USART_CR1_RWU_Msk                   (0x1U << USART_CR1_RWU_Pos)        /*!< 0x00000002 */
#define USART_CR1_RWU                       USART_CR1_RWU_Msk                  /*!< Receiver wakeup */
#define USART_CR1_RE_Pos                    (2U)                               
#define USART_CR1_RE_Msk                    (0x1U << USART_CR1_RE_Pos)         /*!< 0x00000004 */
#define USART_CR1_RE                        USART_CR1_RE_Msk                   /*!< Receiver Enable */
#define USART_CR1_TE_Pos                    (3U)                               
#define USART_CR1_TE_Msk                    (0x1U << USART_CR1_TE_Pos)         /*!< 0x00000008 */
#define USART_CR1_TE                        USART_CR1_TE_Msk                   /*!< Transmitter Enable */
#define USART_CR1_IDLEIE_Pos                (4U)                               
#define USART_CR1_IDLEIE_Msk                (0x1U << USART_CR1_IDLEIE_Pos)     /*!< 0x00000010 */
#define USART_CR1_IDLEIE                    USART_CR1_IDLEIE_Msk               /*!< IDLE Interrupt Enable */
#define USART_CR1_RXNEIE_Pos                (5U)                               
#define USART_CR1_RXNEIE_Msk                (0x1U << USART_CR1_RXNEIE_Pos)     /*!< 0x00000020 */
#define USART_CR1_RXNEIE                    USART_CR1_RXNEIE_Msk               /*!< RXNE Interrupt Enable */
#define USART_CR1_TCIE_Pos                  (6U)                               
#define USART_CR1_TCIE_Msk                  (0x1U << USART_CR1_TCIE_Pos)       /*!< 0x00000040 */
#define USART_CR1_TCIE                      USART_CR1_TCIE_Msk                 /*!< Transmission Complete Interrupt Enable */
#define USART_CR1_TXEIE_Pos                 (7U)                               
#define USART_CR1_TXEIE_Msk                 (0x1U << USART_CR1_TXEIE_Pos)      /*!< 0x00000080 */
#define USART_CR1_TXEIE                     USART_CR1_TXEIE_Msk                /*!< PE Interrupt Enable */
#define USART_CR1_PEIE_Pos                  (8U)                               
#define USART_CR1_PEIE_Msk                  (0x1U << USART_CR1_PEIE_Pos)       /*!< 0x00000100 */
#define USART_CR1_PEIE                      USART_CR1_PEIE_Msk                 /*!< PE Interrupt Enable */
#define USART_CR1_PS_Pos                    (9U)                               
#define USART_CR1_PS_Msk                    (0x1U << USART_CR1_PS_Pos)         /*!< 0x00000200 */
#define USART_CR1_PS                        USART_CR1_PS_Msk                   /*!< Parity Selection */
#define USART_CR1_PCE_Pos                   (10U)                              
#define USART_CR1_PCE_Msk                   (0x1U << USART_CR1_PCE_Pos)        /*!< 0x00000400 */
#define USART_CR1_PCE                       USART_CR1_PCE_Msk                  /*!< Parity Control Enable */
#define USART_CR1_WAKE_Pos                  (11U)                              
#define USART_CR1_WAKE_Msk                  (0x1U << USART_CR1_WAKE_Pos)       /*!< 0x00000800 */
#define USART_CR1_WAKE                      USART_CR1_WAKE_Msk                 /*!< Wakeup method */
#define USART_CR1_M_Pos                     (12U)                              
#define USART_CR1_M_Msk                     (0x1U << USART_CR1_M_Pos)          /*!< 0x00001000 */
#define USART_CR1_M                         USART_CR1_M_Msk                    /*!< Word length */
#define USART_CR1_UE_Pos                    (13U)                              
#define USART_CR1_UE_Msk                    (0x1U << USART_CR1_UE_Pos)         /*!< 0x00002000 */
#define USART_CR1_UE                        USART_CR1_UE_Msk                   /*!< USART Enable */
#define USART_CR1_OVER8_Pos                 (15U)                              
#define USART_CR1_OVER8_Msk                 (0x1U << USART_CR1_OVER8_Pos)      /*!< 0x00008000 */
#define USART_CR1_OVER8                     USART_CR1_OVER8_Msk                /*!< Oversampling by 8-bit mode */

/******************  Bit definition for USART_CR2 register  *******************/
#define USART_CR2_ADD_Pos                   (0U)                               
#define USART_CR2_ADD_Msk                   (0xFU << USART_CR2_ADD_Pos)        /*!< 0x0000000F */
#define USART_CR2_ADD                       USART_CR2_ADD_Msk                  /*!< Address of the USART node */
#define USART_CR2_LBDL_Pos                  (5U)                               
#define USART_CR2_LBDL_Msk                  (0x1U << USART_CR2_LBDL_Pos)       /*!< 0x00000020 */
#define USART_CR2_LBDL                      USART_CR2_LBDL_Msk                 /*!< LIN Break Detection Length */
#define USART_CR2_LBDIE_Pos                 (6U)                               
#define USART_CR2_LBDIE_Msk                 (0x1U << USART_CR2_LBDIE_Pos)      /*!< 0x00000040 */
#define USART_CR2_LBDIE                     USART_CR2_LBDIE_Msk                /*!< LIN Break Detection Interrupt Enable */
#define USART_CR2_LBCL_Pos                  (8U)                               
#define USART_CR2_LBCL_Msk                  (0x1U << USART_CR2_LBCL_Pos)       /*!< 0x00000100 */
#define USART_CR2_LBCL                      USART_CR2_LBCL_Msk                 /*!< Last Bit Clock pulse */
#define USART_CR2_CPHA_Pos                  (9U)                               
#define USART_CR2_CPHA_Msk                  (0x1U << USART_CR2_CPHA_Pos)       /*!< 0x00000200 */
#define USART_CR2_CPHA                      USART_CR2_CPHA_Msk                 /*!< Clock Phase */
#define USART_CR2_CPOL_Pos                  (10U)                              
#define USART_CR2_CPOL_Msk                  (0x1U << USART_CR2_CPOL_Pos)       /*!< 0x00000400 */
#define USART_CR2_CPOL                      USART_CR2_CPOL_Msk                 /*!< Clock Polarity */
#define USART_CR2_CLKEN_Pos                 (11U)                              
#define USART_CR2_CLKEN_Msk                 (0x1U << USART_CR2_CLKEN_Pos)      /*!< 0x00000800 */
#define USART_CR2_CLKEN                     USART_CR2_CLKEN_Msk                /*!< Clock Enable */

#define USART_CR2_STOP_Pos                  (12U)                              
#define USART_CR2_STOP_Msk                  (0x3U << USART_CR2_STOP_Pos)       /*!< 0x00003000 */
#define USART_CR2_STOP                      USART_CR2_STOP_Msk                 /*!< STOP[1:0] bits (STOP bits) */
#define USART_CR2_STOP_0                    (0x1U << USART_CR2_STOP_Pos)       /*!< 0x00001000 */
#define USART_CR2_STOP_1                    (0x2U << USART_CR2_STOP_Pos)       /*!< 0x00002000 */

#define USART_CR2_LINEN_Pos                 (14U)                              
#define USART_CR2_LINEN_Msk                 (0x1U << USART_CR2_LINEN_Pos)      /*!< 0x00004000 */
#define USART_CR2_LINEN                     USART_CR2_LINEN_Msk                /*!< LIN mode enable */

/******************  Bit definition for USART_CR3 register  *******************/
#define USART_CR3_EIE_Pos                   (0U)                               
#define USART_CR3_EIE_Msk                   (0x1U << USART_CR3_EIE_Pos)        /*!< 0x00000001 */
#define USART_CR3_EIE                       USART_CR3_EIE_Msk                  /*!< Error Interrupt Enable */
#define USART_CR3_IREN_Pos                  (1U)                               
#define USART_CR3_IREN_Msk                  (0x1U << USART_CR3_IREN_Pos)       /*!< 0x00000002 */
#define USART_CR3_IREN                      USART_CR3_IREN_Msk                 /*!< IrDA mode Enable */
#define USART_CR3_IRLP_Pos                  (2U)                               
#define USART_CR3_IRLP_Msk                  (0x1U << USART_CR3_IRLP_Pos)       /*!< 0x00000004 */
#define USART_CR3_IRLP                      USART_CR3_IRLP_Msk                 /*!< IrDA Low-Power */
#define USART_CR3_HDSEL_Pos                 (3U)                               
#define USART_CR3_HDSEL_Msk                 (0x1U << USART_CR3_HDSEL_Pos)      /*!< 0x00000008 */
#define USART_CR3_HDSEL                     USART_CR3_HDSEL_Msk                /*!< Half-Duplex Selection */
#define USART_CR3_NACK_Pos                  (4U)                               
#define USART_CR3_NACK_Msk                  (0x1U << USART_CR3_NACK_Pos)       /*!< 0x00000010 */
#define USART_CR3_NACK                      USART_CR3_NACK_Msk                 /*!< Smartcard NACK enable */
#define USART_CR3_SCEN_Pos                  (5U)                               
#define USART_CR3_SCEN_Msk                  (0x1U << USART_CR3_SCEN_Pos)       /*!< 0x00000020 */
#define USART_CR3_SCEN                      USART_CR3_SCEN_Msk                 /*!< Smartcard mode enable */
#define USART_CR3_DMAR_Pos                  (6U)                               
#define USART_CR3_DMAR_Msk                  (0x1U << USART_CR3_DMAR_Pos)       /*!< 0x00000040 */
#define USART_CR3_DMAR                      USART_CR3_DMAR_Msk                 /*!< DMA Enable Receiver */
#define USART_CR3_DMAT_Pos                  (7U)                               
#define USART_CR3_DMAT_Msk                  (0x1U << USART_CR3_DMAT_Pos)       /*!< 0x00000080 */
#define USART_CR3_DMAT                      USART_CR3_DMAT_Msk                 /*!< DMA Enable Transmitter */
#define USART_CR3_RTSE_Pos                  (8U)                               
#define USART_CR3_RTSE_Msk                  (0x1U << USART_CR3_RTSE_Pos)       /*!< 0x00000100 */
#define USART_CR3_RTSE                      USART_CR3_RTSE_Msk                 /*!< RTS Enable */
#define USART_CR3_CTSE_Pos                  (9U)                               
#define USART_CR3_CTSE_Msk                  (0x1U << USART_CR3_CTSE_Pos)       /*!< 0x00000200 */
#define USART_CR3_CTSE                      USART_CR3_CTSE_Msk                 /*!< CTS Enable */
#define USART_CR3_CTSIE_Pos                 (10U)                              
#define USART_CR3_CTSIE_Msk                 (0x1U << USART_CR3_CTSIE_Pos)      /*!< 0x00000400 */
#define USART_CR3_CTSIE                     USART_CR3_CTSIE_Msk                /*!< CTS Interrupt Enable */
#define USART_CR3_ONEBIT_Pos                (11U)                              
#define USART_CR3_ONEBIT_Msk                (0x1U << USART_CR3_ONEBIT_Pos)     /*!< 0x00000800 */
#define USART_CR3_ONEBIT                    USART_CR3_ONEBIT_Msk               /*!< One sample bit method enable */

/******************  Bit definition for USART_GTPR register  ******************/
#define USART_GTPR_PSC_Pos                  (0U)                               
#define USART_GTPR_PSC_Msk                  (0xFFU << USART_GTPR_PSC_Pos)      /*!< 0x000000FF */
#define USART_GTPR_PSC                      USART_GTPR_PSC_Msk                 /*!< PSC[7:0] bits (Prescaler value) */
#define USART_GTPR_PSC_0                    (0x01U << USART_GTPR_PSC_Pos)      /*!< 0x00000001 */
#define USART_GTPR_PSC_1                    (0x02U << USART_GTPR_PSC_Pos)      /*!< 0x00000002 */
#define USART_GTPR_PSC_2                    (0x04U << USART_GTPR_PSC_Pos)      /*!< 0x00000004 */
#define USART_GTPR_PSC_3                    (0x08U << USART_GTPR_PSC_Pos)      /*!< 0x00000008 */
#define USART_GTPR_PSC_4                    (0x10U << USART_GTPR_PSC_Pos)      /*!< 0x00000010 */
#define USART_GTPR_PSC_5                    (0x20U << USART_GTPR_PSC_Pos)      /*!< 0x00000020 */
#define USART_GTPR_PSC_6                    (0x40U << USART_GTPR_PSC_Pos)      /*!< 0x00000040 */
#define USART_GTPR_PSC_7                    (0x80U << USART_GTPR_PSC_Pos)      /*!< 0x00000080 */

#define USART_GTPR_GT_Pos                   (8U)                               
#define USART_GTPR_GT_Msk                   (0xFFU << USART_GTPR_GT_Pos)       /*!< 0x0000FF00 */
#define USART_GTPR_GT                       USART_GTPR_GT_Msk                  /*!< Guard time value */

/******************************************************************************/
/*                                                                            */
/*                     Universal Serial Bus (USB)                             */
/*                                                                            */
/******************************************************************************/

/*!<Endpoint-specific registers */

#define  USB_EP0R                              USB_BASE                        /*!< endpoint 0 register address */
#define  USB_EP1R                             (USB_BASE + 0x00000004U)         /*!< endpoint 1 register address */
#define  USB_EP2R                             (USB_BASE + 0x00000008U)         /*!< endpoint 2 register address */
#define  USB_EP3R                             (USB_BASE + 0x0000000CU)         /*!< endpoint 3 register address */
#define  USB_EP4R                             (USB_BASE + 0x00000010U)         /*!< endpoint 4 register address */
#define  USB_EP5R                             (USB_BASE + 0x00000014U)         /*!< endpoint 5 register address */
#define  USB_EP6R                             (USB_BASE + 0x00000018U)         /*!< endpoint 6 register address */
#define  USB_EP7R                             (USB_BASE + 0x0000001CU)         /*!< endpoint 7 register address */

/* bit positions */ 
#define USB_EP_CTR_RX_Pos                     (15U)                            
#define USB_EP_CTR_RX_Msk                     (0x1U << USB_EP_CTR_RX_Pos)      /*!< 0x00008000 */
#define USB_EP_CTR_RX                         USB_EP_CTR_RX_Msk                /*!<  EndPoint Correct TRansfer RX */
#define USB_EP_DTOG_RX_Pos                    (14U)                            
#define USB_EP_DTOG_RX_Msk                    (0x1U << USB_EP_DTOG_RX_Pos)     /*!< 0x00004000 */
#define USB_EP_DTOG_RX                        USB_EP_DTOG_RX_Msk               /*!<  EndPoint Data TOGGLE RX */
#define USB_EPRX_STAT_Pos                     (12U)                            
#define USB_EPRX_STAT_Msk                     (0x3U << USB_EPRX_STAT_Pos)      /*!< 0x00003000 */
#define USB_EPRX_STAT                         USB_EPRX_STAT_Msk                /*!<  EndPoint RX STATus bit field */
#define USB_EP_SETUP_Pos                      (11U)                            
#define USB_EP_SETUP_Msk                      (0x1U << USB_EP_SETUP_Pos)       /*!< 0x00000800 */
#define USB_EP_SETUP                          USB_EP_SETUP_Msk                 /*!<  EndPoint SETUP */
#define USB_EP_T_FIELD_Pos                    (9U)                             
#define USB_EP_T_FIELD_Msk                    (0x3U << USB_EP_T_FIELD_Pos)     /*!< 0x00000600 */
#define USB_EP_T_FIELD                        USB_EP_T_FIELD_Msk               /*!<  EndPoint TYPE */
#define USB_EP_KIND_Pos                       (8U)                             
#define USB_EP_KIND_Msk                       (0x1U << USB_EP_KIND_Pos)        /*!< 0x00000100 */
#define USB_EP_KIND                           USB_EP_KIND_Msk                  /*!<  EndPoint KIND */
#define USB_EP_CTR_TX_Pos                     (7U)                             
#define USB_EP_CTR_TX_Msk                     (0x1U << USB_EP_CTR_TX_Pos)      /*!< 0x00000080 */
#define USB_EP_CTR_TX                         USB_EP_CTR_TX_Msk                /*!<  EndPoint Correct TRansfer TX */
#define USB_EP_DTOG_TX_Pos                    (6U)                             
#define USB_EP_DTOG_TX_Msk                    (0x1U << USB_EP_DTOG_TX_Pos)     /*!< 0x00000040 */
#define USB_EP_DTOG_TX                        USB_EP_DTOG_TX_Msk               /*!<  EndPoint Data TOGGLE TX */
#define USB_EPTX_STAT_Pos                     (4U)                             
#define USB_EPTX_STAT_Msk                     (0x3U << USB_EPTX_STAT_Pos)      /*!< 0x00000030 */
#define USB_EPTX_STAT                         USB_EPTX_STAT_Msk                /*!<  EndPoint TX STATus bit field */
#define USB_EPADDR_FIELD_Pos                  (0U)                             
#define USB_EPADDR_FIELD_Msk                  (0xFU << USB_EPADDR_FIELD_Pos)   /*!< 0x0000000F */
#define USB_EPADDR_FIELD                      USB_EPADDR_FIELD_Msk             /*!<  EndPoint ADDRess FIELD */

/* EndPoint REGister MASK (no toggle fields) */
#define  USB_EPREG_MASK     (USB_EP_CTR_RX|USB_EP_SETUP|USB_EP_T_FIELD|USB_EP_KIND|USB_EP_CTR_TX|USB_EPADDR_FIELD)
                                                                               /*!< EP_TYPE[1:0] EndPoint TYPE */
#define USB_EP_TYPE_MASK_Pos                  (9U)                             
#define USB_EP_TYPE_MASK_Msk                  (0x3U << USB_EP_TYPE_MASK_Pos)   /*!< 0x00000600 */
#define USB_EP_TYPE_MASK                      USB_EP_TYPE_MASK_Msk             /*!< EndPoint TYPE Mask */
#define USB_EP_BULK                           (0x00000000U)                    /*!< EndPoint BULK */
#define USB_EP_CONTROL                        (0x00000200U)                    /*!< EndPoint CONTROL */
#define USB_EP_ISOCHRONOUS                    (0x00000400U)                    /*!< EndPoint ISOCHRONOUS */
#define USB_EP_INTERRUPT                      (0x00000600U)                    /*!< EndPoint INTERRUPT */
#define  USB_EP_T_MASK      (~USB_EP_T_FIELD & USB_EPREG_MASK)
                                                                 
#define  USB_EPKIND_MASK    (~USB_EP_KIND & USB_EPREG_MASK)            /*!< EP_KIND EndPoint KIND */
                                                                               /*!< STAT_TX[1:0] STATus for TX transfer */
#define USB_EP_TX_DIS                         (0x00000000U)                    /*!< EndPoint TX DISabled */
#define USB_EP_TX_STALL                       (0x00000010U)                    /*!< EndPoint TX STALLed */
#define USB_EP_TX_NAK                         (0x00000020U)                    /*!< EndPoint TX NAKed */
#define USB_EP_TX_VALID                       (0x00000030U)                    /*!< EndPoint TX VALID */
#define USB_EPTX_DTOG1                        (0x00000010U)                    /*!< EndPoint TX Data TOGgle bit1 */
#define USB_EPTX_DTOG2                        (0x00000020U)                    /*!< EndPoint TX Data TOGgle bit2 */
#define  USB_EPTX_DTOGMASK  (USB_EPTX_STAT|USB_EPREG_MASK)
                                                                               /*!< STAT_RX[1:0] STATus for RX transfer */
#define USB_EP_RX_DIS                         (0x00000000U)                    /*!< EndPoint RX DISabled */
#define USB_EP_RX_STALL                       (0x00001000U)                    /*!< EndPoint RX STALLed */
#define USB_EP_RX_NAK                         (0x00002000U)                    /*!< EndPoint RX NAKed */
#define USB_EP_RX_VALID                       (0x00003000U)                    /*!< EndPoint RX VALID */
#define USB_EPRX_DTOG1                        (0x00001000U)                    /*!< EndPoint RX Data TOGgle bit1 */
#define USB_EPRX_DTOG2                        (0x00002000U)                    /*!< EndPoint RX Data TOGgle bit1 */
#define  USB_EPRX_DTOGMASK  (USB_EPRX_STAT|USB_EPREG_MASK)

/*******************  Bit definition for USB_EP0R register  *******************/
#define USB_EP0R_EA_Pos                       (0U)                             
#define USB_EP0R_EA_Msk                       (0xFU << USB_EP0R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP0R_EA                           USB_EP0R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP0R_STAT_TX_Pos                  (4U)                             
#define USB_EP0R_STAT_TX_Msk                  (0x3U << USB_EP0R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP0R_STAT_TX                      USB_EP0R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP0R_STAT_TX_0                    (0x1U << USB_EP0R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP0R_STAT_TX_1                    (0x2U << USB_EP0R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP0R_DTOG_TX_Pos                  (6U)                             
#define USB_EP0R_DTOG_TX_Msk                  (0x1U << USB_EP0R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP0R_DTOG_TX                      USB_EP0R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP0R_CTR_TX_Pos                   (7U)                             
#define USB_EP0R_CTR_TX_Msk                   (0x1U << USB_EP0R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP0R_CTR_TX                       USB_EP0R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP0R_EP_KIND_Pos                  (8U)                             
#define USB_EP0R_EP_KIND_Msk                  (0x1U << USB_EP0R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP0R_EP_KIND                      USB_EP0R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP0R_EP_TYPE_Pos                  (9U)                             
#define USB_EP0R_EP_TYPE_Msk                  (0x3U << USB_EP0R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP0R_EP_TYPE                      USB_EP0R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP0R_EP_TYPE_0                    (0x1U << USB_EP0R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP0R_EP_TYPE_1                    (0x2U << USB_EP0R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP0R_SETUP_Pos                    (11U)                            
#define USB_EP0R_SETUP_Msk                    (0x1U << USB_EP0R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP0R_SETUP                        USB_EP0R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP0R_STAT_RX_Pos                  (12U)                            
#define USB_EP0R_STAT_RX_Msk                  (0x3U << USB_EP0R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP0R_STAT_RX                      USB_EP0R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP0R_STAT_RX_0                    (0x1U << USB_EP0R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP0R_STAT_RX_1                    (0x2U << USB_EP0R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP0R_DTOG_RX_Pos                  (14U)                            
#define USB_EP0R_DTOG_RX_Msk                  (0x1U << USB_EP0R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP0R_DTOG_RX                      USB_EP0R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP0R_CTR_RX_Pos                   (15U)                            
#define USB_EP0R_CTR_RX_Msk                   (0x1U << USB_EP0R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP0R_CTR_RX                       USB_EP0R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP1R register  *******************/
#define USB_EP1R_EA_Pos                       (0U)                             
#define USB_EP1R_EA_Msk                       (0xFU << USB_EP1R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP1R_EA                           USB_EP1R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP1R_STAT_TX_Pos                  (4U)                             
#define USB_EP1R_STAT_TX_Msk                  (0x3U << USB_EP1R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP1R_STAT_TX                      USB_EP1R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP1R_STAT_TX_0                    (0x1U << USB_EP1R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP1R_STAT_TX_1                    (0x2U << USB_EP1R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP1R_DTOG_TX_Pos                  (6U)                             
#define USB_EP1R_DTOG_TX_Msk                  (0x1U << USB_EP1R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP1R_DTOG_TX                      USB_EP1R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP1R_CTR_TX_Pos                   (7U)                             
#define USB_EP1R_CTR_TX_Msk                   (0x1U << USB_EP1R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP1R_CTR_TX                       USB_EP1R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP1R_EP_KIND_Pos                  (8U)                             
#define USB_EP1R_EP_KIND_Msk                  (0x1U << USB_EP1R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP1R_EP_KIND                      USB_EP1R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP1R_EP_TYPE_Pos                  (9U)                             
#define USB_EP1R_EP_TYPE_Msk                  (0x3U << USB_EP1R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP1R_EP_TYPE                      USB_EP1R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP1R_EP_TYPE_0                    (0x1U << USB_EP1R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP1R_EP_TYPE_1                    (0x2U << USB_EP1R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP1R_SETUP_Pos                    (11U)                            
#define USB_EP1R_SETUP_Msk                    (0x1U << USB_EP1R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP1R_SETUP                        USB_EP1R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP1R_STAT_RX_Pos                  (12U)                            
#define USB_EP1R_STAT_RX_Msk                  (0x3U << USB_EP1R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP1R_STAT_RX                      USB_EP1R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP1R_STAT_RX_0                    (0x1U << USB_EP1R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP1R_STAT_RX_1                    (0x2U << USB_EP1R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP1R_DTOG_RX_Pos                  (14U)                            
#define USB_EP1R_DTOG_RX_Msk                  (0x1U << USB_EP1R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP1R_DTOG_RX                      USB_EP1R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP1R_CTR_RX_Pos                   (15U)                            
#define USB_EP1R_CTR_RX_Msk                   (0x1U << USB_EP1R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP1R_CTR_RX                       USB_EP1R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP2R register  *******************/
#define USB_EP2R_EA_Pos                       (0U)                             
#define USB_EP2R_EA_Msk                       (0xFU << USB_EP2R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP2R_EA                           USB_EP2R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP2R_STAT_TX_Pos                  (4U)                             
#define USB_EP2R_STAT_TX_Msk                  (0x3U << USB_EP2R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP2R_STAT_TX                      USB_EP2R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP2R_STAT_TX_0                    (0x1U << USB_EP2R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP2R_STAT_TX_1                    (0x2U << USB_EP2R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP2R_DTOG_TX_Pos                  (6U)                             
#define USB_EP2R_DTOG_TX_Msk                  (0x1U << USB_EP2R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP2R_DTOG_TX                      USB_EP2R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP2R_CTR_TX_Pos                   (7U)                             
#define USB_EP2R_CTR_TX_Msk                   (0x1U << USB_EP2R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP2R_CTR_TX                       USB_EP2R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP2R_EP_KIND_Pos                  (8U)                             
#define USB_EP2R_EP_KIND_Msk                  (0x1U << USB_EP2R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP2R_EP_KIND                      USB_EP2R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP2R_EP_TYPE_Pos                  (9U)                             
#define USB_EP2R_EP_TYPE_Msk                  (0x3U << USB_EP2R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP2R_EP_TYPE                      USB_EP2R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP2R_EP_TYPE_0                    (0x1U << USB_EP2R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP2R_EP_TYPE_1                    (0x2U << USB_EP2R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP2R_SETUP_Pos                    (11U)                            
#define USB_EP2R_SETUP_Msk                    (0x1U << USB_EP2R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP2R_SETUP                        USB_EP2R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP2R_STAT_RX_Pos                  (12U)                            
#define USB_EP2R_STAT_RX_Msk                  (0x3U << USB_EP2R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP2R_STAT_RX                      USB_EP2R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP2R_STAT_RX_0                    (0x1U << USB_EP2R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP2R_STAT_RX_1                    (0x2U << USB_EP2R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP2R_DTOG_RX_Pos                  (14U)                            
#define USB_EP2R_DTOG_RX_Msk                  (0x1U << USB_EP2R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP2R_DTOG_RX                      USB_EP2R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP2R_CTR_RX_Pos                   (15U)                            
#define USB_EP2R_CTR_RX_Msk                   (0x1U << USB_EP2R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP2R_CTR_RX                       USB_EP2R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP3R register  *******************/
#define USB_EP3R_EA_Pos                       (0U)                             
#define USB_EP3R_EA_Msk                       (0xFU << USB_EP3R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP3R_EA                           USB_EP3R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP3R_STAT_TX_Pos                  (4U)                             
#define USB_EP3R_STAT_TX_Msk                  (0x3U << USB_EP3R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP3R_STAT_TX                      USB_EP3R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP3R_STAT_TX_0                    (0x1U << USB_EP3R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP3R_STAT_TX_1                    (0x2U << USB_EP3R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP3R_DTOG_TX_Pos                  (6U)                             
#define USB_EP3R_DTOG_TX_Msk                  (0x1U << USB_EP3R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP3R_DTOG_TX                      USB_EP3R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP3R_CTR_TX_Pos                   (7U)                             
#define USB_EP3R_CTR_TX_Msk                   (0x1U << USB_EP3R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP3R_CTR_TX                       USB_EP3R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP3R_EP_KIND_Pos                  (8U)                             
#define USB_EP3R_EP_KIND_Msk                  (0x1U << USB_EP3R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP3R_EP_KIND                      USB_EP3R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP3R_EP_TYPE_Pos                  (9U)                             
#define USB_EP3R_EP_TYPE_Msk                  (0x3U << USB_EP3R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP3R_EP_TYPE                      USB_EP3R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP3R_EP_TYPE_0                    (0x1U << USB_EP3R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP3R_EP_TYPE_1                    (0x2U << USB_EP3R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP3R_SETUP_Pos                    (11U)                            
#define USB_EP3R_SETUP_Msk                    (0x1U << USB_EP3R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP3R_SETUP                        USB_EP3R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP3R_STAT_RX_Pos                  (12U)                            
#define USB_EP3R_STAT_RX_Msk                  (0x3U << USB_EP3R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP3R_STAT_RX                      USB_EP3R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP3R_STAT_RX_0                    (0x1U << USB_EP3R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP3R_STAT_RX_1                    (0x2U << USB_EP3R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP3R_DTOG_RX_Pos                  (14U)                            
#define USB_EP3R_DTOG_RX_Msk                  (0x1U << USB_EP3R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP3R_DTOG_RX                      USB_EP3R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP3R_CTR_RX_Pos                   (15U)                            
#define USB_EP3R_CTR_RX_Msk                   (0x1U << USB_EP3R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP3R_CTR_RX                       USB_EP3R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP4R register  *******************/
#define USB_EP4R_EA_Pos                       (0U)                             
#define USB_EP4R_EA_Msk                       (0xFU << USB_EP4R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP4R_EA                           USB_EP4R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP4R_STAT_TX_Pos                  (4U)                             
#define USB_EP4R_STAT_TX_Msk                  (0x3U << USB_EP4R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP4R_STAT_TX                      USB_EP4R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP4R_STAT_TX_0                    (0x1U << USB_EP4R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP4R_STAT_TX_1                    (0x2U << USB_EP4R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP4R_DTOG_TX_Pos                  (6U)                             
#define USB_EP4R_DTOG_TX_Msk                  (0x1U << USB_EP4R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP4R_DTOG_TX                      USB_EP4R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP4R_CTR_TX_Pos                   (7U)                             
#define USB_EP4R_CTR_TX_Msk                   (0x1U << USB_EP4R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP4R_CTR_TX                       USB_EP4R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP4R_EP_KIND_Pos                  (8U)                             
#define USB_EP4R_EP_KIND_Msk                  (0x1U << USB_EP4R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP4R_EP_KIND                      USB_EP4R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP4R_EP_TYPE_Pos                  (9U)                             
#define USB_EP4R_EP_TYPE_Msk                  (0x3U << USB_EP4R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP4R_EP_TYPE                      USB_EP4R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP4R_EP_TYPE_0                    (0x1U << USB_EP4R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP4R_EP_TYPE_1                    (0x2U << USB_EP4R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP4R_SETUP_Pos                    (11U)                            
#define USB_EP4R_SETUP_Msk                    (0x1U << USB_EP4R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP4R_SETUP                        USB_EP4R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP4R_STAT_RX_Pos                  (12U)                            
#define USB_EP4R_STAT_RX_Msk                  (0x3U << USB_EP4R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP4R_STAT_RX                      USB_EP4R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP4R_STAT_RX_0                    (0x1U << USB_EP4R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP4R_STAT_RX_1                    (0x2U << USB_EP4R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP4R_DTOG_RX_Pos                  (14U)                            
#define USB_EP4R_DTOG_RX_Msk                  (0x1U << USB_EP4R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP4R_DTOG_RX                      USB_EP4R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP4R_CTR_RX_Pos                   (15U)                            
#define USB_EP4R_CTR_RX_Msk                   (0x1U << USB_EP4R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP4R_CTR_RX                       USB_EP4R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP5R register  *******************/
#define USB_EP5R_EA_Pos                       (0U)                             
#define USB_EP5R_EA_Msk                       (0xFU << USB_EP5R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP5R_EA                           USB_EP5R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP5R_STAT_TX_Pos                  (4U)                             
#define USB_EP5R_STAT_TX_Msk                  (0x3U << USB_EP5R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP5R_STAT_TX                      USB_EP5R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP5R_STAT_TX_0                    (0x1U << USB_EP5R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP5R_STAT_TX_1                    (0x2U << USB_EP5R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP5R_DTOG_TX_Pos                  (6U)                             
#define USB_EP5R_DTOG_TX_Msk                  (0x1U << USB_EP5R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP5R_DTOG_TX                      USB_EP5R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP5R_CTR_TX_Pos                   (7U)                             
#define USB_EP5R_CTR_TX_Msk                   (0x1U << USB_EP5R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP5R_CTR_TX                       USB_EP5R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP5R_EP_KIND_Pos                  (8U)                             
#define USB_EP5R_EP_KIND_Msk                  (0x1U << USB_EP5R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP5R_EP_KIND                      USB_EP5R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP5R_EP_TYPE_Pos                  (9U)                             
#define USB_EP5R_EP_TYPE_Msk                  (0x3U << USB_EP5R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP5R_EP_TYPE                      USB_EP5R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP5R_EP_TYPE_0                    (0x1U << USB_EP5R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP5R_EP_TYPE_1                    (0x2U << USB_EP5R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP5R_SETUP_Pos                    (11U)                            
#define USB_EP5R_SETUP_Msk                    (0x1U << USB_EP5R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP5R_SETUP                        USB_EP5R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP5R_STAT_RX_Pos                  (12U)                            
#define USB_EP5R_STAT_RX_Msk                  (0x3U << USB_EP5R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP5R_STAT_RX                      USB_EP5R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP5R_STAT_RX_0                    (0x1U << USB_EP5R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP5R_STAT_RX_1                    (0x2U << USB_EP5R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP5R_DTOG_RX_Pos                  (14U)                            
#define USB_EP5R_DTOG_RX_Msk                  (0x1U << USB_EP5R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP5R_DTOG_RX                      USB_EP5R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP5R_CTR_RX_Pos                   (15U)                            
#define USB_EP5R_CTR_RX_Msk                   (0x1U << USB_EP5R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP5R_CTR_RX                       USB_EP5R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP6R register  *******************/
#define USB_EP6R_EA_Pos                       (0U)                             
#define USB_EP6R_EA_Msk                       (0xFU << USB_EP6R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP6R_EA                           USB_EP6R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP6R_STAT_TX_Pos                  (4U)                             
#define USB_EP6R_STAT_TX_Msk                  (0x3U << USB_EP6R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP6R_STAT_TX                      USB_EP6R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP6R_STAT_TX_0                    (0x1U << USB_EP6R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP6R_STAT_TX_1                    (0x2U << USB_EP6R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP6R_DTOG_TX_Pos                  (6U)                             
#define USB_EP6R_DTOG_TX_Msk                  (0x1U << USB_EP6R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP6R_DTOG_TX                      USB_EP6R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP6R_CTR_TX_Pos                   (7U)                             
#define USB_EP6R_CTR_TX_Msk                   (0x1U << USB_EP6R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP6R_CTR_TX                       USB_EP6R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP6R_EP_KIND_Pos                  (8U)                             
#define USB_EP6R_EP_KIND_Msk                  (0x1U << USB_EP6R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP6R_EP_KIND                      USB_EP6R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP6R_EP_TYPE_Pos                  (9U)                             
#define USB_EP6R_EP_TYPE_Msk                  (0x3U << USB_EP6R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP6R_EP_TYPE                      USB_EP6R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP6R_EP_TYPE_0                    (0x1U << USB_EP6R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP6R_EP_TYPE_1                    (0x2U << USB_EP6R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP6R_SETUP_Pos                    (11U)                            
#define USB_EP6R_SETUP_Msk                    (0x1U << USB_EP6R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP6R_SETUP                        USB_EP6R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP6R_STAT_RX_Pos                  (12U)                            
#define USB_EP6R_STAT_RX_Msk                  (0x3U << USB_EP6R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP6R_STAT_RX                      USB_EP6R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP6R_STAT_RX_0                    (0x1U << USB_EP6R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP6R_STAT_RX_1                    (0x2U << USB_EP6R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP6R_DTOG_RX_Pos                  (14U)                            
#define USB_EP6R_DTOG_RX_Msk                  (0x1U << USB_EP6R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP6R_DTOG_RX                      USB_EP6R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP6R_CTR_RX_Pos                   (15U)                            
#define USB_EP6R_CTR_RX_Msk                   (0x1U << USB_EP6R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP6R_CTR_RX                       USB_EP6R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*******************  Bit definition for USB_EP7R register  *******************/
#define USB_EP7R_EA_Pos                       (0U)                             
#define USB_EP7R_EA_Msk                       (0xFU << USB_EP7R_EA_Pos)        /*!< 0x0000000F */
#define USB_EP7R_EA                           USB_EP7R_EA_Msk                  /*!<Endpoint Address */

#define USB_EP7R_STAT_TX_Pos                  (4U)                             
#define USB_EP7R_STAT_TX_Msk                  (0x3U << USB_EP7R_STAT_TX_Pos)   /*!< 0x00000030 */
#define USB_EP7R_STAT_TX                      USB_EP7R_STAT_TX_Msk             /*!<STAT_TX[1:0] bits (Status bits, for transmission transfers) */
#define USB_EP7R_STAT_TX_0                    (0x1U << USB_EP7R_STAT_TX_Pos)   /*!< 0x00000010 */
#define USB_EP7R_STAT_TX_1                    (0x2U << USB_EP7R_STAT_TX_Pos)   /*!< 0x00000020 */

#define USB_EP7R_DTOG_TX_Pos                  (6U)                             
#define USB_EP7R_DTOG_TX_Msk                  (0x1U << USB_EP7R_DTOG_TX_Pos)   /*!< 0x00000040 */
#define USB_EP7R_DTOG_TX                      USB_EP7R_DTOG_TX_Msk             /*!<Data Toggle, for transmission transfers */
#define USB_EP7R_CTR_TX_Pos                   (7U)                             
#define USB_EP7R_CTR_TX_Msk                   (0x1U << USB_EP7R_CTR_TX_Pos)    /*!< 0x00000080 */
#define USB_EP7R_CTR_TX                       USB_EP7R_CTR_TX_Msk              /*!<Correct Transfer for transmission */
#define USB_EP7R_EP_KIND_Pos                  (8U)                             
#define USB_EP7R_EP_KIND_Msk                  (0x1U << USB_EP7R_EP_KIND_Pos)   /*!< 0x00000100 */
#define USB_EP7R_EP_KIND                      USB_EP7R_EP_KIND_Msk             /*!<Endpoint Kind */

#define USB_EP7R_EP_TYPE_Pos                  (9U)                             
#define USB_EP7R_EP_TYPE_Msk                  (0x3U << USB_EP7R_EP_TYPE_Pos)   /*!< 0x00000600 */
#define USB_EP7R_EP_TYPE                      USB_EP7R_EP_TYPE_Msk             /*!<EP_TYPE[1:0] bits (Endpoint type) */
#define USB_EP7R_EP_TYPE_0                    (0x1U << USB_EP7R_EP_TYPE_Pos)   /*!< 0x00000200 */
#define USB_EP7R_EP_TYPE_1                    (0x2U << USB_EP7R_EP_TYPE_Pos)   /*!< 0x00000400 */

#define USB_EP7R_SETUP_Pos                    (11U)                            
#define USB_EP7R_SETUP_Msk                    (0x1U << USB_EP7R_SETUP_Pos)     /*!< 0x00000800 */
#define USB_EP7R_SETUP                        USB_EP7R_SETUP_Msk               /*!<Setup transaction completed */

#define USB_EP7R_STAT_RX_Pos                  (12U)                            
#define USB_EP7R_STAT_RX_Msk                  (0x3U << USB_EP7R_STAT_RX_Pos)   /*!< 0x00003000 */
#define USB_EP7R_STAT_RX                      USB_EP7R_STAT_RX_Msk             /*!<STAT_RX[1:0] bits (Status bits, for reception transfers) */
#define USB_EP7R_STAT_RX_0                    (0x1U << USB_EP7R_STAT_RX_Pos)   /*!< 0x00001000 */
#define USB_EP7R_STAT_RX_1                    (0x2U << USB_EP7R_STAT_RX_Pos)   /*!< 0x00002000 */

#define USB_EP7R_DTOG_RX_Pos                  (14U)                            
#define USB_EP7R_DTOG_RX_Msk                  (0x1U << USB_EP7R_DTOG_RX_Pos)   /*!< 0x00004000 */
#define USB_EP7R_DTOG_RX                      USB_EP7R_DTOG_RX_Msk             /*!<Data Toggle, for reception transfers */
#define USB_EP7R_CTR_RX_Pos                   (15U)                            
#define USB_EP7R_CTR_RX_Msk                   (0x1U << USB_EP7R_CTR_RX_Pos)    /*!< 0x00008000 */
#define USB_EP7R_CTR_RX                       USB_EP7R_CTR_RX_Msk              /*!<Correct Transfer for reception */

/*!<Common registers */

#define  USB_CNTR                             (USB_BASE + 0x00000040U)          /*!< Control register */
#define  USB_ISTR                             (USB_BASE + 0x00000044U)          /*!< Interrupt status register */
#define  USB_FNR                              (USB_BASE + 0x00000048U)          /*!< Frame number register */
#define  USB_DADDR                            (USB_BASE + 0x0000004CU)          /*!< Device address register */
#define  USB_BTABLE                           (USB_BASE + 0x00000050U)          /*!< Buffer Table address register */



/*******************  Bit definition for USB_CNTR register  *******************/
#define USB_CNTR_FRES_Pos                     (0U)                             
#define USB_CNTR_FRES_Msk                     (0x1U << USB_CNTR_FRES_Pos)      /*!< 0x00000001 */
#define USB_CNTR_FRES                         USB_CNTR_FRES_Msk                /*!<Force USB Reset */
#define USB_CNTR_PDWN_Pos                     (1U)                             
#define USB_CNTR_PDWN_Msk                     (0x1U << USB_CNTR_PDWN_Pos)      /*!< 0x00000002 */
#define USB_CNTR_PDWN                         USB_CNTR_PDWN_Msk                /*!<Power down */
#define USB_CNTR_LPMODE_Pos                   (2U)                             
#define USB_CNTR_LPMODE_Msk                   (0x1U << USB_CNTR_LPMODE_Pos)    /*!< 0x00000004 */
#define USB_CNTR_LPMODE                       USB_CNTR_LPMODE_Msk              /*!<Low-power mode */
#define USB_CNTR_FSUSP_Pos                    (3U)                             
#define USB_CNTR_FSUSP_Msk                    (0x1U << USB_CNTR_FSUSP_Pos)     /*!< 0x00000008 */
#define USB_CNTR_FSUSP                        USB_CNTR_FSUSP_Msk               /*!<Force suspend */
#define USB_CNTR_RESUME_Pos                   (4U)                             
#define USB_CNTR_RESUME_Msk                   (0x1U << USB_CNTR_RESUME_Pos)    /*!< 0x00000010 */
#define USB_CNTR_RESUME                       USB_CNTR_RESUME_Msk              /*!<Resume request */
#define USB_CNTR_ESOFM_Pos                    (8U)                             
#define USB_CNTR_ESOFM_Msk                    (0x1U << USB_CNTR_ESOFM_Pos)     /*!< 0x00000100 */
#define USB_CNTR_ESOFM                        USB_CNTR_ESOFM_Msk               /*!<Expected Start Of Frame Interrupt Mask */
#define USB_CNTR_SOFM_Pos                     (9U)                             
#define USB_CNTR_SOFM_Msk                     (0x1U << USB_CNTR_SOFM_Pos)      /*!< 0x00000200 */
#define USB_CNTR_SOFM                         USB_CNTR_SOFM_Msk                /*!<Start Of Frame Interrupt Mask */
#define USB_CNTR_RESETM_Pos                   (10U)                            
#define USB_CNTR_RESETM_Msk                   (0x1U << USB_CNTR_RESETM_Pos)    /*!< 0x00000400 */
#define USB_CNTR_RESETM                       USB_CNTR_RESETM_Msk              /*!<RESET Interrupt Mask */
#define USB_CNTR_SUSPM_Pos                    (11U)                            
#define USB_CNTR_SUSPM_Msk                    (0x1U << USB_CNTR_SUSPM_Pos)     /*!< 0x00000800 */
#define USB_CNTR_SUSPM                        USB_CNTR_SUSPM_Msk               /*!<Suspend mode Interrupt Mask */
#define USB_CNTR_WKUPM_Pos                    (12U)                            
#define USB_CNTR_WKUPM_Msk                    (0x1U << USB_CNTR_WKUPM_Pos)     /*!< 0x00001000 */
#define USB_CNTR_WKUPM                        USB_CNTR_WKUPM_Msk               /*!<Wakeup Interrupt Mask */
#define USB_CNTR_ERRM_Pos                     (13U)                            
#define USB_CNTR_ERRM_Msk                     (0x1U << USB_CNTR_ERRM_Pos)      /*!< 0x00002000 */
#define USB_CNTR_ERRM                         USB_CNTR_ERRM_Msk                /*!<Error Interrupt Mask */
#define USB_CNTR_PMAOVRM_Pos                  (14U)                            
#define USB_CNTR_PMAOVRM_Msk                  (0x1U << USB_CNTR_PMAOVRM_Pos)   /*!< 0x00004000 */
#define USB_CNTR_PMAOVRM                      USB_CNTR_PMAOVRM_Msk             /*!<Packet Memory Area Over / Underrun Interrupt Mask */
#define USB_CNTR_CTRM_Pos                     (15U)                            
#define USB_CNTR_CTRM_Msk                     (0x1U << USB_CNTR_CTRM_Pos)      /*!< 0x00008000 */
#define USB_CNTR_CTRM                         USB_CNTR_CTRM_Msk                /*!<Correct Transfer Interrupt Mask */

/*******************  Bit definition for USB_ISTR register  *******************/
#define USB_ISTR_EP_ID_Pos                    (0U)                             
#define USB_ISTR_EP_ID_Msk                    (0xFU << USB_ISTR_EP_ID_Pos)     /*!< 0x0000000F */
#define USB_ISTR_EP_ID                        USB_ISTR_EP_ID_Msk               /*!<Endpoint Identifier */
#define USB_ISTR_DIR_Pos                      (4U)                             
#define USB_ISTR_DIR_Msk                      (0x1U << USB_ISTR_DIR_Pos)       /*!< 0x00000010 */
#define USB_ISTR_DIR                          USB_ISTR_DIR_Msk                 /*!<Direction of transaction */
#define USB_ISTR_ESOF_Pos                     (8U)                             
#define USB_ISTR_ESOF_Msk                     (0x1U << USB_ISTR_ESOF_Pos)      /*!< 0x00000100 */
#define USB_ISTR_ESOF                         USB_ISTR_ESOF_Msk                /*!<Expected Start Of Frame */
#define USB_ISTR_SOF_Pos                      (9U)                             
#define USB_ISTR_SOF_Msk                      (0x1U << USB_ISTR_SOF_Pos)       /*!< 0x00000200 */
#define USB_ISTR_SOF                          USB_ISTR_SOF_Msk                 /*!<Start Of Frame */
#define USB_ISTR_RESET_Pos                    (10U)                            
#define USB_ISTR_RESET_Msk                    (0x1U << USB_ISTR_RESET_Pos)     /*!< 0x00000400 */
#define USB_ISTR_RESET                        USB_ISTR_RESET_Msk               /*!<USB RESET request */
#define USB_ISTR_SUSP_Pos                     (11U)                            
#define USB_ISTR_SUSP_Msk                     (0x1U << USB_ISTR_SUSP_Pos)      /*!< 0x00000800 */
#define USB_ISTR_SUSP                         USB_ISTR_SUSP_Msk                /*!<Suspend mode request */
#define USB_ISTR_WKUP_Pos                     (12U)                            
#define USB_ISTR_WKUP_Msk                     (0x1U << USB_ISTR_WKUP_Pos)      /*!< 0x00001000 */
#define USB_ISTR_WKUP                         USB_ISTR_WKUP_Msk                /*!<Wake up */
#define USB_ISTR_ERR_Pos                      (13U)                            
#define USB_ISTR_ERR_Msk                      (0x1U << USB_ISTR_ERR_Pos)       /*!< 0x00002000 */
#define USB_ISTR_ERR                          USB_ISTR_ERR_Msk                 /*!<Error */
#define USB_ISTR_PMAOVR_Pos                   (14U)                            
#define USB_ISTR_PMAOVR_Msk                   (0x1U << USB_ISTR_PMAOVR_Pos)    /*!< 0x00004000 */
#define USB_ISTR_PMAOVR                       USB_ISTR_PMAOVR_Msk              /*!<Packet Memory Area Over / Underrun */
#define USB_ISTR_CTR_Pos                      (15U)                            
#define USB_ISTR_CTR_Msk                      (0x1U << USB_ISTR_CTR_Pos)       /*!< 0x00008000 */
#define USB_ISTR_CTR                          USB_ISTR_CTR_Msk                 /*!<Correct Transfer */

#define  USB_CLR_CTR                          (~USB_ISTR_CTR)                  /*!< clear Correct TRansfer bit */
#define  USB_CLR_PMAOVRM                      (~USB_ISTR_PMAOVR)               /*!< clear DMA OVeR/underrun bit*/
#define  USB_CLR_ERR                          (~USB_ISTR_ERR)                  /*!< clear ERRor bit */
#define  USB_CLR_WKUP                         (~USB_ISTR_WKUP)                 /*!< clear WaKe UP bit */
#define  USB_CLR_SUSP                         (~USB_ISTR_SUSP)                 /*!< clear SUSPend bit */
#define  USB_CLR_RESET                        (~USB_ISTR_RESET)                /*!< clear RESET bit */
#define  USB_CLR_SOF                          (~USB_ISTR_SOF)                  /*!< clear Start Of Frame bit */
#define  USB_CLR_ESOF                         (~USB_ISTR_ESOF)                 /*!< clear Expected Start Of Frame bit */


/*******************  Bit definition for USB_FNR register  ********************/
#define USB_FNR_FN_Pos                        (0U)                             
#define USB_FNR_FN_Msk                        (0x7FFU << USB_FNR_FN_Pos)       /*!< 0x000007FF */
#define USB_FNR_FN                            USB_FNR_FN_Msk                   /*!<Frame Number */
#define USB_FNR_LSOF_Pos                      (11U)                            
#define USB_FNR_LSOF_Msk                      (0x3U << USB_FNR_LSOF_Pos)       /*!< 0x00001800 */
#define USB_FNR_LSOF                          USB_FNR_LSOF_Msk                 /*!<Lost SOF */
#define USB_FNR_LCK_Pos                       (13U)                            
#define USB_FNR_LCK_Msk                       (0x1U << USB_FNR_LCK_Pos)        /*!< 0x00002000 */
#define USB_FNR_LCK                           USB_FNR_LCK_Msk                  /*!<Locked */
#define USB_FNR_RXDM_Pos                      (14U)                            
#define USB_FNR_RXDM_Msk                      (0x1U << USB_FNR_RXDM_Pos)       /*!< 0x00004000 */
#define USB_FNR_RXDM                          USB_FNR_RXDM_Msk                 /*!<Receive Data - Line Status */
#define USB_FNR_RXDP_Pos                      (15U)                            
#define USB_FNR_RXDP_Msk                      (0x1U << USB_FNR_RXDP_Pos)       /*!< 0x00008000 */
#define USB_FNR_RXDP                          USB_FNR_RXDP_Msk                 /*!<Receive Data + Line Status */

/******************  Bit definition for USB_DADDR register  *******************/
#define USB_DADDR_ADD_Pos                     (0U)                             
#define USB_DADDR_ADD_Msk                     (0x7FU << USB_DADDR_ADD_Pos)     /*!< 0x0000007F */
#define USB_DADDR_ADD                         USB_DADDR_ADD_Msk                /*!<ADD[6:0] bits (Device Address) */
#define USB_DADDR_ADD0_Pos                    (0U)                             
#define USB_DADDR_ADD0_Msk                    (0x1U << USB_DADDR_ADD0_Pos)     /*!< 0x00000001 */
#define USB_DADDR_ADD0                        USB_DADDR_ADD0_Msk               /*!<Bit 0 */
#define USB_DADDR_ADD1_Pos                    (1U)                             
#define USB_DADDR_ADD1_Msk                    (0x1U << USB_DADDR_ADD1_Pos)     /*!< 0x00000002 */
#define USB_DADDR_ADD1                        USB_DADDR_ADD1_Msk               /*!<Bit 1 */
#define USB_DADDR_ADD2_Pos                    (2U)                             
#define USB_DADDR_ADD2_Msk                    (0x1U << USB_DADDR_ADD2_Pos)     /*!< 0x00000004 */
#define USB_DADDR_ADD2                        USB_DADDR_ADD2_Msk               /*!<Bit 2 */
#define USB_DADDR_ADD3_Pos                    (3U)                             
#define USB_DADDR_ADD3_Msk                    (0x1U << USB_DADDR_ADD3_Pos)     /*!< 0x00000008 */
#define USB_DADDR_ADD3                        USB_DADDR_ADD3_Msk               /*!<Bit 3 */
#define USB_DADDR_ADD4_Pos                    (4U)                             
#define USB_DADDR_ADD4_Msk                    (0x1U << USB_DADDR_ADD4_Pos)     /*!< 0x00000010 */
#define USB_DADDR_ADD4                        USB_DADDR_ADD4_Msk               /*!<Bit 4 */
#define USB_DADDR_ADD5_Pos                    (5U)                             
#define USB_DADDR_ADD5_Msk                    (0x1U << USB_DADDR_ADD5_Pos)     /*!< 0x00000020 */
#define USB_DADDR_ADD5                        USB_DADDR_ADD5_Msk               /*!<Bit 5 */
#define USB_DADDR_ADD6_Pos                    (6U)                             
#define USB_DADDR_ADD6_Msk                    (0x1U << USB_DADDR_ADD6_Pos)     /*!< 0x00000040 */
#define USB_DADDR_ADD6                        USB_DADDR_ADD6_Msk               /*!<Bit 6 */

#define USB_DADDR_EF_Pos                      (7U)                             
#define USB_DADDR_EF_Msk                      (0x1U << USB_DADDR_EF_Pos)       /*!< 0x00000080 */
#define USB_DADDR_EF                          USB_DADDR_EF_Msk                 /*!<Enable Function */

/******************  Bit definition for USB_BTABLE register  ******************/    
#define USB_BTABLE_BTABLE_Pos                 (3U)                             
#define USB_BTABLE_BTABLE_Msk                 (0x1FFFU << USB_BTABLE_BTABLE_Pos) /*!< 0x0000FFF8 */
#define USB_BTABLE_BTABLE                     USB_BTABLE_BTABLE_Msk            /*!<Buffer Table */

/*!< Buffer descriptor table */
/*****************  Bit definition for USB_ADDR0_TX register  *****************/
#define USB_ADDR0_TX_ADDR0_TX_Pos             (1U)                             
#define USB_ADDR0_TX_ADDR0_TX_Msk             (0x7FFFU << USB_ADDR0_TX_ADDR0_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR0_TX_ADDR0_TX                 USB_ADDR0_TX_ADDR0_TX_Msk        /*!< Transmission Buffer Address 0 */

/*****************  Bit definition for USB_ADDR1_TX register  *****************/
#define USB_ADDR1_TX_ADDR1_TX_Pos             (1U)                             
#define USB_ADDR1_TX_ADDR1_TX_Msk             (0x7FFFU << USB_ADDR1_TX_ADDR1_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR1_TX_ADDR1_TX                 USB_ADDR1_TX_ADDR1_TX_Msk        /*!< Transmission Buffer Address 1 */

/*****************  Bit definition for USB_ADDR2_TX register  *****************/
#define USB_ADDR2_TX_ADDR2_TX_Pos             (1U)                             
#define USB_ADDR2_TX_ADDR2_TX_Msk             (0x7FFFU << USB_ADDR2_TX_ADDR2_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR2_TX_ADDR2_TX                 USB_ADDR2_TX_ADDR2_TX_Msk        /*!< Transmission Buffer Address 2 */

/*****************  Bit definition for USB_ADDR3_TX register  *****************/
#define USB_ADDR3_TX_ADDR3_TX_Pos             (1U)                             
#define USB_ADDR3_TX_ADDR3_TX_Msk             (0x7FFFU << USB_ADDR3_TX_ADDR3_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR3_TX_ADDR3_TX                 USB_ADDR3_TX_ADDR3_TX_Msk        /*!< Transmission Buffer Address 3 */

/*****************  Bit definition for USB_ADDR4_TX register  *****************/
#define USB_ADDR4_TX_ADDR4_TX_Pos             (1U)                             
#define USB_ADDR4_TX_ADDR4_TX_Msk             (0x7FFFU << USB_ADDR4_TX_ADDR4_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR4_TX_ADDR4_TX                 USB_ADDR4_TX_ADDR4_TX_Msk        /*!< Transmission Buffer Address 4 */

/*****************  Bit definition for USB_ADDR5_TX register  *****************/
#define USB_ADDR5_TX_ADDR5_TX_Pos             (1U)                             
#define USB_ADDR5_TX_ADDR5_TX_Msk             (0x7FFFU << USB_ADDR5_TX_ADDR5_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR5_TX_ADDR5_TX                 USB_ADDR5_TX_ADDR5_TX_Msk        /*!< Transmission Buffer Address 5 */

/*****************  Bit definition for USB_ADDR6_TX register  *****************/
#define USB_ADDR6_TX_ADDR6_TX_Pos             (1U)                             
#define USB_ADDR6_TX_ADDR6_TX_Msk             (0x7FFFU << USB_ADDR6_TX_ADDR6_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR6_TX_ADDR6_TX                 USB_ADDR6_TX_ADDR6_TX_Msk        /*!< Transmission Buffer Address 6 */

/*****************  Bit definition for USB_ADDR7_TX register  *****************/
#define USB_ADDR7_TX_ADDR7_TX_Pos             (1U)                             
#define USB_ADDR7_TX_ADDR7_TX_Msk             (0x7FFFU << USB_ADDR7_TX_ADDR7_TX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR7_TX_ADDR7_TX                 USB_ADDR7_TX_ADDR7_TX_Msk        /*!< Transmission Buffer Address 7 */

/*----------------------------------------------------------------------------*/

/*****************  Bit definition for USB_COUNT0_TX register  ****************/
#define USB_COUNT0_TX_COUNT0_TX_Pos           (0U)                             
#define USB_COUNT0_TX_COUNT0_TX_Msk           (0x3FFU << USB_COUNT0_TX_COUNT0_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT0_TX_COUNT0_TX               USB_COUNT0_TX_COUNT0_TX_Msk      /*!< Transmission Byte Count 0 */

/*****************  Bit definition for USB_COUNT1_TX register  ****************/
#define USB_COUNT1_TX_COUNT1_TX_Pos           (0U)                             
#define USB_COUNT1_TX_COUNT1_TX_Msk           (0x3FFU << USB_COUNT1_TX_COUNT1_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT1_TX_COUNT1_TX               USB_COUNT1_TX_COUNT1_TX_Msk      /*!< Transmission Byte Count 1 */

/*****************  Bit definition for USB_COUNT2_TX register  ****************/
#define USB_COUNT2_TX_COUNT2_TX_Pos           (0U)                             
#define USB_COUNT2_TX_COUNT2_TX_Msk           (0x3FFU << USB_COUNT2_TX_COUNT2_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT2_TX_COUNT2_TX               USB_COUNT2_TX_COUNT2_TX_Msk      /*!< Transmission Byte Count 2 */

/*****************  Bit definition for USB_COUNT3_TX register  ****************/
#define USB_COUNT3_TX_COUNT3_TX_Pos           (0U)                             
#define USB_COUNT3_TX_COUNT3_TX_Msk           (0x3FFU << USB_COUNT3_TX_COUNT3_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT3_TX_COUNT3_TX               USB_COUNT3_TX_COUNT3_TX_Msk      /*!< Transmission Byte Count 3 */

/*****************  Bit definition for USB_COUNT4_TX register  ****************/
#define USB_COUNT4_TX_COUNT4_TX_Pos           (0U)                             
#define USB_COUNT4_TX_COUNT4_TX_Msk           (0x3FFU << USB_COUNT4_TX_COUNT4_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT4_TX_COUNT4_TX               USB_COUNT4_TX_COUNT4_TX_Msk      /*!< Transmission Byte Count 4 */

/*****************  Bit definition for USB_COUNT5_TX register  ****************/
#define USB_COUNT5_TX_COUNT5_TX_Pos           (0U)                             
#define USB_COUNT5_TX_COUNT5_TX_Msk           (0x3FFU << USB_COUNT5_TX_COUNT5_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT5_TX_COUNT5_TX               USB_COUNT5_TX_COUNT5_TX_Msk      /*!< Transmission Byte Count 5 */

/*****************  Bit definition for USB_COUNT6_TX register  ****************/
#define USB_COUNT6_TX_COUNT6_TX_Pos           (0U)                             
#define USB_COUNT6_TX_COUNT6_TX_Msk           (0x3FFU << USB_COUNT6_TX_COUNT6_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT6_TX_COUNT6_TX               USB_COUNT6_TX_COUNT6_TX_Msk      /*!< Transmission Byte Count 6 */

/*****************  Bit definition for USB_COUNT7_TX register  ****************/
#define USB_COUNT7_TX_COUNT7_TX_Pos           (0U)                             
#define USB_COUNT7_TX_COUNT7_TX_Msk           (0x3FFU << USB_COUNT7_TX_COUNT7_TX_Pos) /*!< 0x000003FF */
#define USB_COUNT7_TX_COUNT7_TX               USB_COUNT7_TX_COUNT7_TX_Msk      /*!< Transmission Byte Count 7 */

/*----------------------------------------------------------------------------*/

/****************  Bit definition for USB_COUNT0_TX_0 register  ***************/
#define USB_COUNT0_TX_0_COUNT0_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 0 (low) */

/****************  Bit definition for USB_COUNT0_TX_1 register  ***************/
#define USB_COUNT0_TX_1_COUNT0_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 0 (high) */

/****************  Bit definition for USB_COUNT1_TX_0 register  ***************/
#define USB_COUNT1_TX_0_COUNT1_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 1 (low) */

/****************  Bit definition for USB_COUNT1_TX_1 register  ***************/
#define USB_COUNT1_TX_1_COUNT1_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 1 (high) */

/****************  Bit definition for USB_COUNT2_TX_0 register  ***************/
#define USB_COUNT2_TX_0_COUNT2_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 2 (low) */

/****************  Bit definition for USB_COUNT2_TX_1 register  ***************/
#define USB_COUNT2_TX_1_COUNT2_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 2 (high) */

/****************  Bit definition for USB_COUNT3_TX_0 register  ***************/
#define  USB_COUNT3_TX_0_COUNT3_TX_0         ((uint32_t)0x00000000U03FF)        /*!< Transmission Byte Count 3 (low) */

/****************  Bit definition for USB_COUNT3_TX_1 register  ***************/
#define  USB_COUNT3_TX_1_COUNT3_TX_1         ((uint32_t)0x000003FFU0000)        /*!< Transmission Byte Count 3 (high) */

/****************  Bit definition for USB_COUNT4_TX_0 register  ***************/
#define USB_COUNT4_TX_0_COUNT4_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 4 (low) */

/****************  Bit definition for USB_COUNT4_TX_1 register  ***************/
#define USB_COUNT4_TX_1_COUNT4_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 4 (high) */

/****************  Bit definition for USB_COUNT5_TX_0 register  ***************/
#define USB_COUNT5_TX_0_COUNT5_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 5 (low) */

/****************  Bit definition for USB_COUNT5_TX_1 register  ***************/
#define USB_COUNT5_TX_1_COUNT5_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 5 (high) */

/****************  Bit definition for USB_COUNT6_TX_0 register  ***************/
#define USB_COUNT6_TX_0_COUNT6_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 6 (low) */

/****************  Bit definition for USB_COUNT6_TX_1 register  ***************/
#define USB_COUNT6_TX_1_COUNT6_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 6 (high) */

/****************  Bit definition for USB_COUNT7_TX_0 register  ***************/
#define USB_COUNT7_TX_0_COUNT7_TX_0           (0x000003FFU)                    /*!< Transmission Byte Count 7 (low) */

/****************  Bit definition for USB_COUNT7_TX_1 register  ***************/
#define USB_COUNT7_TX_1_COUNT7_TX_1           (0x03FF0000U)                    /*!< Transmission Byte Count 7 (high) */

/*----------------------------------------------------------------------------*/

/*****************  Bit definition for USB_ADDR0_RX register  *****************/
#define USB_ADDR0_RX_ADDR0_RX_Pos             (1U)                             
#define USB_ADDR0_RX_ADDR0_RX_Msk             (0x7FFFU << USB_ADDR0_RX_ADDR0_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR0_RX_ADDR0_RX                 USB_ADDR0_RX_ADDR0_RX_Msk        /*!< Reception Buffer Address 0 */

/*****************  Bit definition for USB_ADDR1_RX register  *****************/
#define USB_ADDR1_RX_ADDR1_RX_Pos             (1U)                             
#define USB_ADDR1_RX_ADDR1_RX_Msk             (0x7FFFU << USB_ADDR1_RX_ADDR1_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR1_RX_ADDR1_RX                 USB_ADDR1_RX_ADDR1_RX_Msk        /*!< Reception Buffer Address 1 */

/*****************  Bit definition for USB_ADDR2_RX register  *****************/
#define USB_ADDR2_RX_ADDR2_RX_Pos             (1U)                             
#define USB_ADDR2_RX_ADDR2_RX_Msk             (0x7FFFU << USB_ADDR2_RX_ADDR2_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR2_RX_ADDR2_RX                 USB_ADDR2_RX_ADDR2_RX_Msk        /*!< Reception Buffer Address 2 */

/*****************  Bit definition for USB_ADDR3_RX register  *****************/
#define USB_ADDR3_RX_ADDR3_RX_Pos             (1U)                             
#define USB_ADDR3_RX_ADDR3_RX_Msk             (0x7FFFU << USB_ADDR3_RX_ADDR3_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR3_RX_ADDR3_RX                 USB_ADDR3_RX_ADDR3_RX_Msk        /*!< Reception Buffer Address 3 */

/*****************  Bit definition for USB_ADDR4_RX register  *****************/
#define USB_ADDR4_RX_ADDR4_RX_Pos             (1U)                             
#define USB_ADDR4_RX_ADDR4_RX_Msk             (0x7FFFU << USB_ADDR4_RX_ADDR4_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR4_RX_ADDR4_RX                 USB_ADDR4_RX_ADDR4_RX_Msk        /*!< Reception Buffer Address 4 */

/*****************  Bit definition for USB_ADDR5_RX register  *****************/
#define USB_ADDR5_RX_ADDR5_RX_Pos             (1U)                             
#define USB_ADDR5_RX_ADDR5_RX_Msk             (0x7FFFU << USB_ADDR5_RX_ADDR5_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR5_RX_ADDR5_RX                 USB_ADDR5_RX_ADDR5_RX_Msk        /*!< Reception Buffer Address 5 */

/*****************  Bit definition for USB_ADDR6_RX register  *****************/
#define USB_ADDR6_RX_ADDR6_RX_Pos             (1U)                             
#define USB_ADDR6_RX_ADDR6_RX_Msk             (0x7FFFU << USB_ADDR6_RX_ADDR6_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR6_RX_ADDR6_RX                 USB_ADDR6_RX_ADDR6_RX_Msk        /*!< Reception Buffer Address 6 */

/*****************  Bit definition for USB_ADDR7_RX register  *****************/
#define USB_ADDR7_RX_ADDR7_RX_Pos             (1U)                             
#define USB_ADDR7_RX_ADDR7_RX_Msk             (0x7FFFU << USB_ADDR7_RX_ADDR7_RX_Pos) /*!< 0x0000FFFE */
#define USB_ADDR7_RX_ADDR7_RX                 USB_ADDR7_RX_ADDR7_RX_Msk        /*!< Reception Buffer Address 7 */

/*----------------------------------------------------------------------------*/

/*****************  Bit definition for USB_COUNT0_RX register  ****************/
#define USB_COUNT0_RX_COUNT0_RX_Pos           (0U)                             
#define USB_COUNT0_RX_COUNT0_RX_Msk           (0x3FFU << USB_COUNT0_RX_COUNT0_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT0_RX_COUNT0_RX               USB_COUNT0_RX_COUNT0_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT0_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT0_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT0_RX_NUM_BLOCK               USB_COUNT0_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT0_RX_NUM_BLOCK_0             (0x01U << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT0_RX_NUM_BLOCK_1             (0x02U << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT0_RX_NUM_BLOCK_2             (0x04U << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT0_RX_NUM_BLOCK_3             (0x08U << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT0_RX_NUM_BLOCK_4             (0x10U << USB_COUNT0_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT0_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT0_RX_BLSIZE_Msk              (0x1U << USB_COUNT0_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT0_RX_BLSIZE                  USB_COUNT0_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT1_RX register  ****************/
#define USB_COUNT1_RX_COUNT1_RX_Pos           (0U)                             
#define USB_COUNT1_RX_COUNT1_RX_Msk           (0x3FFU << USB_COUNT1_RX_COUNT1_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT1_RX_COUNT1_RX               USB_COUNT1_RX_COUNT1_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT1_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT1_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT1_RX_NUM_BLOCK               USB_COUNT1_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT1_RX_NUM_BLOCK_0             (0x01U << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT1_RX_NUM_BLOCK_1             (0x02U << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT1_RX_NUM_BLOCK_2             (0x04U << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT1_RX_NUM_BLOCK_3             (0x08U << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT1_RX_NUM_BLOCK_4             (0x10U << USB_COUNT1_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT1_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT1_RX_BLSIZE_Msk              (0x1U << USB_COUNT1_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT1_RX_BLSIZE                  USB_COUNT1_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT2_RX register  ****************/
#define USB_COUNT2_RX_COUNT2_RX_Pos           (0U)                             
#define USB_COUNT2_RX_COUNT2_RX_Msk           (0x3FFU << USB_COUNT2_RX_COUNT2_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT2_RX_COUNT2_RX               USB_COUNT2_RX_COUNT2_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT2_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT2_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT2_RX_NUM_BLOCK               USB_COUNT2_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT2_RX_NUM_BLOCK_0             (0x01U << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT2_RX_NUM_BLOCK_1             (0x02U << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT2_RX_NUM_BLOCK_2             (0x04U << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT2_RX_NUM_BLOCK_3             (0x08U << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT2_RX_NUM_BLOCK_4             (0x10U << USB_COUNT2_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT2_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT2_RX_BLSIZE_Msk              (0x1U << USB_COUNT2_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT2_RX_BLSIZE                  USB_COUNT2_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT3_RX register  ****************/
#define USB_COUNT3_RX_COUNT3_RX_Pos           (0U)                             
#define USB_COUNT3_RX_COUNT3_RX_Msk           (0x3FFU << USB_COUNT3_RX_COUNT3_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT3_RX_COUNT3_RX               USB_COUNT3_RX_COUNT3_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT3_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT3_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT3_RX_NUM_BLOCK               USB_COUNT3_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT3_RX_NUM_BLOCK_0             (0x01U << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT3_RX_NUM_BLOCK_1             (0x02U << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT3_RX_NUM_BLOCK_2             (0x04U << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT3_RX_NUM_BLOCK_3             (0x08U << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT3_RX_NUM_BLOCK_4             (0x10U << USB_COUNT3_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT3_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT3_RX_BLSIZE_Msk              (0x1U << USB_COUNT3_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT3_RX_BLSIZE                  USB_COUNT3_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT4_RX register  ****************/
#define USB_COUNT4_RX_COUNT4_RX_Pos           (0U)                             
#define USB_COUNT4_RX_COUNT4_RX_Msk           (0x3FFU << USB_COUNT4_RX_COUNT4_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT4_RX_COUNT4_RX               USB_COUNT4_RX_COUNT4_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT4_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT4_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT4_RX_NUM_BLOCK               USB_COUNT4_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT4_RX_NUM_BLOCK_0             (0x01U << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT4_RX_NUM_BLOCK_1             (0x02U << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT4_RX_NUM_BLOCK_2             (0x04U << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT4_RX_NUM_BLOCK_3             (0x08U << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT4_RX_NUM_BLOCK_4             (0x10U << USB_COUNT4_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT4_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT4_RX_BLSIZE_Msk              (0x1U << USB_COUNT4_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT4_RX_BLSIZE                  USB_COUNT4_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT5_RX register  ****************/
#define USB_COUNT5_RX_COUNT5_RX_Pos           (0U)                             
#define USB_COUNT5_RX_COUNT5_RX_Msk           (0x3FFU << USB_COUNT5_RX_COUNT5_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT5_RX_COUNT5_RX               USB_COUNT5_RX_COUNT5_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT5_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT5_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT5_RX_NUM_BLOCK               USB_COUNT5_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT5_RX_NUM_BLOCK_0             (0x01U << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT5_RX_NUM_BLOCK_1             (0x02U << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT5_RX_NUM_BLOCK_2             (0x04U << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT5_RX_NUM_BLOCK_3             (0x08U << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT5_RX_NUM_BLOCK_4             (0x10U << USB_COUNT5_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT5_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT5_RX_BLSIZE_Msk              (0x1U << USB_COUNT5_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT5_RX_BLSIZE                  USB_COUNT5_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT6_RX register  ****************/
#define USB_COUNT6_RX_COUNT6_RX_Pos           (0U)                             
#define USB_COUNT6_RX_COUNT6_RX_Msk           (0x3FFU << USB_COUNT6_RX_COUNT6_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT6_RX_COUNT6_RX               USB_COUNT6_RX_COUNT6_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT6_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT6_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT6_RX_NUM_BLOCK               USB_COUNT6_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT6_RX_NUM_BLOCK_0             (0x01U << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT6_RX_NUM_BLOCK_1             (0x02U << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT6_RX_NUM_BLOCK_2             (0x04U << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT6_RX_NUM_BLOCK_3             (0x08U << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT6_RX_NUM_BLOCK_4             (0x10U << USB_COUNT6_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT6_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT6_RX_BLSIZE_Msk              (0x1U << USB_COUNT6_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT6_RX_BLSIZE                  USB_COUNT6_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*****************  Bit definition for USB_COUNT7_RX register  ****************/
#define USB_COUNT7_RX_COUNT7_RX_Pos           (0U)                             
#define USB_COUNT7_RX_COUNT7_RX_Msk           (0x3FFU << USB_COUNT7_RX_COUNT7_RX_Pos) /*!< 0x000003FF */
#define USB_COUNT7_RX_COUNT7_RX               USB_COUNT7_RX_COUNT7_RX_Msk      /*!< Reception Byte Count */

#define USB_COUNT7_RX_NUM_BLOCK_Pos           (10U)                            
#define USB_COUNT7_RX_NUM_BLOCK_Msk           (0x1FU << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00007C00 */
#define USB_COUNT7_RX_NUM_BLOCK               USB_COUNT7_RX_NUM_BLOCK_Msk      /*!< NUM_BLOCK[4:0] bits (Number of blocks) */
#define USB_COUNT7_RX_NUM_BLOCK_0             (0x01U << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00000400 */
#define USB_COUNT7_RX_NUM_BLOCK_1             (0x02U << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00000800 */
#define USB_COUNT7_RX_NUM_BLOCK_2             (0x04U << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00001000 */
#define USB_COUNT7_RX_NUM_BLOCK_3             (0x08U << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00002000 */
#define USB_COUNT7_RX_NUM_BLOCK_4             (0x10U << USB_COUNT7_RX_NUM_BLOCK_Pos) /*!< 0x00004000 */

#define USB_COUNT7_RX_BLSIZE_Pos              (15U)                            
#define USB_COUNT7_RX_BLSIZE_Msk              (0x1U << USB_COUNT7_RX_BLSIZE_Pos) /*!< 0x00008000 */
#define USB_COUNT7_RX_BLSIZE                  USB_COUNT7_RX_BLSIZE_Msk         /*!< BLock SIZE */

/*----------------------------------------------------------------------------*/

/****************  Bit definition for USB_COUNT0_RX_0 register  ***************/
#define USB_COUNT0_RX_0_COUNT0_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT0_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT0_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT0_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT0_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT0_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT0_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT0_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT0_RX_1 register  ***************/
#define USB_COUNT0_RX_1_COUNT0_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT0_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT0_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 1 */
#define USB_COUNT0_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT0_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT0_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT0_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT0_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/****************  Bit definition for USB_COUNT1_RX_0 register  ***************/
#define USB_COUNT1_RX_0_COUNT1_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT1_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT1_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT1_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT1_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT1_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT1_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT1_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT1_RX_1 register  ***************/
#define USB_COUNT1_RX_1_COUNT1_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT1_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT1_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT1_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT1_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT1_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT1_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT1_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/****************  Bit definition for USB_COUNT2_RX_0 register  ***************/
#define USB_COUNT2_RX_0_COUNT2_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT2_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT2_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT2_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT2_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT2_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT2_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT2_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT2_RX_1 register  ***************/
#define USB_COUNT2_RX_1_COUNT2_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT2_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT2_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT2_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT2_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT2_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT2_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT2_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/****************  Bit definition for USB_COUNT3_RX_0 register  ***************/
#define USB_COUNT3_RX_0_COUNT3_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT3_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT3_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT3_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT3_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT3_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT3_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT3_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT3_RX_1 register  ***************/
#define USB_COUNT3_RX_1_COUNT3_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT3_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT3_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT3_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT3_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT3_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT3_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT3_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/****************  Bit definition for USB_COUNT4_RX_0 register  ***************/
#define USB_COUNT4_RX_0_COUNT4_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT4_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT4_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT4_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT4_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT4_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT4_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT4_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT4_RX_1 register  ***************/
#define USB_COUNT4_RX_1_COUNT4_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT4_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT4_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT4_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT4_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT4_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT4_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT4_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/****************  Bit definition for USB_COUNT5_RX_0 register  ***************/
#define USB_COUNT5_RX_0_COUNT5_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT5_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT5_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT5_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT5_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT5_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT5_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT5_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT5_RX_1 register  ***************/
#define USB_COUNT5_RX_1_COUNT5_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT5_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT5_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT5_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT5_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT5_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT5_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT5_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/***************  Bit definition for USB_COUNT6_RX_0  register  ***************/
#define USB_COUNT6_RX_0_COUNT6_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT6_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT6_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT6_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT6_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT6_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT6_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT6_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/****************  Bit definition for USB_COUNT6_RX_1 register  ***************/
#define USB_COUNT6_RX_1_COUNT6_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT6_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT6_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT6_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT6_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT6_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT6_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT6_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/***************  Bit definition for USB_COUNT7_RX_0 register  ****************/
#define USB_COUNT7_RX_0_COUNT7_RX_0           (0x000003FFU)                    /*!< Reception Byte Count (low) */

#define USB_COUNT7_RX_0_NUM_BLOCK_0           (0x00007C00U)                    /*!< NUM_BLOCK_0[4:0] bits (Number of blocks) (low) */
#define USB_COUNT7_RX_0_NUM_BLOCK_0_0         (0x00000400U)                    /*!< Bit 0 */
#define USB_COUNT7_RX_0_NUM_BLOCK_0_1         (0x00000800U)                    /*!< Bit 1 */
#define USB_COUNT7_RX_0_NUM_BLOCK_0_2         (0x00001000U)                    /*!< Bit 2 */
#define USB_COUNT7_RX_0_NUM_BLOCK_0_3         (0x00002000U)                    /*!< Bit 3 */
#define USB_COUNT7_RX_0_NUM_BLOCK_0_4         (0x00004000U)                    /*!< Bit 4 */

#define USB_COUNT7_RX_0_BLSIZE_0              (0x00008000U)                    /*!< BLock SIZE (low) */

/***************  Bit definition for USB_COUNT7_RX_1 register  ****************/
#define USB_COUNT7_RX_1_COUNT7_RX_1           (0x03FF0000U)                    /*!< Reception Byte Count (high) */

#define USB_COUNT7_RX_1_NUM_BLOCK_1           (0x7C000000U)                    /*!< NUM_BLOCK_1[4:0] bits (Number of blocks) (high) */
#define USB_COUNT7_RX_1_NUM_BLOCK_1_0         (0x04000000U)                    /*!< Bit 0 */
#define USB_COUNT7_RX_1_NUM_BLOCK_1_1         (0x08000000U)                    /*!< Bit 1 */
#define USB_COUNT7_RX_1_NUM_BLOCK_1_2         (0x10000000U)                    /*!< Bit 2 */
#define USB_COUNT7_RX_1_NUM_BLOCK_1_3         (0x20000000U)                    /*!< Bit 3 */
#define USB_COUNT7_RX_1_NUM_BLOCK_1_4         (0x40000000U)                    /*!< Bit 4 */

#define USB_COUNT7_RX_1_BLSIZE_1              (0x80000000U)                    /*!< BLock SIZE (high) */

/******************************************************************************/
/*                                                                            */
/*                         Window WATCHDOG (WWDG)                             */
/*                                                                            */
/******************************************************************************/

/*******************  Bit definition for WWDG_CR register  ********************/
#define WWDG_CR_T_Pos                       (0U)                               
#define WWDG_CR_T_Msk                       (0x7FU << WWDG_CR_T_Pos)           /*!< 0x0000007F */
#define WWDG_CR_T                           WWDG_CR_T_Msk                      /*!< T[6:0] bits (7-Bit counter (MSB to LSB)) */
#define WWDG_CR_T_0                         (0x01U << WWDG_CR_T_Pos)           /*!< 0x00000001 */
#define WWDG_CR_T_1                         (0x02U << WWDG_CR_T_Pos)           /*!< 0x00000002 */
#define WWDG_CR_T_2                         (0x04U << WWDG_CR_T_Pos)           /*!< 0x00000004 */
#define WWDG_CR_T_3                         (0x08U << WWDG_CR_T_Pos)           /*!< 0x00000008 */
#define WWDG_CR_T_4                         (0x10U << WWDG_CR_T_Pos)           /*!< 0x00000010 */
#define WWDG_CR_T_5                         (0x20U << WWDG_CR_T_Pos)           /*!< 0x00000020 */
#define WWDG_CR_T_6                         (0x40U << WWDG_CR_T_Pos)           /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CR_T0 WWDG_CR_T_0
#define  WWDG_CR_T1 WWDG_CR_T_1
#define  WWDG_CR_T2 WWDG_CR_T_2
#define  WWDG_CR_T3 WWDG_CR_T_3
#define  WWDG_CR_T4 WWDG_CR_T_4
#define  WWDG_CR_T5 WWDG_CR_T_5
#define  WWDG_CR_T6 WWDG_CR_T_6

#define WWDG_CR_WDGA_Pos                    (7U)                               
#define WWDG_CR_WDGA_Msk                    (0x1U << WWDG_CR_WDGA_Pos)         /*!< 0x00000080 */
#define WWDG_CR_WDGA                        WWDG_CR_WDGA_Msk                   /*!< Activation bit */

/*******************  Bit definition for WWDG_CFR register  *******************/
#define WWDG_CFR_W_Pos                      (0U)                               
#define WWDG_CFR_W_Msk                      (0x7FU << WWDG_CFR_W_Pos)          /*!< 0x0000007F */
#define WWDG_CFR_W                          WWDG_CFR_W_Msk                     /*!< W[6:0] bits (7-bit window value) */
#define WWDG_CFR_W_0                        (0x01U << WWDG_CFR_W_Pos)          /*!< 0x00000001 */
#define WWDG_CFR_W_1                        (0x02U << WWDG_CFR_W_Pos)          /*!< 0x00000002 */
#define WWDG_CFR_W_2                        (0x04U << WWDG_CFR_W_Pos)          /*!< 0x00000004 */
#define WWDG_CFR_W_3                        (0x08U << WWDG_CFR_W_Pos)          /*!< 0x00000008 */
#define WWDG_CFR_W_4                        (0x10U << WWDG_CFR_W_Pos)          /*!< 0x00000010 */
#define WWDG_CFR_W_5                        (0x20U << WWDG_CFR_W_Pos)          /*!< 0x00000020 */
#define WWDG_CFR_W_6                        (0x40U << WWDG_CFR_W_Pos)          /*!< 0x00000040 */

/* Legacy defines */
#define  WWDG_CFR_W0 WWDG_CFR_W_0
#define  WWDG_CFR_W1 WWDG_CFR_W_1
#define  WWDG_CFR_W2 WWDG_CFR_W_2
#define  WWDG_CFR_W3 WWDG_CFR_W_3
#define  WWDG_CFR_W4 WWDG_CFR_W_4
#define  WWDG_CFR_W5 WWDG_CFR_W_5
#define  WWDG_CFR_W6 WWDG_CFR_W_6

#define WWDG_CFR_WDGTB_Pos                  (7U)                               
#define WWDG_CFR_WDGTB_Msk                  (0x3U << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000180 */
#define WWDG_CFR_WDGTB                      WWDG_CFR_WDGTB_Msk                 /*!< WDGTB[1:0] bits (Timer Base) */
#define WWDG_CFR_WDGTB_0                    (0x1U << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000080 */
#define WWDG_CFR_WDGTB_1                    (0x2U << WWDG_CFR_WDGTB_Pos)       /*!< 0x00000100 */

/* Legacy defines */
#define  WWDG_CFR_WDGTB0 WWDG_CFR_WDGTB_0
#define  WWDG_CFR_WDGTB1 WWDG_CFR_WDGTB_1

#define WWDG_CFR_EWI_Pos                    (9U)                               
#define WWDG_CFR_EWI_Msk                    (0x1U << WWDG_CFR_EWI_Pos)         /*!< 0x00000200 */
#define WWDG_CFR_EWI                        WWDG_CFR_EWI_Msk                   /*!< Early Wakeup Interrupt */

/*******************  Bit definition for WWDG_SR register  ********************/
#define WWDG_SR_EWIF_Pos                    (0U)                               
#define WWDG_SR_EWIF_Msk                    (0x1U << WWDG_SR_EWIF_Pos)         /*!< 0x00000001 */
#define WWDG_SR_EWIF                        WWDG_SR_EWIF_Msk                   /*!< Early Wakeup Interrupt Flag */

/******************************************************************************/
/*                                                                            */
/*                        SystemTick (SysTick)                                */
/*                                                                            */
/******************************************************************************/

/*****************  Bit definition for SysTick_CTRL register  *****************/
#define SysTick_CTRL_ENABLE                 (0x00000001U)                      /*!< Counter enable */
#define SysTick_CTRL_TICKINT                (0x00000002U)                      /*!< Counting down to 0 pends the SysTick handler */
#define SysTick_CTRL_CLKSOURCE              (0x00000004U)                      /*!< Clock source */
#define SysTick_CTRL_COUNTFLAG              (0x00010000U)                      /*!< Count Flag */

/*****************  Bit definition for SysTick_LOAD register  *****************/
#define SysTick_LOAD_RELOAD                 (0x00FFFFFFU)                      /*!< Value to load into the SysTick Current Value Register when the counter reaches 0 */

/*****************  Bit definition for SysTick_VAL register  ******************/
#define SysTick_VAL_CURRENT                 (0x00FFFFFFU)                      /*!< Current value at the time the register is accessed */

/*****************  Bit definition for SysTick_CALIB register  ****************/
#define SysTick_CALIB_TENMS                 (0x00FFFFFFU)                      /*!< Reload value to use for 10ms timing */
#define SysTick_CALIB_SKEW                  (0x40000000U)                      /*!< Calibration value is not exactly 10 ms */
#define SysTick_CALIB_NOREF                 (0x80000000U)                      /*!< The reference clock is not provided */

/******************************************************************************/
/*                                                                            */
/*               Nested Vectored Interrupt Controller (NVIC)                  */
/*                                                                            */
/******************************************************************************/

/******************  Bit definition for NVIC_ISER register  *******************/
#define NVIC_ISER_SETENA_Pos                (0U)                               
#define NVIC_ISER_SETENA_Msk                (0xFFFFFFFFU << NVIC_ISER_SETENA_Pos) /*!< 0xFFFFFFFF */
#define NVIC_ISER_SETENA                    NVIC_ISER_SETENA_Msk               /*!< Interrupt set enable bits */
#define NVIC_ISER_SETENA_0                  (0x00000001U << NVIC_ISER_SETENA_Pos) /*!< 0x00000001 */
#define NVIC_ISER_SETENA_1                  (0x00000002U << NVIC_ISER_SETENA_Pos) /*!< 0x00000002 */
#define NVIC_ISER_SETENA_2                  (0x00000004U << NVIC_ISER_SETENA_Pos) /*!< 0x00000004 */
#define NVIC_ISER_SETENA_3                  (0x00000008U << NVIC_ISER_SETENA_Pos) /*!< 0x00000008 */
#define NVIC_ISER_SETENA_4                  (0x00000010U << NVIC_ISER_SETENA_Pos) /*!< 0x00000010 */
#define NVIC_ISER_SETENA_5                  (0x00000020U << NVIC_ISER_SETENA_Pos) /*!< 0x00000020 */
#define NVIC_ISER_SETENA_6                  (0x00000040U << NVIC_ISER_SETENA_Pos) /*!< 0x00000040 */
#define NVIC_ISER_SETENA_7                  (0x00000080U << NVIC_ISER_SETENA_Pos) /*!< 0x00000080 */
#define NVIC_ISER_SETENA_8                  (0x00000100U << NVIC_ISER_SETENA_Pos) /*!< 0x00000100 */
#define NVIC_ISER_SETENA_9                  (0x00000200U << NVIC_ISER_SETENA_Pos) /*!< 0x00000200 */
#define NVIC_ISER_SETENA_10                 (0x00000400U << NVIC_ISER_SETENA_Pos) /*!< 0x00000400 */
#define NVIC_ISER_SETENA_11                 (0x00000800U << NVIC_ISER_SETENA_Pos) /*!< 0x00000800 */
#define NVIC_ISER_SETENA_12                 (0x00001000U << NVIC_ISER_SETENA_Pos) /*!< 0x00001000 */
#define NVIC_ISER_SETENA_13                 (0x00002000U << NVIC_ISER_SETENA_Pos) /*!< 0x00002000 */
#define NVIC_ISER_SETENA_14                 (0x00004000U << NVIC_ISER_SETENA_Pos) /*!< 0x00004000 */
#define NVIC_ISER_SETENA_15                 (0x00008000U << NVIC_ISER_SETENA_Pos) /*!< 0x00008000 */
#define NVIC_ISER_SETENA_16                 (0x00010000U << NVIC_ISER_SETENA_Pos) /*!< 0x00010000 */
#define NVIC_ISER_SETENA_17                 (0x00020000U << NVIC_ISER_SETENA_Pos) /*!< 0x00020000 */
#define NVIC_ISER_SETENA_18                 (0x00040000U << NVIC_ISER_SETENA_Pos) /*!< 0x00040000 */
#define NVIC_ISER_SETENA_19                 (0x00080000U << NVIC_ISER_SETENA_Pos) /*!< 0x00080000 */
#define NVIC_ISER_SETENA_20                 (0x00100000U << NVIC_ISER_SETENA_Pos) /*!< 0x00100000 */
#define NVIC_ISER_SETENA_21                 (0x00200000U << NVIC_ISER_SETENA_Pos) /*!< 0x00200000 */
#define NVIC_ISER_SETENA_22                 (0x00400000U << NVIC_ISER_SETENA_Pos) /*!< 0x00400000 */
#define NVIC_ISER_SETENA_23                 (0x00800000U << NVIC_ISER_SETENA_Pos) /*!< 0x00800000 */
#define NVIC_ISER_SETENA_24                 (0x01000000U << NVIC_ISER_SETENA_Pos) /*!< 0x01000000 */
#define NVIC_ISER_SETENA_25                 (0x02000000U << NVIC_ISER_SETENA_Pos) /*!< 0x02000000 */
#define NVIC_ISER_SETENA_26                 (0x04000000U << NVIC_ISER_SETENA_Pos) /*!< 0x04000000 */
#define NVIC_ISER_SETENA_27                 (0x08000000U << NVIC_ISER_SETENA_Pos) /*!< 0x08000000 */
#define NVIC_ISER_SETENA_28                 (0x10000000U << NVIC_ISER_SETENA_Pos) /*!< 0x10000000 */
#define NVIC_ISER_SETENA_29                 (0x20000000U << NVIC_ISER_SETENA_Pos) /*!< 0x20000000 */
#define NVIC_ISER_SETENA_30                 (0x40000000U << NVIC_ISER_SETENA_Pos) /*!< 0x40000000 */
#define NVIC_ISER_SETENA_31                 (0x80000000U << NVIC_ISER_SETENA_Pos) /*!< 0x80000000 */

/******************  Bit definition for NVIC_ICER register  *******************/
#define NVIC_ICER_CLRENA_Pos                (0U)                               
#define NVIC_ICER_CLRENA_Msk                (0xFFFFFFFFU << NVIC_ICER_CLRENA_Pos) /*!< 0xFFFFFFFF */
#define NVIC_ICER_CLRENA                    NVIC_ICER_CLRENA_Msk               /*!< Interrupt clear-enable bits */
#define NVIC_ICER_CLRENA_0                  (0x00000001U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000001 */
#define NVIC_ICER_CLRENA_1                  (0x00000002U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000002 */
#define NVIC_ICER_CLRENA_2                  (0x00000004U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000004 */
#define NVIC_ICER_CLRENA_3                  (0x00000008U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000008 */
#define NVIC_ICER_CLRENA_4                  (0x00000010U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000010 */
#define NVIC_ICER_CLRENA_5                  (0x00000020U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000020 */
#define NVIC_ICER_CLRENA_6                  (0x00000040U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000040 */
#define NVIC_ICER_CLRENA_7                  (0x00000080U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000080 */
#define NVIC_ICER_CLRENA_8                  (0x00000100U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000100 */
#define NVIC_ICER_CLRENA_9                  (0x00000200U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000200 */
#define NVIC_ICER_CLRENA_10                 (0x00000400U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000400 */
#define NVIC_ICER_CLRENA_11                 (0x00000800U << NVIC_ICER_CLRENA_Pos) /*!< 0x00000800 */
#define NVIC_ICER_CLRENA_12                 (0x00001000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00001000 */
#define NVIC_ICER_CLRENA_13                 (0x00002000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00002000 */
#define NVIC_ICER_CLRENA_14                 (0x00004000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00004000 */
#define NVIC_ICER_CLRENA_15                 (0x00008000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00008000 */
#define NVIC_ICER_CLRENA_16                 (0x00010000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00010000 */
#define NVIC_ICER_CLRENA_17                 (0x00020000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00020000 */
#define NVIC_ICER_CLRENA_18                 (0x00040000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00040000 */
#define NVIC_ICER_CLRENA_19                 (0x00080000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00080000 */
#define NVIC_ICER_CLRENA_20                 (0x00100000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00100000 */
#define NVIC_ICER_CLRENA_21                 (0x00200000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00200000 */
#define NVIC_ICER_CLRENA_22                 (0x00400000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00400000 */
#define NVIC_ICER_CLRENA_23                 (0x00800000U << NVIC_ICER_CLRENA_Pos) /*!< 0x00800000 */
#define NVIC_ICER_CLRENA_24                 (0x01000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x01000000 */
#define NVIC_ICER_CLRENA_25                 (0x02000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x02000000 */
#define NVIC_ICER_CLRENA_26                 (0x04000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x04000000 */
#define NVIC_ICER_CLRENA_27                 (0x08000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x08000000 */
#define NVIC_ICER_CLRENA_28                 (0x10000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x10000000 */
#define NVIC_ICER_CLRENA_29                 (0x20000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x20000000 */
#define NVIC_ICER_CLRENA_30                 (0x40000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x40000000 */
#define NVIC_ICER_CLRENA_31                 (0x80000000U << NVIC_ICER_CLRENA_Pos) /*!< 0x80000000 */

/******************  Bit definition for NVIC_ISPR register  *******************/
#define NVIC_ISPR_SETPEND_Pos               (0U)                               
#define NVIC_ISPR_SETPEND_Msk               (0xFFFFFFFFU << NVIC_ISPR_SETPEND_Pos) /*!< 0xFFFFFFFF */
#define NVIC_ISPR_SETPEND                   NVIC_ISPR_SETPEND_Msk              /*!< Interrupt set-pending bits */
#define NVIC_ISPR_SETPEND_0                 (0x00000001U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000001 */
#define NVIC_ISPR_SETPEND_1                 (0x00000002U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000002 */
#define NVIC_ISPR_SETPEND_2                 (0x00000004U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000004 */
#define NVIC_ISPR_SETPEND_3                 (0x00000008U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000008 */
#define NVIC_ISPR_SETPEND_4                 (0x00000010U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000010 */
#define NVIC_ISPR_SETPEND_5                 (0x00000020U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000020 */
#define NVIC_ISPR_SETPEND_6                 (0x00000040U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000040 */
#define NVIC_ISPR_SETPEND_7                 (0x00000080U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000080 */
#define NVIC_ISPR_SETPEND_8                 (0x00000100U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000100 */
#define NVIC_ISPR_SETPEND_9                 (0x00000200U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000200 */
#define NVIC_ISPR_SETPEND_10                (0x00000400U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000400 */
#define NVIC_ISPR_SETPEND_11                (0x00000800U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00000800 */
#define NVIC_ISPR_SETPEND_12                (0x00001000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00001000 */
#define NVIC_ISPR_SETPEND_13                (0x00002000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00002000 */
#define NVIC_ISPR_SETPEND_14                (0x00004000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00004000 */
#define NVIC_ISPR_SETPEND_15                (0x00008000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00008000 */
#define NVIC_ISPR_SETPEND_16                (0x00010000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00010000 */
#define NVIC_ISPR_SETPEND_17                (0x00020000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00020000 */
#define NVIC_ISPR_SETPEND_18                (0x00040000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00040000 */
#define NVIC_ISPR_SETPEND_19                (0x00080000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00080000 */
#define NVIC_ISPR_SETPEND_20                (0x00100000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00100000 */
#define NVIC_ISPR_SETPEND_21                (0x00200000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00200000 */
#define NVIC_ISPR_SETPEND_22                (0x00400000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00400000 */
#define NVIC_ISPR_SETPEND_23                (0x00800000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x00800000 */
#define NVIC_ISPR_SETPEND_24                (0x01000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x01000000 */
#define NVIC_ISPR_SETPEND_25                (0x02000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x02000000 */
#define NVIC_ISPR_SETPEND_26                (0x04000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x04000000 */
#define NVIC_ISPR_SETPEND_27                (0x08000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x08000000 */
#define NVIC_ISPR_SETPEND_28                (0x10000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x10000000 */
#define NVIC_ISPR_SETPEND_29                (0x20000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x20000000 */
#define NVIC_ISPR_SETPEND_30                (0x40000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x40000000 */
#define NVIC_ISPR_SETPEND_31                (0x80000000U << NVIC_ISPR_SETPEND_Pos) /*!< 0x80000000 */

/******************  Bit definition for NVIC_ICPR register  *******************/
#define NVIC_ICPR_CLRPEND_Pos               (0U)                               
#define NVIC_ICPR_CLRPEND_Msk               (0xFFFFFFFFU << NVIC_ICPR_CLRPEND_Pos) /*!< 0xFFFFFFFF */
#define NVIC_ICPR_CLRPEND                   NVIC_ICPR_CLRPEND_Msk              /*!< Interrupt clear-pending bits */
#define NVIC_ICPR_CLRPEND_0                 (0x00000001U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000001 */
#define NVIC_ICPR_CLRPEND_1                 (0x00000002U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000002 */
#define NVIC_ICPR_CLRPEND_2                 (0x00000004U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000004 */
#define NVIC_ICPR_CLRPEND_3                 (0x00000008U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000008 */
#define NVIC_ICPR_CLRPEND_4                 (0x00000010U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000010 */
#define NVIC_ICPR_CLRPEND_5                 (0x00000020U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000020 */
#define NVIC_ICPR_CLRPEND_6                 (0x00000040U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000040 */
#define NVIC_ICPR_CLRPEND_7                 (0x00000080U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000080 */
#define NVIC_ICPR_CLRPEND_8                 (0x00000100U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000100 */
#define NVIC_ICPR_CLRPEND_9                 (0x00000200U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000200 */
#define NVIC_ICPR_CLRPEND_10                (0x00000400U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000400 */
#define NVIC_ICPR_CLRPEND_11                (0x00000800U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00000800 */
#define NVIC_ICPR_CLRPEND_12                (0x00001000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00001000 */
#define NVIC_ICPR_CLRPEND_13                (0x00002000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00002000 */
#define NVIC_ICPR_CLRPEND_14                (0x00004000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00004000 */
#define NVIC_ICPR_CLRPEND_15                (0x00008000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00008000 */
#define NVIC_ICPR_CLRPEND_16                (0x00010000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00010000 */
#define NVIC_ICPR_CLRPEND_17                (0x00020000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00020000 */
#define NVIC_ICPR_CLRPEND_18                (0x00040000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00040000 */
#define NVIC_ICPR_CLRPEND_19                (0x00080000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00080000 */
#define NVIC_ICPR_CLRPEND_20                (0x00100000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00100000 */
#define NVIC_ICPR_CLRPEND_21                (0x00200000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00200000 */
#define NVIC_ICPR_CLRPEND_22                (0x00400000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00400000 */
#define NVIC_ICPR_CLRPEND_23                (0x00800000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x00800000 */
#define NVIC_ICPR_CLRPEND_24                (0x01000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x01000000 */
#define NVIC_ICPR_CLRPEND_25                (0x02000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x02000000 */
#define NVIC_ICPR_CLRPEND_26                (0x04000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x04000000 */
#define NVIC_ICPR_CLRPEND_27                (0x08000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x08000000 */
#define NVIC_ICPR_CLRPEND_28                (0x10000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x10000000 */
#define NVIC_ICPR_CLRPEND_29                (0x20000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x20000000 */
#define NVIC_ICPR_CLRPEND_30                (0x40000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x40000000 */
#define NVIC_ICPR_CLRPEND_31                (0x80000000U << NVIC_ICPR_CLRPEND_Pos) /*!< 0x80000000 */

/******************  Bit definition for NVIC_IABR register  *******************/
#define NVIC_IABR_ACTIVE_Pos                (0U)                               
#define NVIC_IABR_ACTIVE_Msk                (0xFFFFFFFFU << NVIC_IABR_ACTIVE_Pos) /*!< 0xFFFFFFFF */
#define NVIC_IABR_ACTIVE                    NVIC_IABR_ACTIVE_Msk               /*!< Interrupt active flags */
#define NVIC_IABR_ACTIVE_0                  (0x00000001U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000001 */
#define NVIC_IABR_ACTIVE_1                  (0x00000002U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000002 */
#define NVIC_IABR_ACTIVE_2                  (0x00000004U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000004 */
#define NVIC_IABR_ACTIVE_3                  (0x00000008U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000008 */
#define NVIC_IABR_ACTIVE_4                  (0x00000010U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000010 */
#define NVIC_IABR_ACTIVE_5                  (0x00000020U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000020 */
#define NVIC_IABR_ACTIVE_6                  (0x00000040U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000040 */
#define NVIC_IABR_ACTIVE_7                  (0x00000080U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000080 */
#define NVIC_IABR_ACTIVE_8                  (0x00000100U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000100 */
#define NVIC_IABR_ACTIVE_9                  (0x00000200U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000200 */
#define NVIC_IABR_ACTIVE_10                 (0x00000400U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000400 */
#define NVIC_IABR_ACTIVE_11                 (0x00000800U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00000800 */
#define NVIC_IABR_ACTIVE_12                 (0x00001000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00001000 */
#define NVIC_IABR_ACTIVE_13                 (0x00002000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00002000 */
#define NVIC_IABR_ACTIVE_14                 (0x00004000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00004000 */
#define NVIC_IABR_ACTIVE_15                 (0x00008000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00008000 */
#define NVIC_IABR_ACTIVE_16                 (0x00010000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00010000 */
#define NVIC_IABR_ACTIVE_17                 (0x00020000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00020000 */
#define NVIC_IABR_ACTIVE_18                 (0x00040000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00040000 */
#define NVIC_IABR_ACTIVE_19                 (0x00080000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00080000 */
#define NVIC_IABR_ACTIVE_20                 (0x00100000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00100000 */
#define NVIC_IABR_ACTIVE_21                 (0x00200000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00200000 */
#define NVIC_IABR_ACTIVE_22                 (0x00400000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00400000 */
#define NVIC_IABR_ACTIVE_23                 (0x00800000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x00800000 */
#define NVIC_IABR_ACTIVE_24                 (0x01000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x01000000 */
#define NVIC_IABR_ACTIVE_25                 (0x02000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x02000000 */
#define NVIC_IABR_ACTIVE_26                 (0x04000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x04000000 */
#define NVIC_IABR_ACTIVE_27                 (0x08000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x08000000 */
#define NVIC_IABR_ACTIVE_28                 (0x10000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x10000000 */
#define NVIC_IABR_ACTIVE_29                 (0x20000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x20000000 */
#define NVIC_IABR_ACTIVE_30                 (0x40000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x40000000 */
#define NVIC_IABR_ACTIVE_31                 (0x80000000U << NVIC_IABR_ACTIVE_Pos) /*!< 0x80000000 */

/******************  Bit definition for NVIC_PRI0 register  *******************/
#define NVIC_IPR0_PRI_0                     (0x000000FFU)                      /*!< Priority of interrupt 0 */
#define NVIC_IPR0_PRI_1                     (0x0000FF00U)                      /*!< Priority of interrupt 1 */
#define NVIC_IPR0_PRI_2                     (0x00FF0000U)                      /*!< Priority of interrupt 2 */
#define NVIC_IPR0_PRI_3                     (0xFF000000U)                      /*!< Priority of interrupt 3 */

/******************  Bit definition for NVIC_PRI1 register  *******************/
#define NVIC_IPR1_PRI_4                     (0x000000FFU)                      /*!< Priority of interrupt 4 */
#define NVIC_IPR1_PRI_5                     (0x0000FF00U)                      /*!< Priority of interrupt 5 */
#define NVIC_IPR1_PRI_6                     (0x00FF0000U)                      /*!< Priority of interrupt 6 */
#define NVIC_IPR1_PRI_7                     (0xFF000000U)                      /*!< Priority of interrupt 7 */

/******************  Bit definition for NVIC_PRI2 register  *******************/
#define NVIC_IPR2_PRI_8                     (0x000000FFU)                      /*!< Priority of interrupt 8 */
#define NVIC_IPR2_PRI_9                     (0x0000FF00U)                      /*!< Priority of interrupt 9 */
#define NVIC_IPR2_PRI_10                    (0x00FF0000U)                      /*!< Priority of interrupt 10 */
#define NVIC_IPR2_PRI_11                    (0xFF000000U)                      /*!< Priority of interrupt 11 */

/******************  Bit definition for NVIC_PRI3 register  *******************/
#define NVIC_IPR3_PRI_12                    (0x000000FFU)                      /*!< Priority of interrupt 12 */
#define NVIC_IPR3_PRI_13                    (0x0000FF00U)                      /*!< Priority of interrupt 13 */
#define NVIC_IPR3_PRI_14                    (0x00FF0000U)                      /*!< Priority of interrupt 14 */
#define NVIC_IPR3_PRI_15                    (0xFF000000U)                      /*!< Priority of interrupt 15 */

/******************  Bit definition for NVIC_PRI4 register  *******************/
#define NVIC_IPR4_PRI_16                    (0x000000FFU)                      /*!< Priority of interrupt 16 */
#define NVIC_IPR4_PRI_17                    (0x0000FF00U)                      /*!< Priority of interrupt 17 */
#define NVIC_IPR4_PRI_18                    (0x00FF0000U)                      /*!< Priority of interrupt 18 */
#define NVIC_IPR4_PRI_19                    (0xFF000000U)                      /*!< Priority of interrupt 19 */

/******************  Bit definition for NVIC_PRI5 register  *******************/
#define NVIC_IPR5_PRI_20                    (0x000000FFU)                      /*!< Priority of interrupt 20 */
#define NVIC_IPR5_PRI_21                    (0x0000FF00U)                      /*!< Priority of interrupt 21 */
#define NVIC_IPR5_PRI_22                    (0x00FF0000U)                      /*!< Priority of interrupt 22 */
#define NVIC_IPR5_PRI_23                    (0xFF000000U)                      /*!< Priority of interrupt 23 */

/******************  Bit definition for NVIC_PRI6 register  *******************/
#define NVIC_IPR6_PRI_24                    (0x000000FFU)                      /*!< Priority of interrupt 24 */
#define NVIC_IPR6_PRI_25                    (0x0000FF00U)                      /*!< Priority of interrupt 25 */
#define NVIC_IPR6_PRI_26                    (0x00FF0000U)                      /*!< Priority of interrupt 26 */
#define NVIC_IPR6_PRI_27                    (0xFF000000U)                      /*!< Priority of interrupt 27 */

/******************  Bit definition for NVIC_PRI7 register  *******************/
#define NVIC_IPR7_PRI_28                    (0x000000FFU)                      /*!< Priority of interrupt 28 */
#define NVIC_IPR7_PRI_29                    (0x0000FF00U)                      /*!< Priority of interrupt 29 */
#define NVIC_IPR7_PRI_30                    (0x00FF0000U)                      /*!< Priority of interrupt 30 */
#define NVIC_IPR7_PRI_31                    (0xFF000000U)                      /*!< Priority of interrupt 31 */

/******************  Bit definition for SCB_CPUID register  *******************/
#define SCB_CPUID_REVISION                  (0x0000000FU)                      /*!< Implementation defined revision number */
#define SCB_CPUID_PARTNO                    (0x0000FFF0U)                      /*!< Number of processor within serie */
#define SCB_CPUID_Constant                  (0x000F0000U)                      /*!< Reads as 0x0F */
#define SCB_CPUID_VARIANT                   (0x00F00000U)                      /*!< Implementation defined variant number */
#define SCB_CPUID_IMPLEMENTER               (0xFF000000U)                      /*!< Implementer code. ARM is 0x41 */

/*******************  Bit definition for SCB_ICSR register  *******************/
#define SCB_ICSR_VECTACTIVE                 (0x000001FFU)                      /*!< Active ISR number field */
#define SCB_ICSR_RETTOBASE                  (0x00000800U)                      /*!< All active exceptions minus the IPSR_current_exception yields the empty set */
#define SCB_ICSR_VECTPENDING                (0x003FF000U)                      /*!< Pending ISR number field */
#define SCB_ICSR_ISRPENDING                 (0x00400000U)                      /*!< Interrupt pending flag */
#define SCB_ICSR_ISRPREEMPT                 (0x00800000U)                      /*!< It indicates that a pending interrupt becomes active in the next running cycle */
#define SCB_ICSR_PENDSTCLR                  (0x02000000U)                      /*!< Clear pending SysTick bit */
#define SCB_ICSR_PENDSTSET                  (0x04000000U)                      /*!< Set pending SysTick bit */
#define SCB_ICSR_PENDSVCLR                  (0x08000000U)                      /*!< Clear pending pendSV bit */
#define SCB_ICSR_PENDSVSET                  (0x10000000U)                      /*!< Set pending pendSV bit */
#define SCB_ICSR_NMIPENDSET                 (0x80000000U)                      /*!< Set pending NMI bit */

/*******************  Bit definition for SCB_VTOR register  *******************/
#define SCB_VTOR_TBLOFF                     (0x1FFFFF80U)                      /*!< Vector table base offset field */
#define SCB_VTOR_TBLBASE                    (0x20000000U)                      /*!< Table base in code(0) or RAM(1) */

/*!<*****************  Bit definition for SCB_AIRCR register  *******************/
#define SCB_AIRCR_VECTRESET                 (0x00000001U)                      /*!< System Reset bit */
#define SCB_AIRCR_VECTCLRACTIVE             (0x00000002U)                      /*!< Clear active vector bit */
#define SCB_AIRCR_SYSRESETREQ               (0x00000004U)                      /*!< Requests chip control logic to generate a reset */

#define SCB_AIRCR_PRIGROUP                  (0x00000700U)                      /*!< PRIGROUP[2:0] bits (Priority group) */
#define SCB_AIRCR_PRIGROUP_0                (0x00000100U)                      /*!< Bit 0 */
#define SCB_AIRCR_PRIGROUP_1                (0x00000200U)                      /*!< Bit 1 */
#define SCB_AIRCR_PRIGROUP_2                (0x00000400U)                      /*!< Bit 2  */

/* prority group configuration */
#define SCB_AIRCR_PRIGROUP0                 (0x00000000U)                      /*!< Priority group=0 (7 bits of pre-emption priority, 1 bit of subpriority) */
#define SCB_AIRCR_PRIGROUP1                 (0x00000100U)                      /*!< Priority group=1 (6 bits of pre-emption priority, 2 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP2                 (0x00000200U)                      /*!< Priority group=2 (5 bits of pre-emption priority, 3 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP3                 (0x00000300U)                      /*!< Priority group=3 (4 bits of pre-emption priority, 4 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP4                 (0x00000400U)                      /*!< Priority group=4 (3 bits of pre-emption priority, 5 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP5                 (0x00000500U)                      /*!< Priority group=5 (2 bits of pre-emption priority, 6 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP6                 (0x00000600U)                      /*!< Priority group=6 (1 bit of pre-emption priority, 7 bits of subpriority) */
#define SCB_AIRCR_PRIGROUP7                 (0x00000700U)                      /*!< Priority group=7 (no pre-emption priority, 8 bits of subpriority) */

#define SCB_AIRCR_ENDIANESS                 (0x00008000U)                      /*!< Data endianness bit */
#define SCB_AIRCR_VECTKEY                   (0xFFFF0000U)                      /*!< Register key (VECTKEY) - Reads as 0xFA05 (VECTKEYSTAT) */

/*******************  Bit definition for SCB_SCR register  ********************/
#define SCB_SCR_SLEEPONEXIT                 (0x00000002U)                      /*!< Sleep on exit bit */
#define SCB_SCR_SLEEPDEEP                   (0x00000004U)                      /*!< Sleep deep bit */
#define SCB_SCR_SEVONPEND                   (0x00000010U)                      /*!< Wake up from WFE */

/********************  Bit definition for SCB_CCR register  *******************/
#define SCB_CCR_NONBASETHRDENA              (0x00000001U)                      /*!< Thread mode can be entered from any level in Handler mode by controlled return value */
#define SCB_CCR_USERSETMPEND                (0x00000002U)                      /*!< Enables user code to write the Software Trigger Interrupt register to trigger (pend) a Main exception */
#define SCB_CCR_UNALIGN_TRP                 (0x00000008U)                      /*!< Trap for unaligned access */
#define SCB_CCR_DIV_0_TRP                   (0x00000010U)                      /*!< Trap on Divide by 0 */
#define SCB_CCR_BFHFNMIGN                   (0x00000100U)                      /*!< Handlers running at priority -1 and -2 */
#define SCB_CCR_STKALIGN                    (0x00000200U)                      /*!< On exception entry, the SP used prior to the exception is adjusted to be 8-byte aligned */

/*******************  Bit definition for SCB_SHPR register ********************/
#define SCB_SHPR_PRI_N_Pos                  (0U)                               
#define SCB_SHPR_PRI_N_Msk                  (0xFFU << SCB_SHPR_PRI_N_Pos)      /*!< 0x000000FF */
#define SCB_SHPR_PRI_N                      SCB_SHPR_PRI_N_Msk                 /*!< Priority of system handler 4,8, and 12. Mem Manage, reserved and Debug Monitor */
#define SCB_SHPR_PRI_N1_Pos                 (8U)                               
#define SCB_SHPR_PRI_N1_Msk                 (0xFFU << SCB_SHPR_PRI_N1_Pos)     /*!< 0x0000FF00 */
#define SCB_SHPR_PRI_N1                     SCB_SHPR_PRI_N1_Msk                /*!< Priority of system handler 5,9, and 13. Bus Fault, reserved and reserved */
#define SCB_SHPR_PRI_N2_Pos                 (16U)                              
#define SCB_SHPR_PRI_N2_Msk                 (0xFFU << SCB_SHPR_PRI_N2_Pos)     /*!< 0x00FF0000 */
#define SCB_SHPR_PRI_N2                     SCB_SHPR_PRI_N2_Msk                /*!< Priority of system handler 6,10, and 14. Usage Fault, reserved and PendSV */
#define SCB_SHPR_PRI_N3_Pos                 (24U)                              
#define SCB_SHPR_PRI_N3_Msk                 (0xFFU << SCB_SHPR_PRI_N3_Pos)     /*!< 0xFF000000 */
#define SCB_SHPR_PRI_N3                     SCB_SHPR_PRI_N3_Msk                /*!< Priority of system handler 7,11, and 15. Reserved, SVCall and SysTick */

/******************  Bit definition for SCB_SHCSR register  *******************/
#define SCB_SHCSR_MEMFAULTACT               (0x00000001U)                      /*!< MemManage is active */
#define SCB_SHCSR_BUSFAULTACT               (0x00000002U)                      /*!< BusFault is active */
#define SCB_SHCSR_USGFAULTACT               (0x00000008U)                      /*!< UsageFault is active */
#define SCB_SHCSR_SVCALLACT                 (0x00000080U)                      /*!< SVCall is active */
#define SCB_SHCSR_MONITORACT                (0x00000100U)                      /*!< Monitor is active */
#define SCB_SHCSR_PENDSVACT                 (0x00000400U)                      /*!< PendSV is active */
#define SCB_SHCSR_SYSTICKACT                (0x00000800U)                      /*!< SysTick is active */
#define SCB_SHCSR_USGFAULTPENDED            (0x00001000U)                      /*!< Usage Fault is pended */
#define SCB_SHCSR_MEMFAULTPENDED            (0x00002000U)                      /*!< MemManage is pended */
#define SCB_SHCSR_BUSFAULTPENDED            (0x00004000U)                      /*!< Bus Fault is pended */
#define SCB_SHCSR_SVCALLPENDED              (0x00008000U)                      /*!< SVCall is pended */
#define SCB_SHCSR_MEMFAULTENA               (0x00010000U)                      /*!< MemManage enable */
#define SCB_SHCSR_BUSFAULTENA               (0x00020000U)                      /*!< Bus Fault enable */
#define SCB_SHCSR_USGFAULTENA               (0x00040000U)                      /*!< UsageFault enable */

/*******************  Bit definition for SCB_CFSR register  *******************/
/*!< MFSR */
#define SCB_CFSR_IACCVIOL_Pos              (SCB_SHCSR_MEMFAULTACT_Pos + 0U)    /*!< SCB CFSR (MMFSR): IACCVIOL Position */
#define SCB_CFSR_IACCVIOL_Msk              (1UL /*<< SCB_CFSR_IACCVIOL_Pos*/)  /*!< SCB CFSR (MMFSR): IACCVIOL Mask */
#define SCB_CFSR_IACCVIOL                   SCB_CFSR_IACCVIOL_Msk              /*!< Instruction access violation */
#define SCB_CFSR_DACCVIOL_Pos              (SCB_SHCSR_MEMFAULTACT_Pos + 1U)    /*!< SCB CFSR (MMFSR): DACCVIOL Position */
#define SCB_CFSR_DACCVIOL_Msk              (1UL << SCB_CFSR_DACCVIOL_Pos)      /*!< SCB CFSR (MMFSR): DACCVIOL Mask */
#define SCB_CFSR_DACCVIOL                   SCB_CFSR_DACCVIOL_Msk              /*!< Data access violation */
#define SCB_CFSR_MUNSTKERR_Pos             (SCB_SHCSR_MEMFAULTACT_Pos + 3U)    /*!< SCB CFSR (MMFSR): MUNSTKERR Position */
#define SCB_CFSR_MUNSTKERR_Msk             (1UL << SCB_CFSR_MUNSTKERR_Pos)     /*!< SCB CFSR (MMFSR): MUNSTKERR Mask */
#define SCB_CFSR_MUNSTKERR                  SCB_CFSR_MUNSTKERR_Msk             /*!< Unstacking error */
#define SCB_CFSR_MSTKERR_Pos               (SCB_SHCSR_MEMFAULTACT_Pos + 4U)    /*!< SCB CFSR (MMFSR): MSTKERR Position */
#define SCB_CFSR_MSTKERR_Msk               (1UL << SCB_CFSR_MSTKERR_Pos)       /*!< SCB CFSR (MMFSR): MSTKERR Mask */
#define SCB_CFSR_MSTKERR                    SCB_CFSR_MSTKERR_Msk               /*!< Stacking error */
#define SCB_CFSR_MMARVALID_Pos             (SCB_SHCSR_MEMFAULTACT_Pos + 7U)    /*!< SCB CFSR (MMFSR): MMARVALID Position */
#define SCB_CFSR_MMARVALID_Msk             (1UL << SCB_CFSR_MMARVALID_Pos)     /*!< SCB CFSR (MMFSR): MMARVALID Mask */
#define SCB_CFSR_MMARVALID                  SCB_CFSR_MMARVALID_Msk             /*!< Memory Manage Address Register address valid flag */
/*!< BFSR */
#define SCB_CFSR_IBUSERR_Pos              (SCB_CFSR_BUSFAULTSR_Pos + 0U)       /*!< SCB CFSR (BFSR): IBUSERR Position */
#define SCB_CFSR_IBUSERR_Msk              (1UL << SCB_CFSR_IBUSERR_Pos)        /*!< SCB CFSR (BFSR): IBUSERR Mask */
#define SCB_CFSR_IBUSERR                    SCB_CFSR_IBUSERR_Msk               /*!< Instruction bus error flag */
#define SCB_CFSR_PRECISERR_Pos            (SCB_CFSR_BUSFAULTSR_Pos + 1U)       /*!< SCB CFSR (BFSR): PRECISERR Position */
#define SCB_CFSR_PRECISERR_Msk            (1UL << SCB_CFSR_PRECISERR_Pos)      /*!< SCB CFSR (BFSR): PRECISERR Mask */
#define SCB_CFSR_PRECISERR                  SCB_CFSR_PRECISERR_Msk             /*!< Precise data bus error */
#define SCB_CFSR_IMPRECISERR_Pos          (SCB_CFSR_BUSFAULTSR_Pos + 2U)       /*!< SCB CFSR (BFSR): IMPRECISERR Position */
#define SCB_CFSR_IMPRECISERR_Msk          (1UL << SCB_CFSR_IMPRECISERR_Pos)    /*!< SCB CFSR (BFSR): IMPRECISERR Mask */
#define SCB_CFSR_IMPRECISERR                SCB_CFSR_IMPRECISERR_Msk           /*!< Imprecise data bus error */
#define SCB_CFSR_UNSTKERR_Pos             (SCB_CFSR_BUSFAULTSR_Pos + 3U)       /*!< SCB CFSR (BFSR): UNSTKERR Position */
#define SCB_CFSR_UNSTKERR_Msk             (1UL << SCB_CFSR_UNSTKERR_Pos)       /*!< SCB CFSR (BFSR): UNSTKERR Mask */
#define SCB_CFSR_UNSTKERR                   SCB_CFSR_UNSTKERR_Msk              /*!< Unstacking error */
#define SCB_CFSR_STKERR_Pos               (SCB_CFSR_BUSFAULTSR_Pos + 4U)       /*!< SCB CFSR (BFSR): STKERR Position */
#define SCB_CFSR_STKERR_Msk               (1UL << SCB_CFSR_STKERR_Pos)         /*!< SCB CFSR (BFSR): STKERR Mask */
#define SCB_CFSR_STKERR                     SCB_CFSR_STKERR_Msk                /*!< Stacking error */
#define SCB_CFSR_BFARVALID_Pos            (SCB_CFSR_BUSFAULTSR_Pos + 7U)       /*!< SCB CFSR (BFSR): BFARVALID Position */
#define SCB_CFSR_BFARVALID_Msk            (1UL << SCB_CFSR_BFARVALID_Pos)      /*!< SCB CFSR (BFSR): BFARVALID Mask */
#define SCB_CFSR_BFARVALID                  SCB_CFSR_BFARVALID_Msk             /*!< Bus Fault Address Register address valid flag */
/*!< UFSR */
#define SCB_CFSR_UNDEFINSTR_Pos           (SCB_CFSR_USGFAULTSR_Pos + 0U)       /*!< SCB CFSR (UFSR): UNDEFINSTR Position */
#define SCB_CFSR_UNDEFINSTR_Msk           (1UL << SCB_CFSR_UNDEFINSTR_Pos)     /*!< SCB CFSR (UFSR): UNDEFINSTR Mask */
#define SCB_CFSR_UNDEFINSTR                 SCB_CFSR_UNDEFINSTR_Msk            /*!< The processor attempt to excecute an undefined instruction */
#define SCB_CFSR_INVSTATE_Pos             (SCB_CFSR_USGFAULTSR_Pos + 1U)       /*!< SCB CFSR (UFSR): INVSTATE Position */
#define SCB_CFSR_INVSTATE_Msk             (1UL << SCB_CFSR_INVSTATE_Pos)       /*!< SCB CFSR (UFSR): INVSTATE Mask */
#define SCB_CFSR_INVSTATE                   SCB_CFSR_INVSTATE_Msk              /*!< Invalid combination of EPSR and instruction */
#define SCB_CFSR_INVPC_Pos                (SCB_CFSR_USGFAULTSR_Pos + 2U)       /*!< SCB CFSR (UFSR): INVPC Position */
#define SCB_CFSR_INVPC_Msk                (1UL << SCB_CFSR_INVPC_Pos)          /*!< SCB CFSR (UFSR): INVPC Mask */
#define SCB_CFSR_INVPC                      SCB_CFSR_INVPC_Msk                 /*!< Attempt to load EXC_RETURN into pc illegally */
#define SCB_CFSR_NOCP_Pos                 (SCB_CFSR_USGFAULTSR_Pos + 3U)       /*!< SCB CFSR (UFSR): NOCP Position */
#define SCB_CFSR_NOCP_Msk                 (1UL << SCB_CFSR_NOCP_Pos)           /*!< SCB CFSR (UFSR): NOCP Mask */
#define SCB_CFSR_NOCP                       SCB_CFSR_NOCP_Msk                  /*!< Attempt to use a coprocessor instruction */
#define SCB_CFSR_UNALIGNED_Pos            (SCB_CFSR_USGFAULTSR_Pos + 8U)       /*!< SCB CFSR (UFSR): UNALIGNED Position */
#define SCB_CFSR_UNALIGNED_Msk            (1UL << SCB_CFSR_UNALIGNED_Pos)      /*!< SCB CFSR (UFSR): UNALIGNED Mask */
#define SCB_CFSR_UNALIGNED                  SCB_CFSR_UNALIGNED_Msk             /*!< Fault occurs when there is an attempt to make an unaligned memory access */
#define SCB_CFSR_DIVBYZERO_Pos            (SCB_CFSR_USGFAULTSR_Pos + 9U)       /*!< SCB CFSR (UFSR): DIVBYZERO Position */
#define SCB_CFSR_DIVBYZERO_Msk            (1UL << SCB_CFSR_DIVBYZERO_Pos)      /*!< SCB CFSR (UFSR): DIVBYZERO Mask */
#define SCB_CFSR_DIVBYZERO                  SCB_CFSR_DIVBYZERO_Msk             /*!< Fault occurs when SDIV or DIV instruction is used with a divisor of 0 */

/*******************  Bit definition for SCB_HFSR register  *******************/
#define SCB_HFSR_VECTTBL                    (0x00000002U)                      /*!< Fault occures because of vector table read on exception processing */
#define SCB_HFSR_FORCED                     (0x40000000U)                      /*!< Hard Fault activated when a configurable Fault was received and cannot activate */
#define SCB_HFSR_DEBUGEVT                   (0x80000000U)                      /*!< Fault related to debug */

/*******************  Bit definition for SCB_DFSR register  *******************/
#define SCB_DFSR_HALTED                     (0x00000001U)                      /*!< Halt request flag */
#define SCB_DFSR_BKPT                       (0x00000002U)                      /*!< BKPT flag */
#define SCB_DFSR_DWTTRAP                    (0x00000004U)                      /*!< Data Watchpoint and Trace (DWT) flag */
#define SCB_DFSR_VCATCH                     (0x00000008U)                      /*!< Vector catch flag */
#define SCB_DFSR_EXTERNAL                   (0x00000010U)                      /*!< External debug request flag */

/*******************  Bit definition for SCB_MMFAR register  ******************/
#define SCB_MMFAR_ADDRESS_Pos               (0U)                               
#define SCB_MMFAR_ADDRESS_Msk               (0xFFFFFFFFU << SCB_MMFAR_ADDRESS_Pos) /*!< 0xFFFFFFFF */
#define SCB_MMFAR_ADDRESS                   SCB_MMFAR_ADDRESS_Msk              /*!< Mem Manage fault address field */

/*******************  Bit definition for SCB_BFAR register  *******************/
#define SCB_BFAR_ADDRESS_Pos                (0U)                               
#define SCB_BFAR_ADDRESS_Msk                (0xFFFFFFFFU << SCB_BFAR_ADDRESS_Pos) /*!< 0xFFFFFFFF */
#define SCB_BFAR_ADDRESS                    SCB_BFAR_ADDRESS_Msk               /*!< Bus fault address field */

/*******************  Bit definition for SCB_afsr register  *******************/
#define SCB_AFSR_IMPDEF_Pos                 (0U)                               
#define SCB_AFSR_IMPDEF_Msk                 (0xFFFFFFFFU << SCB_AFSR_IMPDEF_Pos) /*!< 0xFFFFFFFF */
#define SCB_AFSR_IMPDEF                     SCB_AFSR_IMPDEF_Msk                /*!< Implementation defined */
/**
  * @}
  */

 /**
  * @}
  */ 
/** @addtogroup Exported_macro
  * @{
  */
      
/****************************** ADC Instances *********************************/
#define IS_ADC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == ADC1)

#define IS_ADC_COMMON_INSTANCE(INSTANCE) ((INSTANCE) == ADC1_COMMON)

/******************************** COMP Instances ******************************/
#define IS_COMP_ALL_INSTANCE(INSTANCE) (((INSTANCE) == COMP1) || \
                                        ((INSTANCE) == COMP2))

#define IS_COMP_COMMON_INSTANCE(COMMON_INSTANCE) ((COMMON_INSTANCE) == COMP12_COMMON)

/****************************** CRC Instances *********************************/
#define IS_CRC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == CRC)

/****************************** DAC Instances *********************************/
#define IS_DAC_ALL_INSTANCE(INSTANCE) ((INSTANCE) == DAC)

/****************************** DMA Instances *********************************/
#define IS_DMA_ALL_INSTANCE(INSTANCE) (((INSTANCE) == DMA1_Channel1) || \
                                       ((INSTANCE) == DMA1_Channel2) || \
                                       ((INSTANCE) == DMA1_Channel3) || \
                                       ((INSTANCE) == DMA1_Channel4) || \
                                       ((INSTANCE) == DMA1_Channel5) || \
                                       ((INSTANCE) == DMA1_Channel6) || \
                                       ((INSTANCE) == DMA1_Channel7) || \
                                       ((INSTANCE) == DMA2_Channel1) || \
                                       ((INSTANCE) == DMA2_Channel2) || \
                                       ((INSTANCE) == DMA2_Channel3) || \
                                       ((INSTANCE) == DMA2_Channel4) || \
                                       ((INSTANCE) == DMA2_Channel5))

/******************************* GPIO Instances *******************************/
#define IS_GPIO_ALL_INSTANCE(INSTANCE) (((INSTANCE) == GPIOA) || \
                                        ((INSTANCE) == GPIOB) || \
                                        ((INSTANCE) == GPIOC) || \
                                        ((INSTANCE) == GPIOD) || \
                                        ((INSTANCE) == GPIOE) || \
                                        ((INSTANCE) == GPIOF) || \
                                        ((INSTANCE) == GPIOG) || \
                                        ((INSTANCE) == GPIOH))

/**************************** GPIO Alternate Function Instances ***************/
#define IS_GPIO_AF_INSTANCE(INSTANCE) IS_GPIO_ALL_INSTANCE(INSTANCE)

/**************************** GPIO Lock Instances *****************************/
/* On L1, all GPIO Bank support the Lock mechanism */
#define IS_GPIO_LOCK_INSTANCE(INSTANCE) IS_GPIO_ALL_INSTANCE(INSTANCE)

/******************************** I2C Instances *******************************/
#define IS_I2C_ALL_INSTANCE(INSTANCE) (((INSTANCE) == I2C1) || \
                                       ((INSTANCE) == I2C2))

/****************************** SMBUS Instances *******************************/
#define IS_SMBUS_ALL_INSTANCE(INSTANCE) IS_I2C_ALL_INSTANCE(INSTANCE)

/******************************** I2S Instances *******************************/
#define IS_I2S_ALL_INSTANCE(INSTANCE) (((INSTANCE) == SPI2) || \
                                       ((INSTANCE) == SPI3))
/****************************** IWDG Instances ********************************/
#define IS_IWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == IWDG)

/****************************** OPAMP Instances *******************************/
#define IS_OPAMP_ALL_INSTANCE(INSTANCE) (((INSTANCE) == OPAMP1) || \
                                         ((INSTANCE) == OPAMP2) || \
                                         ((INSTANCE) == OPAMP3))

#define IS_OPAMP_COMMON_INSTANCE(COMMON_INSTANCE) ((COMMON_INSTANCE) == OPAMP123_COMMON)

/****************************** RTC Instances *********************************/
#define IS_RTC_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == RTC)

/****************************** SDIO Instances *********************************/
#define IS_SDIO_ALL_INSTANCE(INSTANCE) ((INSTANCE) == SDIO)

/******************************** SPI Instances *******************************/
#define IS_SPI_ALL_INSTANCE(INSTANCE) (((INSTANCE) == SPI1) || \
                                       ((INSTANCE) == SPI2) || \
                                       ((INSTANCE) == SPI3))

/****************************** TIM Instances *********************************/ 
#define IS_TIM_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)    || \
                                   ((INSTANCE) == TIM3)    || \
                                   ((INSTANCE) == TIM4)    || \
                                   ((INSTANCE) == TIM5)    || \
                                   ((INSTANCE) == TIM6)    || \
                                   ((INSTANCE) == TIM7)    || \
                                   ((INSTANCE) == TIM9)    || \
                                   ((INSTANCE) == TIM10)   || \
                                   ((INSTANCE) == TIM11))

#define IS_TIM_CC1_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5)  || \
                                       ((INSTANCE) == TIM9)  || \
                                       ((INSTANCE) == TIM10) || \
                                       ((INSTANCE) == TIM11))

#define IS_TIM_CC2_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5)  || \
                                       ((INSTANCE) == TIM9))

#define IS_TIM_CC3_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5))

#define IS_TIM_CC4_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5))

#define IS_TIM_CLOCKSOURCE_ETRMODE1_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                        ((INSTANCE) == TIM3)  || \
                                                        ((INSTANCE) == TIM4)  || \
                                                        ((INSTANCE) == TIM5)  || \
                                                        ((INSTANCE) == TIM9))

#define IS_TIM_CLOCKSOURCE_ETRMODE2_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                        ((INSTANCE) == TIM3)  || \
                                                        ((INSTANCE) == TIM4)  || \
                                                        ((INSTANCE) == TIM5)  || \
                                                        ((INSTANCE) == TIM9)  || \
                                                        ((INSTANCE) == TIM10) || \
                                                        ((INSTANCE) == TIM11))

#define IS_TIM_CLOCKSOURCE_TIX_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                   ((INSTANCE) == TIM3)  || \
                                                   ((INSTANCE) == TIM4)  || \
                                                   ((INSTANCE) == TIM5)  || \
                                                   ((INSTANCE) == TIM9))

#define IS_TIM_CLOCKSOURCE_ITRX_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                    ((INSTANCE) == TIM3)  || \
                                                    ((INSTANCE) == TIM4)  || \
                                                    ((INSTANCE) == TIM5)  || \
                                                    ((INSTANCE) == TIM9))

#define IS_TIM_OCXREF_CLEAR_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                ((INSTANCE) == TIM3)  || \
                                                ((INSTANCE) == TIM4))

#define IS_TIM_XOR_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5))
                                       
#define IS_TIM_ETR_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                       ((INSTANCE) == TIM3)  || \
                                       ((INSTANCE) == TIM4)  || \
                                       ((INSTANCE) == TIM5))


#define IS_TIM_MASTER_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                          ((INSTANCE) == TIM3)  || \
                                          ((INSTANCE) == TIM4)  || \
                                          ((INSTANCE) == TIM5)  || \
                                          ((INSTANCE) == TIM6)  || \
                                          ((INSTANCE) == TIM7)  || \
                                          ((INSTANCE) == TIM9))

#define IS_TIM_SLAVE_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                         ((INSTANCE) == TIM3)  || \
                                         ((INSTANCE) == TIM4)  || \
                                         ((INSTANCE) == TIM5)  || \
                                         ((INSTANCE) == TIM9))

#define IS_TIM_32B_COUNTER_INSTANCE(INSTANCE) ((INSTANCE) == TIM5)

#define IS_TIM_DMABURST_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                            ((INSTANCE) == TIM3)  || \
                                            ((INSTANCE) == TIM4)  || \
                                            ((INSTANCE) == TIM5))

#define IS_TIM_CCX_INSTANCE(INSTANCE, CHANNEL) \
    ((((INSTANCE) == TIM2) &&                   \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
    ||                                         \
    (((INSTANCE) == TIM3) &&                   \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
    ||                                         \
    (((INSTANCE) == TIM4) &&                   \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
    ||                                         \
    (((INSTANCE) == TIM5) &&                   \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2) ||          \
      ((CHANNEL) == TIM_CHANNEL_3) ||          \
      ((CHANNEL) == TIM_CHANNEL_4)))           \
    ||                                         \
    (((INSTANCE) == TIM9) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1) ||          \
      ((CHANNEL) == TIM_CHANNEL_2)))           \
    ||                                         \
    (((INSTANCE) == TIM10) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1)))           \
    ||                                         \
    (((INSTANCE) == TIM11) &&                  \
     (((CHANNEL) == TIM_CHANNEL_1))))

#define IS_TIM_CLOCK_DIVISION_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                  ((INSTANCE) == TIM3)  || \
                                                  ((INSTANCE) == TIM4)  || \
                                                  ((INSTANCE) == TIM5)  || \
                                                  ((INSTANCE) == TIM9)  || \
                                                  ((INSTANCE) == TIM10) || \
                                                  ((INSTANCE) == TIM11))

#define IS_TIM_DMA_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)    || \
                                       ((INSTANCE) == TIM3)    || \
                                       ((INSTANCE) == TIM4)    || \
                                       ((INSTANCE) == TIM5)    || \
                                       ((INSTANCE) == TIM6)    || \
                                       ((INSTANCE) == TIM7))
    
#define IS_TIM_DMA_CC_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                          ((INSTANCE) == TIM3)  || \
                                          ((INSTANCE) == TIM4)  || \
                                          ((INSTANCE) == TIM5))

#define IS_TIM_COUNTER_MODE_SELECT_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)    || \
                                                       ((INSTANCE) == TIM3)    || \
                                                       ((INSTANCE) == TIM4)    || \
                                                       ((INSTANCE) == TIM5)    || \
                                                       ((INSTANCE) == TIM9))

#define IS_TIM_ENCODER_INTERFACE_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)  || \
                                                     ((INSTANCE) == TIM3)  || \
                                                     ((INSTANCE) == TIM4)  || \
                                                     ((INSTANCE) == TIM5)  || \
                                                     ((INSTANCE) == TIM9))

#define IS_TIM_REMAP_INSTANCE(INSTANCE) (((INSTANCE) == TIM2)    || \
                                         ((INSTANCE) == TIM3)    || \
                                         ((INSTANCE) == TIM9)    || \
                                         ((INSTANCE) == TIM10)   || \
                                         ((INSTANCE) == TIM11))

/******************** USART Instances : Synchronous mode **********************/                                           
#define IS_USART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                     ((INSTANCE) == USART2) || \
                                     ((INSTANCE) == USART3))

/******************** UART Instances : Asynchronous mode **********************/
#define IS_UART_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2) || \
                                    ((INSTANCE) == USART3) || \
                                    ((INSTANCE) == UART4)  || \
                                    ((INSTANCE) == UART5))

/******************** UART Instances : Half-Duplex mode **********************/
#define IS_UART_HALFDUPLEX_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                                 ((INSTANCE) == USART2) || \
                                                 ((INSTANCE) == USART3) || \
                                                 ((INSTANCE) == UART4)  || \
                                                 ((INSTANCE) == UART5))                                       

/******************** UART Instances : LIN mode **********************/
#define IS_UART_LIN_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                          ((INSTANCE) == USART2) || \
                                          ((INSTANCE) == USART3) || \
                                          ((INSTANCE) == UART4)  || \
                                          ((INSTANCE) == UART5)) 

/****************** UART Instances : Hardware Flow control ********************/                                    
#define IS_UART_HWFLOW_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                           ((INSTANCE) == USART2) || \
                                           ((INSTANCE) == USART3))

/********************* UART Instances : Smard card mode ***********************/
#define IS_SMARTCARD_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                         ((INSTANCE) == USART2) || \
                                         ((INSTANCE) == USART3))

/*********************** UART Instances : IRDA mode ***************************/
#define IS_IRDA_INSTANCE(INSTANCE) (((INSTANCE) == USART1) || \
                                    ((INSTANCE) == USART2) || \
                                    ((INSTANCE) == USART3) || \
                                    ((INSTANCE) == UART4)  || \
                                    ((INSTANCE) == UART5))

/***************** UART Instances : Multi-Processor mode **********************/
#define IS_UART_MULTIPROCESSOR_INSTANCE(INSTANCE)   (((INSTANCE) == USART1) || \
                                                     ((INSTANCE) == USART2) || \
                                                     ((INSTANCE) == USART3) || \
                                                     ((INSTANCE) == UART4)  || \
                                                     ((INSTANCE) == UART5)) 

/****************************** WWDG Instances ********************************/
#define IS_WWDG_ALL_INSTANCE(INSTANCE)  ((INSTANCE) == WWDG)


/****************************** LCD Instances ********************************/
#define IS_LCD_ALL_INSTANCE(INSTANCE) ((INSTANCE) == LCD)

/****************************** USB Instances ********************************/
#define IS_USB_ALL_INSTANCE(INSTANCE) ((INSTANCE) == USB)

/**
  * @}
  */

/******************************************************************************/
/*  For a painless codes migration between the STM32L1xx device product       */
/*  lines, the aliases defined below are put in place to overcome the         */
/*  differences in the interrupt handlers and IRQn definitions.               */
/*  No need to update developed interrupt code when moving across             */ 
/*  product lines within the same STM32L1 Family                              */
/******************************************************************************/

/* Aliases for __IRQn */

/* Aliases for __IRQHandler */

/**
  * @}
  */

/**
  * @}
  */

#ifdef __cplusplus
}
#endif /* __cplusplus */

#endif /* __STM32L152xD_H */



/************************ (C) COPYRIGHT STMicroelectronics *****END OF FILE****/
