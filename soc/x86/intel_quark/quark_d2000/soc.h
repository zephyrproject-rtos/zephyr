/*
 * Copyright (c) 2015 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @brief board configuration macros for the Quark D2000
 * This header file is used to specify and describe board-level aspects for
 * the Quark D2000 Platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <zephyr/types.h>
#include <misc/util.h>
#include <uart.h>
#include <drivers/ioapic.h>

/* Base Register */
#define SCSS_REGISTER_BASE              0xB0800000

/* Clock */
#define CLOCK_PERIPHERAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x18)
#define CLOCK_EXTERNAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x24)
#define CLOCK_SENSOR_BASE_ADDR		(SCSS_REGISTER_BASE + 0x28)
#define CLOCK_SYSTEM_CLOCK_CONTROL      (SCSS_REGISTER_BASE + 0x38)

#define INT_UNMASK_IA			(~0x00000001)

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
#define PINMUX_NUM_PINS			25

#define UART_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)

#ifdef CONFIG_SPI_DW

#define SPI_DW_PORT_0_REGS		0xB0001000
#define SPI_DW_PORT_0_IRQ		2
#define SPI_DW_PORT_0_INT_MASK		(SCSS_REGISTER_BASE + 0x454)

#define SPI_DW_IRQ_FLAGS		(IOAPIC_LEVEL | IOAPIC_HIGH)

#endif

#endif /* __SOC_H_ */
