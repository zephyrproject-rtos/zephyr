/******************************************************************************
* @file     gd32vf103.h
* @brief    NMSIS Core Peripheral Access Layer Header File for GD32VF103 series
*
* @version  V1.00
* @date     4. Jan 2020
******************************************************************************/
/*
 * Copyright (c) 2019 Nuclei Limited. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __GD32VF103_H__
#define __GD32VF103_H__

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup gd32
 * @{
 */


/** @addtogroup gd32vf103
 * @{
 */


/** @addtogroup Configuration_of_NMSIS
 * @{
 */




/* =========================================================================================================================== */
/* ================                                Interrupt Number Definition                                ================ */
/* =========================================================================================================================== */

typedef enum IRQn {
	/* =======================================  Nuclei Core Specific Interrupt Numbers  ======================================== */

	Reserved0_IRQn  =   0,                          /*!<  Internal reserved */
	Reserved1_IRQn  =   1,                          /*!<  Internal reserved */
	Reserved2_IRQn  =   2,                          /*!<  Internal reserved */
	SysTimerSW_IRQn =   3,                          /*!<  System Timer SW interrupt */
	Reserved3_IRQn  =   4,                          /*!<  Internal reserved */
	Reserved4_IRQn  =   5,                          /*!<  Internal reserved */
	Reserved5_IRQn  =   6,                          /*!<  Internal reserved */
	SysTimer_IRQn   =   7,                          /*!<  System Timer Interrupt */
	Reserved6_IRQn  =   8,                          /*!<  Internal reserved */
	Reserved7_IRQn  =   9,                          /*!<  Internal reserved */
	Reserved8_IRQn  =  10,                          /*!<  Internal reserved */
	Reserved9_IRQn  =  11,                          /*!<  Internal reserved */
	Reserved10_IRQn =  12,                          /*!<  Internal reserved */
	Reserved11_IRQn =  13,                          /*!<  Internal reserved */
	Reserved12_IRQn =  14,                          /*!<  Internal reserved */
	Reserved13_IRQn =  15,                          /*!<  Internal reserved */
	Reserved14_IRQn =  16,                          /*!<  Internal reserved */
	BusError_IRQn   =  17,                          /*!<  Bus Error interrupt */
	PerfMon_IRQn    =  18,                          /*!<  Performance Monitor */

	/* ===========================================  GD32VF103 Specific Interrupt Numbers  ========================================= */
	/* ToDo: add here your device specific external interrupt numbers. 19~1023 is reserved number for user. Maxmum interrupt supported
	         could get from clicinfo.NUM_INTERRUPT. According the interrupt handlers defined in startup_Device.s
	         eg.: Interrupt for Timer#1       TIM1_IRQHandler   ->   TIM1_IRQn */
	/* interruput numbers */
	WWDGT_IRQn              = 19,           /*!< window watchDog timer interrupt                          */
	LVD_IRQn                = 20,           /*!< LVD through EXTI line detect interrupt                   */
	TAMPER_IRQn             = 21,           /*!< tamper through EXTI line detect                          */
	RTC_IRQn                = 22,           /*!< RTC alarm interrupt                                      */
	FMC_IRQn                = 23,           /*!< FMC interrupt                                            */
	RCU_CTC_IRQn            = 24,           /*!< RCU and CTC interrupt                                    */
	EXTI0_IRQn              = 25,           /*!< EXTI line 0 interrupts                                   */
	EXTI1_IRQn              = 26,           /*!< EXTI line 1 interrupts                                   */
	EXTI2_IRQn              = 27,           /*!< EXTI line 2 interrupts                                   */
	EXTI3_IRQn              = 28,           /*!< EXTI line 3 interrupts                                   */
	EXTI4_IRQn              = 29,           /*!< EXTI line 4 interrupts                                   */
	DMA0_Channel0_IRQn      = 30,           /*!< DMA0 channel0 interrupt                                  */
	DMA0_Channel1_IRQn      = 31,           /*!< DMA0 channel1 interrupt                                  */
	DMA0_Channel2_IRQn      = 32,           /*!< DMA0 channel2 interrupt                                  */
	DMA0_Channel3_IRQn      = 33,           /*!< DMA0 channel3 interrupt                                  */
	DMA0_Channel4_IRQn      = 34,           /*!< DMA0 channel4 interrupt                                  */
	DMA0_Channel5_IRQn      = 35,           /*!< DMA0 channel5 interrupt                                  */
	DMA0_Channel6_IRQn      = 36,           /*!< DMA0 channel6 interrupt                                  */
	ADC0_1_IRQn             = 37,           /*!< ADC0 and ADC1 interrupt                                  */
	CAN0_TX_IRQn            = 38,           /*!< CAN0 TX interrupts                                       */
	CAN0_RX0_IRQn           = 39,           /*!< CAN0 RX0 interrupts                                      */
	CAN0_RX1_IRQn           = 40,           /*!< CAN0 RX1 interrupts                                      */
	CAN0_EWMC_IRQn          = 41,           /*!< CAN0 EWMC interrupts                                     */
	EXTI5_9_IRQn            = 42,           /*!< EXTI[9:5] interrupts                                     */
	TIMER0_BRK_IRQn         = 43,           /*!< TIMER0 break interrupts                                  */
	TIMER0_UP_IRQn          = 44,           /*!< TIMER0 update interrupts                                 */
	TIMER0_TRG_CMT_IRQn     = 45,           /*!< TIMER0 trigger and commutation interrupts                */
	TIMER0_Channel_IRQn     = 46,           /*!< TIMER0 channel capture compare interrupts                */
	TIMER1_IRQn             = 47,           /*!< TIMER1 interrupt                                         */
	TIMER2_IRQn             = 48,           /*!< TIMER2 interrupt                                         */
	TIMER3_IRQn             = 49,           /*!< TIMER3 interrupts                                        */
	I2C0_EV_IRQn            = 50,           /*!< I2C0 event interrupt                                     */
	I2C0_ER_IRQn            = 51,           /*!< I2C0 error interrupt                                     */
	I2C1_EV_IRQn            = 52,           /*!< I2C1 event interrupt                                     */
	I2C1_ER_IRQn            = 53,           /*!< I2C1 error interrupt                                     */
	SPI0_IRQn               = 54,           /*!< SPI0 interrupt                                           */
	SPI1_IRQn               = 55,           /*!< SPI1 interrupt                                           */
	USART0_IRQn             = 56,           /*!< USART0 interrupt                                         */
	USART1_IRQn             = 57,           /*!< USART1 interrupt                                         */
	USART2_IRQn             = 58,           /*!< USART2 interrupt                                         */
	EXTI10_15_IRQn          = 59,           /*!< EXTI[15:10] interrupts                                   */
	RTC_ALARM_IRQn          = 60,           /*!< RTC alarm interrupt EXTI                                 */
	USBFS_WKUP_IRQn         = 61,           /*!< USBFS wakeup interrupt                                   */

	EXMC_IRQn               = 67,           /*!< EXMC global interrupt                                    */

	TIMER4_IRQn             = 69,           /*!< TIMER4 global interrupt                                  */
	SPI2_IRQn               = 70,           /*!< SPI2 global interrupt                                    */
	UART3_IRQn              = 71,           /*!< UART3 global interrupt                                   */
	UART4_IRQn              = 72,           /*!< UART4 global interrupt                                   */
	TIMER5_IRQn             = 73,           /*!< TIMER5 global interrupt                                  */
	TIMER6_IRQn             = 74,           /*!< TIMER6 global interrupt                                  */
	DMA1_Channel0_IRQn      = 75,           /*!< DMA1 channel0 global interrupt                           */
	DMA1_Channel1_IRQn      = 76,           /*!< DMA1 channel1 global interrupt                           */
	DMA1_Channel2_IRQn      = 77,           /*!< DMA1 channel2 global interrupt                           */
	DMA1_Channel3_IRQn      = 78,           /*!< DMA1 channel3 global interrupt                           */
	DMA1_Channel4_IRQn      = 79,           /*!< DMA1 channel3 global interrupt                           */

	CAN1_TX_IRQn            = 82,           /*!< CAN1 TX interrupt                                        */
	CAN1_RX0_IRQn           = 83,           /*!< CAN1 RX0 interrupt                                       */
	CAN1_RX1_IRQn           = 84,           /*!< CAN1 RX1 interrupt                                       */
	CAN1_EWMC_IRQn          = 85,           /*!< CAN1 EWMC interrupt                                      */
	USBFS_IRQn              = 86,           /*!< USBFS global interrupt                                   */

	SOC_INT_MAX,

} IRQn_Type;

