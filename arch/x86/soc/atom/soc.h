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

/**
 * @file
 * @brief Board configuration macros for the ia32 platform
 *
 * This header file is used to specify and describe board-level aspects for
 * the 'ia32' platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <misc/util.h>

#ifndef _ASMLANGUAGE
#include <device.h>
#include <drivers/rand32.h>
#endif

#define INT_VEC_IRQ0 0x20 /* vector number for IRQ0 */

/*
 * UART
 */
#define UART_NS16550_ACCESS_IOPORT

#define UART_NS16550_PORT_0_BASE_ADDR		0x03F8
#define UART_NS16550_PORT_0_IRQ			4
#define UART_NS16550_PORT_0_CLK_FREQ		1843200

#define UART_NS16550_PORT_1_BASE_ADDR		0x02F8
#define UART_NS16550_PORT_1_IRQ			3
#define UART_NS16550_PORT_1_CLK_FREQ		1843200

#ifdef CONFIG_IOAPIC
#include <drivers/ioapic.h>
#define UART_IRQ_FLAGS				(IOAPIC_EDGE | IOAPIC_HIGH)
#endif /* CONFIG_IOAPIC */

#endif /* __SOC_H_ */
