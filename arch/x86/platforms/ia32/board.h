/* board.h - board configuration macros for the ia32 platform */

/*
 * Copyright (c) 2010-2015, Wind River Systems, Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
 * DESCRIPTION
 * This header file is used to specify and describe board-level aspects for
 * the 'ia32' platform.
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
#define UART_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_LOW)
#else
#define UART_IRQ_FLAGS (IOAPIC_LEVEL)
#endif
#else /* edge triggered interrupt */
#ifdef CONFIG_SERIAL_INTERRUPT_LOW
/* generate interrupt on falling edge */
#define UART_IRQ_FLAGS (IOAPIC_LOW)
#else
/* generate interrupt on raising edge */
#define UART_IRQ_FLAGS (0)
#endif
#endif
#endif

#define INT_VEC_IRQ0 0x20 /* vector number for IRQ0 */

/* uart configuration settings */

#ifdef CONFIG_UART_NS16550

/* Pipe UART definitions */
#define CONFIG_UART_PIPE_INDEX 1
#define CONFIG_UART_PIPE_BAUDRATE	CONFIG_UART_NS16550_PORT_1_BAUD_RATE
#define CONFIG_UART_PIPE_IRQ		CONFIG_UART_NS16550_PORT_1_IRQ
#define CONFIG_UART_PIPE_INT_PRI	CONFIG_UART_NS16550_PORT_1_IRQ_PRI
#define CONFIG_UART_PIPE_FREQ		CONFIG_UART_NS16550_PORT_1_CLK_FREQ

#ifndef _ASMLANGUAGE
extern struct device * const uart_devs[];
#endif

/* Console definitions */
#if defined(CONFIG_UART_CONSOLE)

#define CONFIG_UART_CONSOLE_IRQ		CONFIG_UART_NS16550_PORT_0_IRQ
#define CONFIG_UART_CONSOLE_INT_PRI	CONFIG_UART_NS16550_PORT_0_IRQ_PRI

#define UART_CONSOLE_DEV (uart_devs[CONFIG_UART_CONSOLE_INDEX])

#endif /* CONFIG_UART_CONSOLE */

/* Bluetooth UART definitions */
#if defined(CONFIG_BLUETOOTH_UART)

#define CONFIG_BLUETOOTH_UART_INDEX 1
#define CONFIG_BLUETOOTH_UART_IRQ	CONFIG_UART_NS16550_PORT_1_IRQ
#define CONFIG_BLUETOOTH_UART_INT_PRI	CONFIG_UART_NS16550_PORT_1_IRQ_PRI

#define BT_UART_DEV (uart_devs[CONFIG_BLUETOOTH_UART_INDEX])

#endif /* CONFIG_BLUETOOTH_UART */

#endif /* CONFIG_UART_NS16550 */

#endif /* __INCboardh */
