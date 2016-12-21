/*
 * Copyright (c) 2014-2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief Board configuration macros for Quark SE Sensor Subsystem
 *
 * This header file is used to specify and describe board-level
 * aspects for the Quark SE Sensor Subsystem.
 */

#ifndef _BOARD__H_
#define _BOARD__H_

#include <misc/util.h>

/* default system clock */

#define SYSCLK_DEFAULT_IOSC_HZ MHZ(32)

/* address bases */

#define SCSS_REGISTER_BASE 0xB0800000 /*Sensor Subsystem Base*/

#define PERIPH_ADDR_BASE_ADC 0x80015000 /* ADC */

#define PERIPH_ADDR_BASE_CREG_MST0 0x80018000 /* CREG Master 0 */
#define PERIPH_ADDR_BASE_CREG_SLV0 0x80018080 /* CREG Slave 0 */
#define PERIPH_ADDR_BASE_CREG_SLV1 0x80018180 /* CREG Slave 1 */

#define PERIPH_ADDR_BASE_GPIO0 0x80017800 /* GPIO 0 */
#define PERIPH_ADDR_BASE_GPIO1 0x80017900 /* GPIO 1 */

#define PERIPH_ADDR_BASE_SPI_MST0 0x80010000 /* SPI Master 0 */
#define PERIPH_ADDR_BASE_SPI_MST1 0x80010100 /* SPI Master 1 */

/* IRQs */

/* The CPU-visible IRQ numbers change between the ARC and IA cores,
 * and QMSI itself has no easy way to pick the correct one, though it
 * does have the necessary information to do it ourselves (in the meantime).
 * This macro will be used by the shim drivers to get the IRQ number to
 * use, and it should always be called using the QM_IRQ_*_INT macro
 * provided by QMSI.
 */
#define IRQ_GET_NUMBER(_irq) _irq##_VECTOR

#define IRQ_TIMER0 16
#define IRQ_TIMER1 17
#define IRQ_I2C0_RX_AVAIL 18
#define IRQ_I2C0_TX_REQ 19
#define IRQ_I2C0_STOP_DET 20
#define IRQ_I2C0_ERR 21
#define IRQ_I2C1_RX_AVAIL 22
#define IRQ_I2C1_TX_REQ 23
#define IRQ_I2C1_STOP_DET 24
#define IRQ_I2C1_ERR 25
#define IRQ_SPI0_ERR_INT 30
#define IRQ_SPI0_RX_AVAIL 31
#define IRQ_SPI0_TX_REQ 32

#define IRQ_SPI1_ERR_INT 33
#define IRQ_SPI1_RX_AVAIL 34
#define IRQ_SPI1_TX_REQ 35

#define IRQ_ADC_ERR 18
#define IRQ_ADC_IRQ 19
#define IRQ_GPIO0_INTR 20
#define IRQ_GPIO1_INTR 21
#define IRQ_I2C_MST0_INTR 36
#define IRQ_I2C_MST1_INTR 37
#define IRQ_SPI_MST0_INTR 38
#define IRQ_SPI_MST1_INTR 39
#define IRQ_SPI_SLV_INTR 40
#define IRQ_UART0_INTR 41
#define IRQ_UART1_INTR 42
#define IRQ_I2S_INTR 43
#define IRQ_GPIO_INTR 44
#define IRQ_PWM_TIMER_INTR 45
#define IRQ_USB_INTR 46
#define IRQ_RTC_INTR 47
#define IRQ_WDOG_INTR 48
#define IRQ_DMA_CHAN0 49
#define IRQ_DMA_CHAN1 50
#define IRQ_DMA_CHAN2 51
#define IRQ_DMA_CHAN3 52
#define IRQ_DMA_CHAN4 53
#define IRQ_DMA_CHAN5 54
#define IRQ_DMA_CHAN6 55
#define IRQ_DMA_CHAN7 56
#define IRQ_MAILBOXES_INTR 57
#define IRQ_COMPARATORS_INTR 58
#define IRQ_SYS_PMU_INTR 59
#define IRQ_DMA_CHANS_ERR 60
#define IRQ_INT_SRAM_CTLR 61
#define IRQ_INT_FLASH0_CTLR 62
#define IRQ_INT_FLASH1_CTLR 63
#define IRQ_ALWAYS_ON_TMR 64
#define IRQ_ADC_PWR 65
#define IRQ_ADC_CALIB 66
#define IRQ_ALWAYS_ON_GPIO 67