/* =========================================================================================================================== */
/* ================                                  Exception Code Definition                                ================ */
/* =========================================================================================================================== */

typedef enum EXCn {
	/* =======================================  Nuclei N/NX Specific Exception Code  ======================================== */
	InsUnalign_EXCn         =   0,                  /*!<  Instruction address misaligned */
	InsAccFault_EXCn        =   1,                  /*!<  Instruction access fault */
	IlleIns_EXCn            =   2,                  /*!<  Illegal instruction */
	Break_EXCn              =   3,                  /*!<  Beakpoint */
	LdAddrUnalign_EXCn      =   4,                  /*!<  Load address misaligned */
	LdFault_EXCn            =   5,                  /*!<  Load access fault */
	StAddrUnalign_EXCn      =   6,                  /*!<  Store or AMO address misaligned */
	StAccessFault_EXCn      =   7,                  /*!<  Store or AMO access fault */
	UmodeEcall_EXCn         =   8,                  /*!<  Environment call from User mode */
	MmodeEcall_EXCn         =  11,                  /*!<  Environment call from Machine mode */
	NMI_EXCn                = 0xfff,                /*!<  NMI interrupt*/
} EXCn_Type;

/* =========================================================================================================================== */
/* ================                           Processor and Core Peripheral Section                           ================ */
/* =========================================================================================================================== */

