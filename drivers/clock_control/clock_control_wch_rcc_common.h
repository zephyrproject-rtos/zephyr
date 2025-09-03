/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#ifndef ZEPHYR_DRIVERS_CLOCK_CONTROL_WCH_RCC_COMMON_H__
#define ZEPHYR_DRIVERS_CLOCK_CONTROL_WCH_RCC_COMMON_H__

#include <stdint.h>

#include <zephyr/devicetree.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/ch32-common.h>
#include <zephyr/arch/riscv/sys_io.h>

#include <hal_ch32fun.h>

#define CH32_CLK_SRC_IS(clk, src)                                                                  \
	DT_SAME_NODE(DT_CLOCKS_CTLR_BY_IDX(DT_INST_CLOCKS_CTLR_BY_NAME(0, clk), 0),                \
		     DT_INST_CLOCKS_CTLR_BY_NAME(0, src))

#define CH32_GET_CLK(name)  DT_INST_CLOCKS_CTLR_BY_NAME(0, name)
#define CH32_CLK_OKAY(name) DT_NODE_HAS_STATUS_OKAY(CH32_GET_CLK(name))

#define _CH32_CLKBIT_ENABLE(reg, on_bit, state_bit)                                                \
	do {                                                                                       \
		reg |= on_bit;                                                                     \
		while ((reg & state_bit) == 0) {                                                   \
		}                                                                                  \
	} while (0)

#define CH32_CLKBIT_ENABLE(reg, clk) _CH32_CLKBIT_ENABLE(reg, RCC_##clk##ON, RCC_##clk##RDY)

struct wch_clk_config {
	uint32_t source;
	uint16_t clk_div;
	uint16_t clk_mul;
};

static inline int clock_control_wch_common_clock_on(RCC_TypeDef *rcc_regs,
						    clock_control_subsys_t sys)
{
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&rcc_regs->AHBPCENR + CH32_CLOCK_CONFIG_BUS(id));
	uint32_t val = sys_read32(reg);

	val |= BIT(CH32_CLOCK_CONFIG_BIT(id));
	sys_write32(val, reg);

	return 0;
}

static inline int clock_control_wch_common_clock_off(RCC_TypeDef *rcc_regs,
						     clock_control_subsys_t sys)
{
	uint8_t id = (uintptr_t)sys;
	uint32_t reg = (uint32_t)(&rcc_regs->AHBPCENR + CH32_CLOCK_CONFIG_BUS(id));
	uint32_t val = sys_read32(reg);

	val &= ~BIT(CH32_CLOCK_CONFIG_BIT(id));
	sys_write32(val, reg);

	return 0;
}

#endif /* ZEPHYR_DRIVERS_CLOCK_CONTROL_WCH_RCC_COMMON_H__ */
