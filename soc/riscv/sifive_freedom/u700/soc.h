/*
 * Copyright (c) 2017 Jean-Paul Etienne <fractalclone@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

/**
 * @file SoC configuration macros for the SiFive Freedom processor
 */

#ifndef __RISCV_SIFIVE_FREEDOM_U700_SOC_H_
#define __RISCV_SIFIVE_FREEDOM_U700_SOC_H_

/* Clock controller. */
#define PRCI_BASE_ADDR               0x10000000

/* On FU740, peripherals are clocked by PCLK. */
#define SIFIVE_PERIPHERAL_CLOCK_FREQUENCY \
	DT_PROP(DT_NODELABEL(pclk), clock_frequency)

#endif /* __RISCV_SIFIVE_FREEDOM_U700_SOC_H_ */