/* ToDo: set the defines according your Device */
/* ToDo: define the correct core revision */
#define __NUCLEI_N_REV            0x0100    /*!< Core Revision r1p0 */

/* ToDo: define the correct core features for the nuclei_soc */
#define __ECLIC_PRESENT           1                     /*!< Set to 1 if ECLIC is present */
#define __ECLIC_BASEADDR          0xD2000000UL          /*!< Set to ECLIC baseaddr of your device */

#define __ECLIC_INTCTLBITS        4                     /*!< Set to 1 - 8, the number of hardware bits are actually implemented in the clicintctl registers. */
#define __ECLIC_INTNUM            86                    /*!< Set to 1 - 1005, the external interrupt number of ECLIC Unit */
#define __SYSTIMER_PRESENT        1                     /*!< Set to 1 if System Timer is present */
#define __SYSTIMER_BASEADDR       0xD1000000UL          /*!< Set to SysTimer baseaddr of your device */

/*!< Set to 0, 1, or 2, 0 not present, 1 single floating point unit present, 2 double floating point unit present */
#define __FPU_PRESENT             0

#define __DSP_PRESENT             0                     /*!< Set to 1 if DSP is present */
#define __PMP_PRESENT             1                     /*!< Set to 1 if PMP is present */
#define __PMP_ENTRY_NUM           8                     /*!< Set to 8 or 16, the number of PMP entries */
#define __ICACHE_PRESENT          0                     /*!< Set to 1 if I-Cache is present */
#define __DCACHE_PRESENT          0                     /*!< Set to 1 if D-Cache is present */
#define __Vendor_SysTickConfig    0                     /*!< Set to 1 if different SysTick Config is used */
#define __Vendor_EXCEPTION        0                     /*!< Set to 1 if vendor exception hander is present */

/** @} */ /* End of group Configuration_of_CMSIS */



#include <nmsis_core.h>                         /*!< Nuclei N/NX class processor and core peripherals */


/* ========================================  Start of section using anonymous unions  ======================================== */

/* system frequency define */
#define __IRC8M           (IRC8M_VALUE)                 /* internal 8 MHz RC oscillator frequency */
#define __HXTAL           (HXTAL_VALUE)                 /* high speed crystal oscillator frequency */
#define __SYS_OSC_CLK     (__IRC8M)                     /* main oscillator frequency */

#define __SYSTEM_CLOCK_108M_PLL_HXTAL           (uint32_t)(108000000)


#define RTC_FREQ LXTAL_VALUE
// The TIMER frequency is just the RTC frequency
#define SOC_TIMER_FREQ     ((uint32_t)SystemCoreClock / 4)  // LXTAL_VALUE units HZ


/* enum definitions */
typedef enum {
	DISABLE = 0,
	ENABLE  = !DISABLE
} EventStatus, ControlStatus;

typedef enum {
	FALSE   = 0,
	TRUE    = !FALSE
} BOOL;

typedef enum {
	RESET   = 0,
	SET     = 1,
	MAX     = 0X7FFFFFFF
} FlagStatus;

typedef enum {
	ERROR   = 0,
	SUCCESS = !ERROR
} ErrStatus;


/* =========================================================================================================================== */
/* ================                          Device Specific Peripheral Address Map                           ================ */
/* =========================================================================================================================== */

#include <devicetree.h>
#include <sys/util.h>

/* ToDo: add here your device peripherals base addresses
         following is an example for timer */
/** @addtogroup Device_Peripheral_peripheralAddr
 * @{
 */
