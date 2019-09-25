/*
 * Copyright (c) 2018 Synopsys, Inc. All rights reserved.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for EM Starter kit board
 *
 * This header file is used to specify and describe board-level
 * aspects for the target.
 */

#ifndef _SOC__H_
#define _SOC__H_

#include <sys/util.h>

/* default system clock */
/* On the EM Starter Kit board, the peripheral bus clock frequency is 50Mhz */
#define SYSCLK_DEFAULT_IOSC_HZ			MHZ(50)

/* ARC EM Core IRQs */
#define IRQ_TIMER0				16
#define IRQ_TIMER1				17

#if defined(CONFIG_BOARD_EM_STARTERKIT_R23) && defined(CONFIG_SOC_EMSK_EM7D)
#define IRQ_SEC_TIMER0				20
#endif /* CONFIG_BOARD_EM_STARTERKIT_R23 && CONFIG_SOC_EMSK_EM7D */

#if defined(CONFIG_BOARD_EM_STARTERKIT_R23) && defined(CONFIG_SOC_EMSK_EM7D)
#define IRQ_CORE_DMA_COMPLETE			22
#define IRQ_CORE_DMA_ERROR			23
#else /* CONFIG_BOARD_EM_STARTERKIT_R23 && CONFIG_SOC_EMSK_EM7D */
#define IRQ_CORE_DMA_COMPLETE			20
#define IRQ_CORE_DMA_ERROR			21
#endif /* !(CONFIG_BOARD_EM_STARTERKIT_R23 && CONFIG_SOC_EMSK_EM7D) */


#ifndef _ASMLANGUAGE

#include <sys/util.h>
#include <random/rand32.h>

#define INT_ENABLE_ARC				~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS			(8)

/*
 * UARTs: UART0 & UART1 & UART2
 */
#define DT_UART_NS16550_PORT_0_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_1_IRQ_FLAGS	0 /* Default */
#define DT_UART_NS16550_PORT_2_IRQ_FLAGS	0 /* Default */

#ifndef CONFIG_HAS_DTS
/* I2C */
/* I2C_0 is on Pmod2 connector */
#define DT_I2C_0_BASE_ADDR			0xF0004000
#define DT_I2C_0_IRQ_FLAGS			0

/* I2C_1 is on Pmod4 connector */
#define DT_I2C_1_BASE_ADDR			0xF0005000
#define DT_I2C_1_IRQ_FLAGS			0


/* GPIO */
#define DT_GPIO_DW_0_BASE_ADDR			0xF0002000 /* GPIO 0 : PORTA */
#define DT_GPIO_DW_0_BITS				32
#define GPIO_DW_PORT_0_INT_MASK			0 /* n/a */
#define DT_GPIO_DW_0_IRQ_FLAGS			0 /* Defaults */

#define DT_GPIO_DW_1_BASE_ADDR			0xF000200C /* GPIO 1 : PORTB */
#define DT_GPIO_DW_1_BITS				9          /* 9 LEDs on board */
#define GPIO_DW_PORT_1_INT_MASK			0 /* n/a */

#define DT_GPIO_DW_2_BASE_ADDR			0xF0002018 /* GPIO 2 : PORTC */
#define DT_GPIO_DW_2_BITS				32
#define GPIO_DW_PORT_2_INT_MASK			0 /* n/a */

#define DT_GPIO_DW_3_BASE_ADDR			0xF0002024 /* GPIO 3 : PORTD */
#define DT_GPIO_DW_3_BITS				12
#define GPIO_DW_PORT_3_INT_MASK			0 /* n/a */

/* SPI */
#define DT_SPI_DW_IRQ_FLAGS			0

/*
 * SPI Chip Select Assignments on EM Starter Kit
 *
 *  CS0   Pmod6 - pin 1 - J6
 *  CS1   Pmod5 - pin 1 - J5 & Pmod 6 - pin 7 - J6
 *  CS2   Pmod6 - pin 8 - J6
 *  CS3   SDCard (onboard)
 *  CS4   Internal SPI Slave - loopback
 *  CS5   SPI-Flash (onboard)
 */

/**
 * Peripheral Interrupt Connection Configurations
 */
#ifdef CONFIG_BOARD_EM_STARTERKIT_R23
#define DT_GPIO_DW_0_IRQ				24
#define DT_I2C_0_IRQ			25
#define DT_I2C_1_IRQ			26
#else /* CONFIG_BOARD_EM_STARTERKIT_R23 */
#define DT_GPIO_DW_0_IRQ				22
#define DT_I2C_0_IRQ			23
#define DT_I2C_1_IRQ			24
#endif /* !CONFIG_BOARD_EM_STARTERKIT_R23 */

#define DT_GPIO_DW_1_IRQ				0	/* can't interrupt */
#define DT_GPIO_DW_2_IRQ				0	/* can't interrupt */
#define DT_GPIO_DW_3_IRQ				0	/* can't interrupt */

#endif /* CONFIG_HAS_DTS */

#endif /* !_ASMLANGUAGE */

#endif /* _SOC__H_ */
