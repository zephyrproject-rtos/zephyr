/*
 * Copyright (c) 2016 Linaro Limited.
 *
 * SPDX-License-Identifier: Apache-2.0
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
#define _BEETLE_TIMER0_BASE	(_BEETLE_APB_BASE + 0x0000)
#define _BEETLE_TIMER1_BASE	(_BEETLE_APB_BASE + 0x1000)
#define _BEETLE_DTIMER_BASE	(_BEETLE_APB_BASE + 0x2000)
#define _BEETLE_FCACHE_BASE	(_BEETLE_APB_BASE + 0x3000)
#define _BEETLE_UART0_BASE	(_BEETLE_APB_BASE + 0x4000)
#define _BEETLE_UART1_BASE	(_BEETLE_APB_BASE + 0x5000)
#define _BEETLE_RTC_BASE	(_BEETLE_APB_BASE + 0x6000)
#define _BEETLE_I2C0_BASE	(_BEETLE_APB_BASE + 0x7000)
#define _BEETLE_WDOG_BASE	(_BEETLE_APB_BASE + 0x8000)
#define _BEETLE_QSPI_BASE	(_BEETLE_APB_BASE + 0xB000)
#define _BEETLE_SPI0_BASE	(_BEETLE_APB_BASE + 0xC000)
#define _BEETLE_SPI1_BASE	(_BEETLE_APB_BASE + 0xD000)
#define _BEETLE_I2C1_BASE	(_BEETLE_APB_BASE + 0xE000)
#define _BEETLE_TRNG_BASE	(_BEETLE_APB_BASE + 0xF000)

#ifndef _ASMLANGUAGE

#include <device.h>
#include <misc/util.h>

#include "soc_pins.h"
#include "soc_power.h"
#include "soc_registers.h"
#include "soc_pll.h"

/* System Control Register (SYSCON) */
#define __BEETLE_SYSCON ((volatile struct syscon *)_BEETLE_SYSCON_BASE)

/* CMSDK AHB General Purpose Input/Output (GPIO) */
#define CMSDK_AHB_GPIO0 _BEETLE_GPIO0_BASE
#define CMSDK_AHB_GPIO1 _BEETLE_GPIO1_BASE
#define CMSDK_AHB_GPIO2 _BEETLE_GPIO2_BASE
#define CMSDK_AHB_GPIO3 _BEETLE_GPIO3_BASE

/* CMSDK APB Timers */
#define CMSDK_APB_TIMER0 _BEETLE_TIMER0_BASE
#define CMSDK_APB_TIMER1 _BEETLE_TIMER1_BASE

/* CMSDK APB Dual Timer */
#define CMSDK_APB_DTIMER _BEETLE_DTIMER_BASE

/* CMSDK APB Universal Asynchronous Receiver-Transmitter (UART) */
#define CMSDK_APB_UART0 _BEETLE_UART0_BASE
#define CMSDK_APB_UART1 _BEETLE_UART1_BASE

/* CMSDK APB Watchdog */
#define CMSDK_APB_WDOG _BEETLE_WDOG_BASE

#endif /* !_ASMLANGUAGE */

#endif /* _ARM_BEETLE_SOC_H_ */
