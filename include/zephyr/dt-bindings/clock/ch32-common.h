/*
 * Copyright (c) 2025 Fiona Behrens
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#ifndef ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_CLOCKS_H__
#define ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_CLOCKS_H__

#define CH32_CLOCK_CONFIG(bus, bit) (((CH32_CLOCK_CONFIG_BUS_##bus) << 5) | (bit))
#define CH32_CLOCK_CONFIG_BUS(sys)  (((sys) >> 5) & 0xFF)
#define CH32_CLOCK_CONFIG_BIT(sys)  ((sys) & 0x1F)

#define CH32_CLKID_CLK_SYS 0
#define CH32_CLKID_CLK_PLL 1
#define CH32_CLKID_CLK_HB  2

#endif /* ZEPHYR_INCLUDE_DT_BINDINGS_CLOCK_CH32_CLOCKS_H__ */
