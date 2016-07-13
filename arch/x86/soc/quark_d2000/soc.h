/*
 * Copyright (c) 2015 Wind River Systems, Inc.
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
 * @brief board configuration macros for the Quark D2000
 * This header file is used to specify and describe board-level aspects for
 * the Quark D2000 Platform.
 */

#ifndef __SOC_H_
#define __SOC_H_

#include <stdint.h>
#include <misc/util.h>
#include <uart.h>
#include <drivers/ioapic.h>

#define INT_VEC_IRQ0  0x20 /* Vector number for IRQ0 */
#define FIXED_HARDWARE_IRQ_TO_VEC_MAPPING(x) (INT_VEC_IRQ0 + x)
#define IOAPIC_LO32_RTE_SUPPORTED_MASK (IOAPIC_INT_MASK | IOAPIC_TRIGGER_MASK)

/* Base Register */
#define SCSS_REGISTER_BASE              0xB0800000

/* Clock */
#define CLOCK_PERIPHERAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x18)
#define CLOCK_EXTERNAL_BASE_ADDR	(SCSS_REGISTER_BASE + 0x24)
#define CLOCK_SENSOR_BASE_ADDR		(SCSS_REGISTER_BASE + 0x28)
#define CLOCK_SYSTEM_CLOCK_CONTROL      (SCSS_REGISTER_BASE + 0x38)

#define INT_UNMASK_IA			(~0x00000001)


/*
 * Local APIC (LOAPIC) device information (Intel loapic)
 */
#define LOAPIC_IRQ_BASE			CONFIG_LOAPIC_TIMER_IRQ
#define LOAPIC_IRQ_COUNT		1
#define LOAPIC_LVT_REG_SPACING  	0x10

/*
 * PINMUX configuration settings
 */
#define PINMUX_BASE_ADDR		0xb0800900
#define PINMUX_NUM_PINS			25

#define UART_IRQ_FLAGS (IOAPIC_LEVEL | IOAPIC_HIGH)

#endif /* __SOC_H_ */
