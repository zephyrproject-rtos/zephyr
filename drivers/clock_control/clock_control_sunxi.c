/*
 * Copyright (c) 2026 Khai Do
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT allwinner_sun50i_h618_ccu

#include <zephyr/drivers/clock_control.h>
#include <zephyr/devicetree.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/util.h>
#include <zephyr/dt-bindings/clock/sun50i-h618-ccu.h>
#include <zephyr/dt-bindings/reset/sun50i-h618-ccu.h>

LOG_MODULE_REGISTER(clock_control_sunxi, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

/* CCU Register Offsets */
#define CCU_PLL_PERIPH0_CTRL_REG 0x020
#define CCU_APB2_CFG_REG         0x524

/* PLL_PERIPH0 Clock Factors */
#define CCU_PLL_PERIPH0_EN_BIT  31
#define CCU_PLL_PERIPH0_N_MASK  GENMASK(15, 8)
#define CCU_PLL_PERIPH0_M_MASK  GENMASK(1, 1)
#define CCU_PLL_PERIPH0_P_MASK  GENMASK(0, 0)

/* APB2 Clock Factors */
#define CCU_APB2_CLK_SRC_MASK       GENMASK(25, 24)
#define CCU_APB2_CLK_FACTOR_N_MASK  GENMASK(9, 8)
#define CCU_APB2_CLK_FACTOR_M_MASK  GENMASK(1, 0)

/* UART Bus Gates & Resets */
#define CCU_UART_BGR_REG 0x90c

#define SUN50I_H618_OSC24M_RATE  DT_PROP(DT_NODELABEL(osc24m), clock_frequency)
#define SUN50I_H618_OSC32K_RATE  DT_PROP(DT_NODELABEL(osc32k), clock_frequency)

struct ccu_gate {
	uint16_t offset;
	uint8_t bit;
	uint8_t rst_bit;
};

#define SUNXI_CCU_NO_RST 0xFF

#define SUNXI_CCU_GATE_RST(clk_id, reg_offset, bit_idx, r_idx) \
	[clk_id] = { .offset = reg_offset, .bit = bit_idx, .rst_bit = r_idx }

static const struct ccu_gate sun50i_h618_gates[] = {
	/* UART Bus Gates */
	SUNXI_CCU_GATE_RST(CLK_BUS_UART0, CCU_UART_BGR_REG, 0, 16),
	SUNXI_CCU_GATE_RST(CLK_BUS_UART1, CCU_UART_BGR_REG, 1, 17),
	SUNXI_CCU_GATE_RST(CLK_BUS_UART2, CCU_UART_BGR_REG, 2, 18),
	SUNXI_CCU_GATE_RST(CLK_BUS_UART3, CCU_UART_BGR_REG, 3, 19),
	SUNXI_CCU_GATE_RST(CLK_BUS_UART4, CCU_UART_BGR_REG, 4, 20),
	SUNXI_CCU_GATE_RST(CLK_BUS_UART5, CCU_UART_BGR_REG, 5, 21),
};


static int sunxi_ccu_on(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)(uintptr_t)sub_system;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	const struct ccu_gate *gate;

	if (clk_id >= ARRAY_SIZE(sun50i_h618_gates)) {
		return -EINVAL;
	}

	gate = &sun50i_h618_gates[clk_id];
	if (gate->offset == 0) {
		return -ENOTSUP;
	}

	sys_set_bit(base + gate->offset, gate->bit);

	if (gate->rst_bit != SUNXI_CCU_NO_RST) {
		sys_set_bit(base + gate->offset, gate->rst_bit);
	}

	return 0;
}

static int sunxi_ccu_off(const struct device *dev, clock_control_subsys_t sub_system)
{
	uint32_t clk_id = (uint32_t)(uintptr_t)sub_system;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	const struct ccu_gate *gate;

	if (clk_id >= ARRAY_SIZE(sun50i_h618_gates)) {
		return -EINVAL;
	}

	gate = &sun50i_h618_gates[clk_id];
	if (gate->offset == 0) {
		return -ENOTSUP;
	}

	sys_clear_bit(base + gate->offset, gate->bit);

	if (gate->rst_bit != SUNXI_CCU_NO_RST) {
		sys_clear_bit(base + gate->offset, gate->rst_bit);
	}

	return 0;
}

