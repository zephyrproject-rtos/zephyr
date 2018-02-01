/*
 * Copyright (c) 2015 Intel Corporation
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief board configuration macros for the Quark SE
 * This header file is used to specify and describe board-level aspects for
 * the Quark SE Platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <zephyr/types.h>
#include <misc/util.h>
#include <uart.h>

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#endif

#define INT_VEC_IRQ0  0x20	/* Vector number for IRQ0 */

#ifndef _ASMLANGUAGE

/* Base Register */
#define SCSS_REGISTER_BASE		0xB0800000

#define INT_UNMASK_IA  		        (~0x00000001)

/* Clock */
#define CLOCK_PERIPHERAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x18)
#define CLOCK_EXTERNAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x24)
#define CLOCK_SENSOR_BASE_ADDR		(SCSS_REGISTER_BASE + 0x28)
#define CLOCK_SYSTEM_CLOCK_CONTROL	(SCSS_REGISTER_BASE + SCSS_CCU_SYS_CLK_CTL)

/* ARC INIT */
#define RESET_VECTOR                   	CONFIG_SS_RESET_VECTOR
#define SCSS_SS_CFG                    	0x0600
#define SCSS_SS_STS                    	0x0604
#define ARC_HALT_INT_REDIR             	(1 << 26)
#define ARC_HALT_REQ_A                 	(1 << 25)
#define ARC_RUN_REQ_A                  	(1 << 24)
#define ARC_RUN				(ARC_HALT_INT_REDIR | ARC_RUN_REQ_A)
#define ARC_HALT			(ARC_HALT_INT_REDIR | ARC_HALT_REQ_A)

/* The CPU-visible IRQ numbers change between the ARC and IA cores,
 * and QMSI itself has no easy way to pick the correct one, though it
 * does have the necessary information to do it ourselves (in the meantime).
 * This macro will be used by the shim drivers to get the IRQ number to
 * use, and it should always be called using the QM_IRQ_*_INT macro
 * provided by QMSI.
 */
#define IRQ_GET_NUMBER(_irq) _irq

/*
 * PINMUX configuration settings
 */
#define PINMUX_BASE_ADDR		0xb0800900

#define UART_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)

#ifdef CONFIG_SPI_DW

#define SPI_DW_PORT_0_REGS		0xB0001000
#define SPI_DW_PORT_0_IRQ		2
#define SPI_DW_PORT_0_INT_MASK		(SCSS_REGISTER_BASE + 0x454)

#define SPI_DW_PORT_1_REGS		0xB0001400
#define SPI_DW_PORT_1_IRQ		3
#define SPI_DW_PORT_1_INT_MASK		(SCSS_REGISTER_BASE + 0x458)

#define SPI_DW_PORT_2_REGS		0xB0001800
#define SPI_DW_PORT_2_IRQ		4
#define SPI_DW_PORT_2_INT_MASK		(SCSS_REGISTER_BASE + 0x45C)

#define SPI_DW_IRQ_FLAGS		(IOAPIC_LEVEL | IOAPIC_HIGH)

#endif /* CONFIG_SPI_DW */

#ifdef CONFIG_USB_DW

#define	USB_DW_BASE			QM_USB_0_BASE
#define	USB_DW_IRQ			QM_IRQ_USB_0_INT

#endif

#endif /*  _ASMLANGUAGE */

#ifdef CONFIG_ARC_INIT
int _arc_init(struct device *arg);
#endif /* CONFIG_ARC_INIT */

#endif /* __SOC_H_ */
