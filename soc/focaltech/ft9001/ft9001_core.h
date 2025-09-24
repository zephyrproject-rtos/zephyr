/*
 * Copyright (c) 2025, FocalTech Systems CO.,Ltd
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * FT9001 Core Definitions for Zephyr integration
 */

#ifndef __FT9001_CORE_H
#define __FT9001_CORE_H

#ifdef __cplusplus
extern "C" {
#endif

/** @addtogroup Configuration_section_for_CMSIS
 * @{
 */

/**
 * @brief Configuration of the Cortex-M4 Processor and Core Peripherals
 */
#define __CM4_REV              0x0001 /*!< Core revision r0p1 */
#define __MPU_PRESENT          1      /*!< FT9001 provides an MPU */
#define __NVIC_PRIO_BITS       3      /*!< FT9001 uses 3 Bits for the Priority Levels */
#define __Vendor_SysTickConfig 0      /*!< Set to 1 if different SysTick Config is used */
#define __FPU_PRESENT          1      /*!< FPU present */

/**
 * @}
 */

/** @addtogroup Peripheral_interrupt_number_definition
 * @{
 */

/**
 * @brief FT9001 Interrupt Number Definition
 */
typedef enum IRQn {
	/******  Cortex-M4 Processor Exceptions Numbers
	 * ****************************************************************/
	NonMaskableInt_IRQn = -14,   /*!< 2 Non Maskable Interrupt */
	HardFault_IRQn = -13,        /*!< 3 Cortex-M4 Hard Fault Interrupt */
	MemoryManagement_IRQn = -12, /*!< 4 Cortex-M4 Memory Management Interrupt */
	BusFault_IRQn = -11,         /*!< 5 Cortex-M4 Bus Fault Interrupt */
	UsageFault_IRQn = -10,       /*!< 6 Cortex-M4 Usage Fault Interrupt */
	SVCall_IRQn = -5,            /*!< 11 Cortex-M4 SV Call Interrupt */
	DebugMonitor_IRQn = -4,      /*!< 12 Cortex-M4 Debug Monitor Interrupt */
	PendSV_IRQn = -2,            /*!< 14 Cortex-M4 Pend SV Interrupt */
	SysTick_IRQn = -1,           /*!< 15 Cortex-M4 System Tick Interrupt */

	/******  FT9001 specific Interrupt Numbers
	 * ******************************************************/
	OTP_IRQn = 0,    /*!< OTP Interrupt */
	PMU_IRQn = 1,    /*!< PMU Interrupt */
	TC_IRQn = 2,     /*!< Timer Counter Interrupt */
	PIT1_IRQn = 3,   /*!< PIT1 interrupt */
	PIT2_IRQn = 4,   /*!< PIT2 Interrupt */
	EDMA0_IRQn = 5,  /*!< EDMA0 Interrupt */
	EDMA1_IRQn = 6,  /*!< EDMA1 Interrupt */
	DMA1_IRQn = 7,   /*!< DMA1 Interrupt */
	DMA2_IRQn = 8,   /*!< DMA2 Interrupt */
	TRNG_IRQn = 10,  /*!< TRNG Interrupt */
	UART1_IRQn = 32, /*!< UART1 Interrupt */
	UART2_IRQn = 33, /*!< UART2 Interrupt */
	UART3_IRQn = 61, /*!< UART3 Interrupt */
	/* Additional interrupts from original ft9001.h can be added here as needed */
} IRQn_Type;

/**
 * @}
 */

/* Include CMSIS Cortex-M4 Core Peripheral Access Layer */
#include <stdint.h>
#include "core_cm4.h"

/** @addtogroup Peripheral_memory_map
 * @{
 */
#define FT9001_ROM_BASE    0x00000000UL /*!< ROM (64K) base address */
#define FT9001_FLASH_BASE  0x10000000UL /*!< FLASH (2MB) base address */
#define FT9001_SRAM_BASE   0x20000000UL /*!< Internal SRAM base address */
#define FT9001_PERIPH_BASE 0x40000000UL /*!< Peripheral registers base address */

/* Compatibility aliases for existing code */
#define ROM_BASE     FT9001_ROM_BASE
#define FLASH_BASE   FT9001_FLASH_BASE
#define IN_SRAM_BASE FT9001_SRAM_BASE
#define REG_BASE     FT9001_PERIPH_BASE

/**
 * @}
 */

/** @addtogroup Peripheral_Registers_Structures
 * @{
 */

/**
 * @brief Debug MCU
 */
typedef struct {
	__IO uint32_t IDCODE; /*!< MCU device ID code */
	__IO uint32_t CR;     /*!< Debug MCU configuration register */
	__IO uint32_t APB1FZ; /*!< Debug MCU APB1 freeze register */
	__IO uint32_t APB2FZ; /*!< Debug MCU APB2 freeze register */
} DBGMCU_TypeDef;

/**
 * @}
 */

/** @addtogroup Peripheral_declaration
 * @{
 */
#define DBGMCU_BASE (FT9001_PERIPH_BASE + 0x00042000UL)
#define DBGMCU      ((DBGMCU_TypeDef *)DBGMCU_BASE)

/**
 * @}
 */

#ifdef __cplusplus
}
#endif

#endif /* __FT9001_CORE_H */