/* main flash and SRAM memory map */
#define FLASH_BASE            DT_REG_ADDR(DT_NODELABEL(flash0))
#define SRAM_BASE             DT_REG_ADDR(DT_NODELABEL(sram0))
#define OB_BASE               ((uint32_t)0x1FFFF800U)           /*!< OB base address                  */
#define DBG_BASE              ((uint32_t)0xE0042000U)           /*!< DBG base address                 */
#define EXMC_BASE             ((uint32_t)0xA0000000U)           /*!< EXMC register base address       */

/* peripheral memory map */
#define APB1_BUS_BASE         ((uint32_t)0x40000000U)           /*!< apb1 base address                */
#define APB2_BUS_BASE         ((uint32_t)0x40010000U)           /*!< apb2 base address                */
#define AHB1_BUS_BASE         ((uint32_t)0x40018000U)           /*!< ahb1 base address                */
#define AHB3_BUS_BASE         ((uint32_t)0x60000000U)           /*!< ahb3 base address                */

/* advanced peripheral bus 1 memory map */
#define TIMER_BASE            (APB1_BUS_BASE + 0x00000000U)     /*!< TIMER base address               */
#define RTC_BASE              (APB1_BUS_BASE + 0x00002800U)     /*!< RTC base address                 */
#define WWDGT_BASE            (APB1_BUS_BASE + 0x00002C00U)     /*!< WWDGT base address               */
#define FWDGT_BASE            (APB1_BUS_BASE + 0x00003000U)     /*!< FWDGT base address               */
#define SPI_BASE              (APB1_BUS_BASE + 0x00003800U)     /*!< SPI base address                 */
#define USART_BASE            (APB1_BUS_BASE + 0x00004400U)     /*!< USART base address               */
#define I2C_BASE              (APB1_BUS_BASE + 0x00005400U)     /*!< I2C base address                 */
#define CAN_BASE              (APB1_BUS_BASE + 0x00006400U)     /*!< CAN base address                 */
#define BKP_BASE              (APB1_BUS_BASE + 0x00006C00U)     /*!< BKP base address                 */
#define PMU_BASE              (APB1_BUS_BASE + 0x00007000U)     /*!< PMU base address                 */
#define DAC_BASE              (APB1_BUS_BASE + 0x00007400U)     /*!< DAC base address                 */

/* advanced peripheral bus 2 memory map */
#define AFIO_BASE             DT_REG_ADDR(DT_NODELABEL(afio))
#define EXTI_BASE             DT_REG_ADDR(DT_NODELABEL(exti))
#define GPIO_BASE             DT_REG_ADDR(DT_NODELABEL(gpioa))
#define ADC_BASE              (APB2_BUS_BASE + 0x00002400U)  /*!< ADC base address                 */

/* advanced high performance bus 1 memory map */
#define DMA_BASE              (AHB1_BUS_BASE + 0x00008000U)     /*!< DMA base address                 */
#define RCU_BASE              DT_REG_ADDR(DT_NODELABEL(rcu))
#define FMC_BASE              (AHB1_BUS_BASE + 0x0000A000U)     /*!< FMC base address                 */
#define CRC_BASE              (AHB1_BUS_BASE + 0x0000B000U)     /*!< CRC base address                 */
#define USBFS_BASE            (AHB1_BUS_BASE + 0x0FFE8000U)     /*!< USBFS base address               */


/** @} */ /* End of group Device_Peripheral_peripheralAddr */


/* =========================================================================================================================== */
/* ================                                  Peripheral declaration                                   ================ */
/* =========================================================================================================================== */


/* ToDo: add here your device peripherals pointer definitions
         following is an example for timer */
/** @addtogroup Device_Peripheral_declaration
 * @{
 */
/* bit operations */
#define REG32(addr)                  (*(volatile uint32_t *)(uint32_t)(addr))
#define REG16(addr)                  (*(volatile uint16_t *)(uint32_t)(addr))
#define REG8(addr)                   (*(volatile uint8_t *)(uint32_t)(addr))
#define BITS(start, end)             ((0xFFFFFFFFUL << (start)) & (0xFFFFFFFFUL >> (31U - (uint32_t)(end))))
#define GET_BITS(regval, start, end) (((regval) & BITS((start), (end))) >> (start))

// Interrupt Numbers
#define SOC_ECLIC_NUM_INTERRUPTS    86
#define SOC_ECLIC_INT_GPIO_BASE     19


// Interrupt Handler Definitions
#define SOC_MTIMER_HANDLER          eclic_mtip_handler
#define SOC_SOFTINT_HANDLER         eclic_msip_handler

#define NUM_GPIO 32

/** @} */ /* End of group gd32vf103_soc */

/** @} */ /* End of group gd32vf103 */

#ifdef __cplusplus
}
#endif

#endif  /* __GD32VF103_SOC_H__ */
