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

#endif /* __SOC_H_ */
