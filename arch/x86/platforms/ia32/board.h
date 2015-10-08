/* board.h - board configuration macros for the ia32 platform */

/*
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1) Redistributions of source code must retain the above copyright notice,
 * this list of conditions and the following disclaimer.
 *
 * 2) Redistributions in binary form must reproduce the above copyright notice,
 * this list of conditions and the following disclaimer in the documentation
 * and/or other materials provided with the distribution.
 *
 * 3) Neither the name of Wind River Systems nor the names of its contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

/*
DESCRIPTION
This header file is used to specify and describe board-level aspects for
the 'ia32' platform.
 */

#ifndef __INCboardh
#define __INCboardh

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <drivers/rand32.h>
#endif

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#ifdef CONFIG_SERIAL_INTERRUPT_LEVEL
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#else
#define UART_IOAPIC_FLAGS (IOAPIC_LEVEL)
#endif
#else /* edge triggered interrupt */
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
/* generate interrupt on falling edge */
#define UART_IOAPIC_FLAGS (IOAPIC_LOW)
#else
/* generate interrupt on raising edge */
#define UART_IOAPIC_FLAGS (0)
#endif
#endif
#endif

#define INT_VEC_IRQ0 0x20 /* vector number for IRQ0 */

/* serial port (aka COM port) information */

#ifdef CONFIG_NS16550

#define COM1_BASE_ADRS 0x3f8
#define COM1_INT_LVL 0x04 /* COM1 connected to IRQ4 */
#define COM1_INT_VEC (INT_VEC_IRQ0 + COM1_INT_LVL)
#define COM1_INT_PRI 3
#define COM1_BAUD_RATE 115200

#define COM2_BASE_ADRS 0x2f8
#define COM2_INT_LVL 0x03 /* COM2 connected to IRQ3 */
#define COM2_INT_VEC (INT_VEC_IRQ0 + COM2_INT_LVL)
#define COM2_INT_PRI 3
#define COM2_BAUD_RATE 115200

#define UART_REG_ADDR_INTERVAL 1 /* address diff of adjacent regs. */
#define UART_XTAL_FREQ 1843200

/* uart configuration settings */

/* Generic definitions */
#define CONFIG_UART_BAUDRATE		COM1_BAUD_RATE
#define CONFIG_UART_PORT_0_REGS		COM1_BASE_ADRS
#define CONFIG_UART_PORT_0_IRQ		COM1_INT_LVL
#define CONFIG_UART_PORT_0_IRQ_PRIORITY	COM1_INT_PRI
#define CONFIG_UART_PORT_1_REGS		COM2_BASE_ADRS
#define CONFIG_UART_PORT_1_IRQ		COM2_INT_LVL
#define CONFIG_UART_PORT_1_IRQ_PRIORITY	COM2_INT_PRI

/* Simple UART definitions */
#define CONFIG_UART_SIMPLE_INDEX 1
#define CONFIG_UART_SIMPLE_BAUDRATE CONFIG_UART_BAUDRATE
#define CONFIG_UART_SIMPLE_IRQ COM2_INT_LVL
#define CONFIG_UART_SIMPLE_INT_PRI COM2_INT_PRI
#define CONFIG_UART_SIMPLE_FREQ UART_XTAL_FREQ

#ifndef _ASMLANGUAGE
extern struct device * const uart_devs[];
#endif

/* Console definitions */
#if defined(CONFIG_UART_CONSOLE)

#define CONFIG_UART_CONSOLE_IRQ		COM1_INT_LVL
#define CONFIG_UART_CONSOLE_INT_PRI	COM1_INT_PRI

#define UART_CONSOLE_DEV (uart_devs[CONFIG_UART_CONSOLE_INDEX])

#endif /* CONFIG_UART_CONSOLE */

/* Bluetooth UART definitions */
#if defined(CONFIG_BLUETOOTH_UART)

#define CONFIG_BLUETOOTH_UART_INDEX 1
#define CONFIG_BLUETOOTH_UART_IRQ COM2_INT_LVL
#define CONFIG_BLUETOOTH_UART_INT_PRI COM2_INT_PRI
#define CONFIG_BLUETOOTH_UART_FREQ UART_XTAL_FREQ
#define CONFIG_BLUETOOTH_UART_BAUDRATE CONFIG_UART_BAUDRATE

#define BT_UART_DEV (uart_devs[CONFIG_BLUETOOTH_UART_INDEX])

#endif /* CONFIG_BLUETOOTH_UART */

#endif /* CONFIG_NS16550 */

#ifndef _ASMLANGUAGE

extern void _SysIntVecProgram(unsigned int vector, unsigned int);

#endif /* !_ASMLANGUAGE */

#endif /* __INCboardh */
