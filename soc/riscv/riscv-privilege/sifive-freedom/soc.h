/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the SiFive Freedom processor
 */

#ifndef __RISCV_SIFIVE_FREEDOM_SOC_H_
#define __RISCV_SIFIVE_FREEDOM_SOC_H_

#include <soc_common.h>

#if defined(CONFIG_SOC_RISCV_SIFIVE_FREEDOM)
/* PINMUX MAX PINS */
#define SIFIVE_PINMUX_PINS            32

/* Clock controller. */
#define PRCI_BASE_ADDR               0x10008000

#elif defined(CONFIG_SOC_RISCV_SIFIVE_FU540) || defined(CONFIG_SOC_RISCV_SIFIVE_FU740)

/* Clock controller. */
#define PRCI_BASE_ADDR               0x10000000

/* PINMUX MAX PINS */
#define SIFIVE_PINMUX_PINS           16

#endif

#if defined(CONFIG_SOC_RISCV_SIFIVE_FREEDOM) || defined(CONFIG_SOC_RISCV_SIFIVE_FU540)

/*
 * On FE310 and FU540, peripherals such as SPI, UART, I2C and PWM are clocked
 * by TLCLK, which is derived from CORECLK.
 */
#define SIFIVE_TLCLK_BASE_FREQUENCY \
	DT_PROP_BY_PHANDLE_IDX(DT_NODELABEL(tlclk), clocks, 0, clock_frequency)
#define SIFIVE_TLCLK_DIVIDER DT_PROP(DT_NODELABEL(tlclk), clock_div)
#define SIFIVE_PERIPHERAL_CLOCK_FREQUENCY \
	(SIFIVE_TLCLK_BASE_FREQUENCY / SIFIVE_TLCLK_DIVIDER)

#elif defined(CONFIG_SOC_RISCV_SIFIVE_FU740)

/* On FU740, peripherals are clocked by PCLK. */
#define SIFIVE_PERIPHERAL_CLOCK_FREQUENCY \
	DT_PROP(DT_NODELABEL(pclk), clock_frequency)

#endif

/* Timer configuration */
#define RISCV_MTIME_BASE             0x0200BFF8
#define RISCV_MTIMECMP_BASE          0x02004000

#endif /* __RISCV_SIFIVE_FREEDOM_SOC_H_ */