#ifndef _ASMLANGUAGE

#include <misc/util.h>
#include <drivers/rand32.h>
#include <quark_se/shared_mem.h>

#define INT_ENABLE_ARC					~(0x00000001 << 8)
#define INT_ENABLE_ARC_BIT_POS				(8)

/*
 * I2C
 */
#define I2C_SS_0_ERR_VECTOR				22
#define I2C_SS_0_ERR_MASK				0x410
#define I2C_SS_0_RX_VECTOR				23
#define I2C_SS_0_RX_MASK				0x414
#define I2C_SS_0_TX_VECTOR				24
#define I2C_SS_0_TX_MASK				0x418
#define I2C_SS_0_STOP_VECTOR				25
#define I2C_SS_0_STOP_MASK				0x41C

#define I2C_SS_1_ERR_VECTOR				26
#define I2C_SS_1_ERR_MASK				0x420
#define I2C_SS_1_RX_VECTOR				27
#define I2C_SS_1_RX_MASK				0x424
#define I2C_SS_1_TX_VECTOR				28
#define I2C_SS_1_TX_MASK				0x428
#define I2C_SS_1_STOP_VECTOR				29
#define I2C_SS_1_STOP_MASK				0x42C

/*
 * GPIO
 */
#define GPIO_DW_IO_ACCESS

#define GPIO_DW_0_BASE_ADDR			0x80017800
#define GPIO_DW_0_IRQ				20
#define GPIO_DW_0_BITS				8
#define GPIO_DW_PORT_0_INT_MASK			(SCSS_REGISTER_BASE + 0x408)

#define GPIO_DW_1_BASE_ADDR			0x80017900
#define GPIO_DW_1_IRQ				21
#define GPIO_DW_1_BITS				8
#define GPIO_DW_PORT_1_INT_MASK			(SCSS_REGISTER_BASE + 0x40C)

#if defined(CONFIG_IOAPIC)
#define GPIO_DW_0_IRQ_FLAGS			(IOAPIC_EDGE | IOAPIC_HIGH)
#define GPIO_DW_1_IRQ_FLAGS			(IOAPIC_EDGE | IOAPIC_HIGH)
#endif

/*
 * UART
 */

#define UART_IRQ_FLAGS                                  0

#define UART_NS16550_PORT_0_BASE_ADDR			0xB0002000
#define UART_NS16550_PORT_0_IRQ				41
#define UART_NS16550_PORT_0_CLK_FREQ			SYSCLK_DEFAULT_IOSC_HZ
#define UART_NS16550_PORT_0_INT_MASK			0x460

#define UART_NS16550_PORT_1_BASE_ADDR			0xB0002400
#define UART_NS16550_PORT_1_IRQ				42
#define UART_NS16550_PORT_1_CLK_FREQ			SYSCLK_DEFAULT_IOSC_HZ
#define UART_NS16550_PORT_1_INT_MASK			0x464

/*
 * SPI
 */

#define SPI_DW_PORT_0_REGS				0x80010000
#define SPI_DW_PORT_1_REGS				0x80010100

#define SPI_DW_PORT_0_ERROR_INT_MASK			(SCSS_REGISTER_BASE + 0x430)
#define SPI_DW_PORT_0_RX_INT_MASK			(SCSS_REGISTER_BASE + 0x434)
#define SPI_DW_PORT_0_TX_INT_MASK			(SCSS_REGISTER_BASE + 0x438)

#define SPI_DW_PORT_1_ERROR_INT_MASK			(SCSS_REGISTER_BASE + 0x43C)
#define SPI_DW_PORT_1_RX_INT_MASK			(SCSS_REGISTER_BASE + 0x440)
#define SPI_DW_PORT_1_TX_INT_MASK			(SCSS_REGISTER_BASE + 0x444)

#define SPI_DW_IRQ_FLAGS				0

static inline void _quark_se_ss_ready(void)
{
	shared_data->flags |= ARC_READY;
}

#endif /* !_ASMLANGUAGE */

#endif /* _BOARD__H_ */