static uint32_t sunxi_get_pll_periph_rate(uintptr_t base, uint32_t offset)
{
	uint32_t reg_val = sys_read32(base + offset);

	/* N Factor */
	uint32_t n = FIELD_GET(CCU_PLL_PERIPH0_N_MASK, reg_val) + 1;
	/* M Factor */
	uint32_t m = FIELD_GET(CCU_PLL_PERIPH0_M_MASK, reg_val) + 1;
	/* P Factor */
	uint32_t p = FIELD_GET(CCU_PLL_PERIPH0_P_MASK, reg_val) + 1;

	if (!(reg_val & BIT(CCU_PLL_PERIPH0_EN_BIT))) {
		return 0;
	}

	/* PLL_PERIPH0 has a hardware fixed post-divider of 2 */
	return (SUN50I_H618_OSC24M_RATE * n) / (m * p * 2);
}

static uint32_t sunxi_get_apb2_rate(uintptr_t base)
{
	uint32_t val = sys_read32(base + CCU_APB2_CFG_REG);
	uint32_t src_sel = FIELD_GET(CCU_APB2_CLK_SRC_MASK, val);
	uint32_t parent_rate = 0;

	if (src_sel == 0) {
		parent_rate = SUN50I_H618_OSC24M_RATE;
	} else if (src_sel == 1) {
		parent_rate = SUN50I_H618_OSC32K_RATE;
	} else if (src_sel == 3) {
		/* 11: PLL_PERI0(1X) */
		parent_rate = sunxi_get_pll_periph_rate(base, CCU_PLL_PERIPH0_CTRL_REG);
	} else {
		/* 10: PSI */
		parent_rate = 0;
	}

	uint32_t n = FIELD_GET(CCU_APB2_CLK_FACTOR_N_MASK, val);
	uint32_t m = FIELD_GET(CCU_APB2_CLK_FACTOR_M_MASK, val);

	uint32_t divider = (1 << n) * (m + 1);

	return parent_rate / divider;
}

static int sunxi_ccu_get_rate(const struct device *dev, clock_control_subsys_t sub_system,
			      uint32_t *rate)
{
	uint32_t clk_id = (uint32_t)(uintptr_t)sub_system;
	uintptr_t base = DEVICE_MMIO_GET(dev);

	switch (clk_id) {
	case CLK_PLL_PERIPH0:
		*rate = sunxi_get_pll_periph_rate(base, CCU_PLL_PERIPH0_CTRL_REG);
		return 0;
	case CLK_APB2:
		*rate = sunxi_get_apb2_rate(base);
		return 0;
	case CLK_BUS_UART0:
	case CLK_BUS_UART1:
	case CLK_BUS_UART2:
	case CLK_BUS_UART3:
	case CLK_BUS_UART4:
	case CLK_BUS_UART5:
		*rate = sunxi_get_apb2_rate(base);
		return 0;

	default:
		return -ENOTSUP;
	}
}

static int sunxi_ccu_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

struct sunxi_ccu_config {
	DEVICE_MMIO_ROM;
};

struct sunxi_ccu_data {
	DEVICE_MMIO_RAM;
};

static DEVICE_API(clock_control, sunxi_ccu_api) = {
	.on = sunxi_ccu_on,
	.off = sunxi_ccu_off,
	.get_rate = sunxi_ccu_get_rate,
};

#define SUNXI_CCU_INIT(inst) \
	static const struct sunxi_ccu_config sunxi_ccu_cfg_##inst = { \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)), \
	}; \
	static struct sunxi_ccu_data sunxi_ccu_data_##inst; \
	DEVICE_DT_INST_DEFINE(inst, &sunxi_ccu_init, NULL, &sunxi_ccu_data_##inst, \
			      &sunxi_ccu_cfg_##inst, PRE_KERNEL_1, \
			      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &sunxi_ccu_api);

DT_INST_FOREACH_STATUS_OKAY(SUNXI_CCU_INIT)
