/*
 * Copyright (c) 2016 Linaro Limited.
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
 * @file SoC configuration macros for the ARM LTD Beetle SoC.
 *
 */

#ifndef _ARM_BEETLE_SOC_H_
#define _ARM_BEETLE_SOC_H_

#include "soc_irq.h"

/*
 * The bit definitions below are used to enable/disable the following
 * peripheral configurations:
 * - Clocks in active state
 * - Clocks in sleep state
 * - Clocks in deep sleep state
 * - Wake up sources
 */

/* Beetle SoC AHB Devices */
#define _BEETLE_GPIO0       (1 << 0)
#define _BEETLE_GPIO1       (1 << 1)
#define _BEETLE_GPIO2       (1 << 2)
#define _BEETLE_GPIO3       (1 << 3)

/* Beetle SoC APB Devices */
#define _BEETLE_TIMER0      (1 << 0)
#define _BEETLE_TIMER1      (1 << 1)
#define _BEETLE_DUALTIMER0  (1 << 2)
#define _BEETLE_UART0       (1 << 4)
#define _BEETLE_UART1       (1 << 5)
#define _BEETLE_I2C0        (1 << 7)
#define _BEETLE_WDOG        (1 << 8)
#define _BEETLE_QSPI        (1 << 11)
#define _BEETLE_SPI0        (1 << 12)
#define _BEETLE_SPI1        (1 << 13)
#define _BEETLE_I2C1        (1 << 14)
#define _BEETLE_TRNG        (1 << 15)

/*
 * Address space definitions
 */

/* Beetle SoC Address space definition */
#define _BEETLE_APB_BASE    0x40000000
#define _BEETLE_AHB_BASE    0x40010000

/* Beetle SoC AHB peripherals */
#define _BEETLE_GPIO0_BASE      (_BEETLE_AHB_BASE + 0x0000)
#define _BEETLE_GPIO1_BASE      (_BEETLE_AHB_BASE + 0x1000)
#define _BEETLE_GPIO2_BASE      (_BEETLE_AHB_BASE + 0x2000)
#define _BEETLE_GPIO3_BASE      (_BEETLE_AHB_BASE + 0x3000)
#define _BEETLE_SYSCON_BASE     (_BEETLE_AHB_BASE + 0xF000)

/* Beetle SoC APB peripherals */
#define _BEETLE_UART0_BASE	(_BEETLE_APB_BASE + 0x4000)
#define _BEETLE_UART1_BASE	(_BEETLE_APB_BASE + 0x5000)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>

#include "soc_pins.h"
#include "soc_registers.h"

/* System Control Register (SYSCON) */
#define __BEETLE_SYSCON ((volatile struct syscon *)_BEETLE_SYSCON_BASE)

/* CMSDK AHB General Purpose Input/Output (GPIO) */
#define CMSDK_AHB_GPIO0 _BEETLE_GPIO0_BASE
#define CMSDK_AHB_GPIO1 _BEETLE_GPIO1_BASE
#define CMSDK_AHB_GPIO2 _BEETLE_GPIO2_BASE
#define CMSDK_AHB_GPIO3 _BEETLE_GPIO3_BASE

/* CMSDK APB Universal Asynchronous Receiver-Transmitter (UART) */
#define CMSDK_APB_UART0 _BEETLE_UART0_BASE
#define CMSDK_APB_UART1 _BEETLE_UART1_BASE

#endif /* !_ASMLANGUAGE */

#endif /* _ARM_BEETLE_SOC_H_ */
