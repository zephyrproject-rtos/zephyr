/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the SiFive Freedom processor
 */

#ifndef __RISCV_SIFIVE_FREEDOM_FE300_SOC_H_
#define __RISCV_SIFIVE_FREEDOM_FE300_SOC_H_

/*
 * On FE310 and FU540, peripherals such as SPI, UART, I2C and PWM are clocked
 * by TLCLK, which is derived from CORECLK.
 */
#define SIFIVE_TLCLK_BASE_FREQUENCY \
	DT_PROP_BY_PHANDLE_IDX(DT_NODELABEL(tlclk), clocks, 0, clock_frequency)
#define SIFIVE_TLCLK_DIVIDER DT_PROP(DT_NODELABEL(tlclk), clock_div)
#define SIFIVE_PERIPHERAL_CLOCK_FREQUENCY \
	(SIFIVE_TLCLK_BASE_FREQUENCY / SIFIVE_TLCLK_DIVIDER)

#endif /* __RISCV_SIFIVE_FREEDOM_FE300_SOC_H_ */
