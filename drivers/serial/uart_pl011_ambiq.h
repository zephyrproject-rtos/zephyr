/*
 * Copyright (c) 2023 Antmicro <www.antmicro.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_
#define ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_

#include <zephyr/device.h>
#include <zephyr/kernel.h>

#include "uart_pl011_registers.h"

#define PWRCTRL_MAX_WAIT_US 5

static inline void pl011_ambiq_enable_clk(const struct device *dev)
{
	get_uart(dev)->cr |= PL011_CR_AMBIQ_CLKEN;
}

static inline int pl011_ambiq_clk_set(const struct device *dev, uint32_t clk)
{
	uint8_t clksel;

	switch (clk) {
	case 3000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_3MHZ;
		break;
	case 6000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_6MHZ;
		break;
	case 12000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_12MHZ;
		break;
	case 24000000:
		clksel = PL011_CR_AMBIQ_CLKSEL_24MHZ;
		break;
	default:
		return -EINVAL;
	}

	get_uart(dev)->cr |= FIELD_PREP(PL011_CR_AMBIQ_CLKSEL, clksel);
	return 0;
}

static inline int clk_enable_ambiq_uart(const struct device *dev, uint32_t clk)
{
	pl011_ambiq_enable_clk(dev);
	return pl011_ambiq_clk_set(dev, clk);
}

/* Problem: writes to power configure register takes some time to take effective.
 * Solution: Check device's power status to ensure that register has taken effective.
 * Note: busy wait is not allowed to use here due to UART is initiated before timer starts.
 */

#define AMBIQ_UART_DEFINE(n)                                                                       \
	static int pwr_on_ambiq_uart_##n(void)                                                     \
	{                                                                                          \
		uint32_t addr = DT_REG_ADDR(DT_INST_PHANDLE(n, ambiq_pwrcfg)) +                    \
				DT_INST_PHA(n, ambiq_pwrcfg, offset);                              \
		uint32_t pwr_status_addr = addr + 4;                                               \
		sys_write32((sys_read32(addr) | DT_INST_PHA(n, ambiq_pwrcfg, mask)), addr);        \
		while ((sys_read32(pwr_status_addr) & DT_INST_PHA(n, ambiq_pwrcfg, mask)) !=       \
		       DT_INST_PHA(n, ambiq_pwrcfg, mask)) {                                       \
			arch_nop();                                                                \
		};                                                                                 \
		return 0;                                                                          \
	}                                                                                          \
	static inline int clk_enable_ambiq_uart_##n(const struct device *dev, uint32_t clk)        \
	{                                                                                          \
		return clk_enable_ambiq_uart(dev, clk);                                            \
	}

#endif /* ZEPHYR_DRIVERS_SERIAL_UART_PL011_AMBIQ_H_ */
