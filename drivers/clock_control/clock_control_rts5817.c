/*
 * Copyright (c) 2025 Realtek Semiconductor, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr/kernel.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rts5817_clock.h>
#include "zephyr/arch/common/sys_io.h"
#include "zephyr/device.h"
#include <errno.h>
#include <stdint.h>
#include "clock_control_rts5817_control.h"
#include "clock_control_rts5817_gpll.h"
#include "zephyr/devicetree.h"

#define DT_DRV_COMPAT realtek_rts5817_clock

struct rts_fp_clock_data {
	uint32_t syspll_frequency;
	struct k_spinlock lock;
};

struct rts_fp_clock_config {
	mem_addr_t base;
	mem_addr_t syspll_base;
	mem_addr_t sck_div_base;
};

struct clk_rlx {
	uint16_t id;
	const struct rlx_clk_ops *ops;
	uint16_t clkreg;
	uint16_t clk_change;
};

struct rlx_clk_ops {
	int (*enable)(const struct device *dev, const struct clk_rlx *clk);
	int (*disable)(const struct device *dev, const struct clk_rlx *clk);
	enum clock_control_status (*get_status)(const struct device *dev,
						const struct clk_rlx *clk);
	uint32_t (*get_rate)(const struct device *dev, const struct clk_rlx *clk);
	int (*set_rate)(const struct device *dev, const struct clk_rlx *clk, uint32_t rate);
};

static uint8_t m_div_array[] = {1, 2, 4, 6, 8, 10, 12, 14};

#define DEFINE_CLK_RLX(_name, _id, _ops, _clk_reg, _clk_change)                                    \
	static const struct clk_rlx _name = {                                                      \
		.id = _id,                                                                         \
		.ops = &_ops,                                                                      \
		.clkreg = _clk_reg,                                                                \
		.clk_change = _clk_change,                                                         \
	}
#define CK_CHANGE_NULL 0

static inline uint32_t rts_fp_clk_read_reg(const struct device *dev, uint32_t offset)
{
	const struct rts_fp_clock_config *config = dev->config;

	return sys_read32(config->base + offset);
}

static inline void rts_fp_clk_write_reg(const struct device *dev, uint32_t data, uint32_t offset)
{
	const struct rts_fp_clock_config *config = dev->config;

	sys_write32(data, config->base + offset);
}

static inline uint32_t rts_fp_read_sck_div_reg(const struct device *dev)
{
	const struct rts_fp_clock_config *config = dev->config;

	return sys_read32(config->sck_div_base);
}

static inline void rts_fp_write_sck_div_reg(const struct device *dev, uint32_t data)
{
	const struct rts_fp_clock_config *config = dev->config;

	sys_write32(data, config->sck_div_base);
}

static void set_change_bit(const struct device *dev, uint32_t clk_change)
{
	const struct rts_fp_clock_config *config = dev->config;

	sys_set_bit(config->base + R_SYS_CLK_CHANGE, clk_change);
}

static void clear_change_bit(const struct device *dev, uint32_t clk_change)
{
	const struct rts_fp_clock_config *config = dev->config;

	sys_clear_bit(config->base + R_SYS_CLK_CHANGE, clk_change);
}

static int rlx_enable_syspll(const struct device *dev, const struct clk_rlx *clk)
{
	const struct rts_fp_clock_config *config = dev->config;
	uint32_t timeout = 1000;

	sys_set_bits(config->syspll_base + R_SYSPLL_CTL, POW_SYSPLL_MASK);
	k_busy_wait(20);

	sys_set_bits(config->syspll_base + R_SYSPLL_CTL, PLL_LOAD_EN_MASK);
	k_busy_wait(10);

	sys_clear_bits(config->syspll_base + R_SYSPLL_CTL, PLL_LOAD_EN_MASK);
	k_busy_wait(70);

	while (!(sys_read32(config->syspll_base + R_SYSPLL_STS) & PLL_CKUSABLE_MASK) && timeout--) {
		k_busy_wait(10);
	}
	k_busy_wait(70);
	if (timeout) {
		return 0;
	} else {
		return -ETIMEDOUT;
	}
}

static int rlx_disable_syspll(const struct device *dev, const struct clk_rlx *clk)
{
	const struct rts_fp_clock_config *config = dev->config;

	sys_clear_bits(config->syspll_base + R_SYSPLL_CTL, POW_SYSPLL_MASK);
	k_busy_wait(10);

	return 0;
}

static enum clock_control_status rlx_syspll_status(const struct device *dev,
						   const struct clk_rlx *clk)
{
	const struct rts_fp_clock_config *config = dev->config;

	if (sys_read32(config->syspll_base + R_SYSPLL_STS) & PLL_CKUSABLE_MASK) {
		return CLOCK_CONTROL_STATUS_ON;
	}
	return CLOCK_CONTROL_STATUS_OFF;
}

static uint32_t rlx_syspll_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	ARG_UNUSED(clk);
	const struct rts_fp_clock_config *config = dev->config;
	struct rts_fp_clock_data *clk_data = dev->data;
	uint32_t ssc_n, ssc_f;
	uint32_t reg;

	reg = sys_read32(config->syspll_base + R_SYSPLL_NF_CODE);
	ssc_n = reg & N_SSC_MASK;
	ssc_f = reg & F_SSC_MASK;
	clk_data->syspll_frequency = MHZ(4) * (ssc_n + 2 + ssc_f / 4096); /* Hz */

	return clk_data->syspll_frequency;
}

static int rlx_syspll_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	const struct rts_fp_clock_config *config = dev->config;
	struct rts_fp_clock_data *clk_data = dev->data;
	uint32_t ssc_n, ssc_f;
	uint32_t reg;

	clk_data->syspll_frequency = rate;
	ssc_n = (rate / MHZ(4)) - 2;
	ssc_f = 4096 * (rate % MHZ(4)) / MHZ(4);

	sys_clear_bits(config->syspll_base + R_SYSPLL_CFG, PLL_REG_CCO_SEL_MASK);
	if (ssc_f) {
		sys_clear_bits(config->syspll_base + R_SYSPLL_CFG, PLL_REG_PI_SEL_MASK);
		sys_clear_bits(config->syspll_base + R_SYSPLL_CFG, PLL_BYPASS_PI_MASK);
	} else {
		sys_set_bits(config->syspll_base + R_SYSPLL_CFG, PLL_REG_PI_SEL_MASK);
		sys_set_bits(config->syspll_base + R_SYSPLL_CFG, PLL_BYPASS_PI_MASK);
	}
	sys_set_bits(config->syspll_base + R_SYSPLL_CFG, REG_SC_H_MASK);
	reg = sys_read32(config->syspll_base + R_SYSPLL_NF_CODE);
	reg = (reg & ~(F_SSC_MASK | N_SSC_MASK)) | (ssc_n << N_SSC_OFFSET) |
	      (ssc_f << F_SSC_OFFSET);
	sys_write32(reg, config->syspll_base + R_SYSPLL_NF_CODE);

	return 0;
}

static const struct rlx_clk_ops rlx_clk_syspll_ops = {
	.enable = rlx_enable_syspll,
	.disable = rlx_disable_syspll,
	.get_status = rlx_syspll_status,
	.get_rate = rlx_syspll_get_rate,
	.set_rate = rlx_syspll_set_rate,
};

static int rlx_enable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t en_mask;

	if (clk->id == RTS_FP_CLK_GE) {
		en_mask = GE_CLK_EN;
	} else {
		en_mask = COMMON_CLK_EN;
	}

	if (!(reg & en_mask)) {
		rts_fp_clk_write_reg(dev, (reg | en_mask), clk->clkreg);
	}
	return 0;
}

static int rlx_disable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t en_mask;

	if (clk->id == RTS_FP_CLK_GE) {
		en_mask = GE_CLK_EN;
	} else {
		en_mask = COMMON_CLK_EN;
	}

	if (reg & en_mask) {
		rts_fp_clk_write_reg(dev, (reg & ~en_mask), clk->clkreg);
	}
	return 0;
}

static enum clock_control_status rlx_get_status(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t en_mask;

	if (clk->id == RTS_FP_CLK_GE) {
		en_mask = GE_CLK_EN;
	} else {
		en_mask = COMMON_CLK_EN;
	}

	if (reg & en_mask) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static uint32_t rlx_common_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	struct rts_fp_clock_data *clk_data = dev->data;
	uint32_t src[] = {MHZ(240), MHZ(160), MHZ(96), clk_data->syspll_frequency};
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t divider, idx;
	uint32_t rate;

	idx = reg & COMMON_CLK_SRC_SEL_MASK;
	divider = (reg & COMMON_CLK_DIV_MASK) >> COMMON_CLK_DIV_OFFSET;
	rate = src[idx] / m_div_array[divider];

	return rate;
}

static void __aligned(32) __noinline change_bus_clk_sub(const struct device *dev,
							const struct clk_rlx *clk, uint32_t data)
{
	rts_fp_write_sck_div_reg(dev, 0x02);
	set_change_bit(dev, clk->clk_change);
	/* Assign writing R_SYS_BUS_CLK_CFG_REG to the next cache line */
	/* to ensure that it takes effect after reading the previous cacheline done */
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	rts_fp_clk_write_reg(dev, data, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	clear_change_bit(dev, clk->clk_change);
}

static void __aligned(32) __noinline change_cache_spi_clk_sub(const struct device *dev,
							      const struct clk_rlx *clk,
							      uint32_t data)
{
	set_change_bit(dev, clk->clk_change);
	/* Assign writing R_SYS_SPI_CACHE_CLK_CFG_REG to the next cache line */
	/* to ensure that it takes effect after reading the previous cacheline done */
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	rts_fp_clk_write_reg(dev, data, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	clear_change_bit(dev, clk->clk_change);
}

static int rlx_common_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	struct rts_fp_clock_data *data = dev->data;
	uint32_t divider, idx;
	uint32_t reg;
	k_spinlock_key_t key;

	if (clk->ops->get_rate(dev, clk) == rate) {
		return 0;
	}

	reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	switch (rate) {
	case MHZ(240):
		idx = 0;
		divider = 0;
		break;
	case MHZ(120):
		idx = 0;
		divider = 1;
		break;
	case MHZ(60):
		idx = 0;
		divider = 2;
		break;
	case MHZ(30):
		idx = 0;
		divider = 4;
		break;
	case MHZ(160):
		idx = 1;
		divider = 0;
		break;
	case MHZ(80):
		idx = 1;
		divider = 1;
		break;
	case MHZ(40):
		idx = 1;
		divider = 2;
		break;
	case MHZ(20):
		idx = 1;
		divider = 4;
		break;
	case MHZ(96):
		idx = 2;
		divider = 0;
		break;
	case MHZ(48):
		idx = 2;
		divider = 1;
		break;
	case MHZ(24):
		idx = 2;
		divider = 2;
		break;
	case MHZ(16):
		idx = 2;
		divider = 3;
		break;
	default:
		return -EINVAL;
	}

	reg &= ~(COMMON_CLK_SRC_SEL_MASK | COMMON_CLK_DIV_MASK);
	reg |= (idx << COMMON_CLK_SRC_SEL_OFFSET) | (divider << COMMON_CLK_DIV_OFFSET);

	if (clk->id == RTS_FP_CLK_BUS) {
		uint8_t sck_div = rts_fp_read_sck_div_reg(dev);

		key = k_spin_lock(&data->lock);
		change_bus_clk_sub(dev, clk, reg);
		k_spin_unlock(&data->lock, key);
		rts_fp_write_sck_div_reg(dev, sck_div);
	} else if (clk->id == RTS_FP_CLK_SPI_CACHE) {
		key = k_spin_lock(&data->lock);
		change_cache_spi_clk_sub(dev, clk, reg);
		k_spin_unlock(&data->lock, key);
	} else {
		key = k_spin_lock(&data->lock);
		set_change_bit(dev, clk->clk_change);
		rts_fp_clk_write_reg(dev, reg, clk->clkreg);
		arch_nop();
		arch_nop();
		arch_nop();
		arch_nop();
		clear_change_bit(dev, clk->clk_change);

		k_spin_unlock(&data->lock, key);
	}

	return 0;
}

static const struct rlx_clk_ops rlx_clk_bus_ops = {
	.get_rate = rlx_common_get_rate,
	.set_rate = rlx_common_set_rate,
};

static const struct rlx_clk_ops rlx_clk_common_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.get_status = rlx_get_status,
	.get_rate = rlx_common_get_rate,
	.set_rate = rlx_common_set_rate,
};

static uint32_t rlx_spi_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	struct rts_fp_clock_data *clk_data = dev->data;
	uint32_t src[] = {MHZ(240), MHZ(96), MHZ(80), clk_data->syspll_frequency};
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t divider, idx;
	uint32_t rate;

	idx = reg & COMMON_CLK_SRC_SEL_MASK;
	divider = (reg & COMMON_CLK_DIV_MASK) >> COMMON_CLK_DIV_OFFSET;
	rate = src[idx] / m_div_array[divider];

	return rate;
}

static int rlx_spi_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	struct rts_fp_clock_data *data = dev->data;
	uint32_t divider, idx;
	uint32_t reg;
	k_spinlock_key_t key;

	if (clk->ops->get_rate(dev, clk) == rate) {
		return 0;
	}

	reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	switch (rate) {
	case MHZ(240):
		idx = 0;
		divider = 0;
		break;
	case MHZ(120):
		idx = 0;
		divider = 1;
		break;
	case MHZ(60):
		idx = 0;
		divider = 2;
		break;
	case MHZ(30):
		idx = 0;
		divider = 4;
		break;
	case MHZ(80):
		idx = 2;
		divider = 0;
		break;
	case MHZ(40):
		idx = 2;
		divider = 1;
		break;
	case MHZ(20):
		idx = 2;
		divider = 2;
		break;
	case MHZ(96):
		idx = 1;
		divider = 0;
		break;
	case MHZ(48):
		idx = 1;
		divider = 1;
		break;
	case MHZ(24):
		idx = 1;
		divider = 2;
		break;
	case MHZ(16):
		idx = 1;
		divider = 3;
		break;
	default:
		return -EINVAL;
	}

	reg &= ~(COMMON_CLK_SRC_SEL_MASK | COMMON_CLK_DIV_MASK);
	reg |= (idx << COMMON_CLK_SRC_SEL_OFFSET) | (divider << COMMON_CLK_DIV_OFFSET);

	key = k_spin_lock(&data->lock);
	set_change_bit(dev, clk->clk_change);
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	clear_change_bit(dev, clk->clk_change);

	k_spin_unlock(&data->lock, key);

	return 0;
}

static const struct rlx_clk_ops rlx_clk_spi_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.get_status = rlx_get_status,
	.get_rate = rlx_spi_get_rate,
	.set_rate = rlx_spi_set_rate,
};

static uint32_t rlx_uart_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	struct rts_fp_clock_data *clk_data = dev->data;
	uint32_t src[] = {MHZ(96), MHZ(120), 0, clk_data->syspll_frequency};
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t divider, idx;
	uint32_t rate;

	idx = reg & COMMON_CLK_SRC_SEL_MASK;
	divider = (reg & COMMON_CLK_DIV_MASK) >> COMMON_CLK_DIV_OFFSET;
	rate = src[idx] / m_div_array[divider];

	return rate;
}

static int rlx_uart_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	struct rts_fp_clock_data *data = dev->data;
	uint32_t divider, idx;
	uint32_t reg;
	k_spinlock_key_t key;

	if (clk->ops->get_rate(dev, clk) == rate) {
		return 0;
	}

	reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	switch (rate) {
	case MHZ(120):
		idx = 1;
		divider = 0;
		break;
	case MHZ(60):
		idx = 1;
		divider = 1;
		break;
	case MHZ(30):
		idx = 1;
		divider = 2;
		break;
	case MHZ(48):
		idx = 0;
		divider = 1;
		break;
	case MHZ(24):
		idx = 0;
		divider = 2;
		break;
	case MHZ(16):
		idx = 0;
		divider = 3;
		break;
	default:
		return -EINVAL;
	}

	reg &= ~(COMMON_CLK_SRC_SEL_MASK | COMMON_CLK_DIV_MASK);
	reg |= (idx << COMMON_CLK_SRC_SEL_OFFSET) | (divider << COMMON_CLK_DIV_OFFSET);

	key = k_spin_lock(&data->lock);
	set_change_bit(dev, clk->clk_change);
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	clear_change_bit(dev, clk->clk_change);

	k_spin_unlock(&data->lock, key);
	return 0;
}

static const struct rlx_clk_ops rlx_clk_uart_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.get_status = rlx_get_status,
	.get_rate = rlx_uart_get_rate,
	.set_rate = rlx_uart_set_rate,
};

static uint32_t rlx_pke_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t src_clk_frq = MHZ(120);
	uint32_t divider;
	uint32_t rate;

	divider = (reg & COMMON_CLK_DIV_MASK) >> COMMON_CLK_DIV_OFFSET;
	rate = src_clk_frq / m_div_array[divider];

	return rate;
}

static int rlx_pke_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	struct rts_fp_clock_data *data = dev->data;
	uint32_t divider;
	uint32_t reg;
	k_spinlock_key_t key;

	if (clk->ops->get_rate(dev, clk) == rate) {
		return 0;
	}

	reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	switch (rate) {
	case MHZ(120):
		divider = 0;
		break;
	case MHZ(60):
		divider = 1;
		break;
	case MHZ(30):
		divider = 2;
		break;
	case MHZ(20):
		divider = 3;
		break;
	default:
		return -EINVAL;
	}

	reg &= ~COMMON_CLK_DIV_MASK;
	reg |= divider << COMMON_CLK_DIV_OFFSET;

	key = k_spin_lock(&data->lock);
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	k_spin_unlock(&data->lock, key);
	return 0;
}

static const struct rlx_clk_ops rlx_clk_pke_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.get_status = rlx_get_status,
	.get_rate = rlx_pke_get_rate,
	.set_rate = rlx_pke_set_rate,
};

static int rlx_i2c_enable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);

	reg |= I2C_CLK_EN;
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	if (clk->id == RTS_FP_CLK_I2C0) {
		reg |= I2C0_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C1) {
		reg |= I2C1_CLK_EN;
	} else {
		return -EINVAL;
	}

	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	return 0;
}

static int rlx_i2c_disable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);

	if (clk->id == RTS_FP_CLK_I2C0) {
		reg &= ~I2C0_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C1) {
		reg &= ~I2C1_CLK_EN;
	} else {
		return -EINVAL;
	}

	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	if (reg & (I2C0_CLK_EN | I2C1_CLK_EN)) {
		return 0;
	}

	reg &= ~I2C_CLK_EN;
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);

	return 0;
}

static enum clock_control_status rlx_i2c_get_status(const struct device *dev,
						    const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t en_mask;

	if (clk->id == RTS_FP_CLK_I2C0) {
		en_mask = I2C0_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C1) {
		en_mask = I2C1_CLK_EN;
	} else {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (reg & en_mask) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static uint32_t rlx_i2c_get_rate(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t src_clk_frq = MHZ(240);
	uint32_t divider;
	uint32_t rate;

	divider = (reg & COMMON_CLK_DIV_MASK) >> COMMON_CLK_DIV_OFFSET;
	rate = src_clk_frq / m_div_array[divider];

	return rate;
}

static int rlx_i2c_set_rate(const struct device *dev, const struct clk_rlx *clk, uint32_t rate)
{
	struct rts_fp_clock_data *data = dev->data;
	uint32_t divider;
	uint32_t reg;
	k_spinlock_key_t key;

	if (clk->ops->get_rate(dev, clk) == rate) {
		return 0;
	}

	reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	switch (rate) {
	case MHZ(240):
		divider = 0;
		break;
	case MHZ(120):
		divider = 1;
		break;
	case MHZ(60):
		divider = 2;
		break;
	case MHZ(40):
		divider = 3;
		break;
	default:
		return -EINVAL;
	}

	reg &= ~COMMON_CLK_DIV_MASK;
	reg |= divider << COMMON_CLK_DIV_OFFSET;

	key = k_spin_lock(&data->lock);
	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	arch_nop();
	arch_nop();
	arch_nop();
	arch_nop();
	k_spin_unlock(&data->lock, key);
	return 0;
}

static const struct rlx_clk_ops rlx_clk_i2c_ops = {
	.enable = rlx_i2c_enable_clk,
	.disable = rlx_i2c_disable_clk,
	.get_status = rlx_i2c_get_status,
	.get_rate = rlx_i2c_get_rate,
	.set_rate = rlx_i2c_set_rate,
};

static int rlx_ck60_enable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);

	if (clk->id == RTS_FP_CLK_TRNG) {
		reg |= TRNG_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C_S) {
		reg |= I2C_S_CLK_EN;
	} else {
		return -EINVAL;
	}

	rts_fp_clk_write_reg(dev, reg, clk->clkreg);
	return 0;
}

static int rlx_ck60_disable_clk(const struct device *dev, const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);

	if (clk->id == RTS_FP_CLK_TRNG) {
		reg &= ~TRNG_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C_S) {
		reg &= ~I2C_S_CLK_EN;
	} else {
		return -EINVAL;
	}

	rts_fp_clk_write_reg(dev, reg, clk->clkreg);

	return 0;
}

static enum clock_control_status rlx_ck60_get_status(const struct device *dev,
						     const struct clk_rlx *clk)
{
	uint32_t reg = rts_fp_clk_read_reg(dev, clk->clkreg);
	uint32_t en_mask;

	if (clk->id == RTS_FP_CLK_TRNG) {
		en_mask = TRNG_CLK_EN;
	} else if (clk->id == RTS_FP_CLK_I2C_S) {
		en_mask = I2C_S_CLK_EN;
	} else {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	if (reg & en_mask) {
		return CLOCK_CONTROL_STATUS_ON;
	} else {
		return CLOCK_CONTROL_STATUS_OFF;
	}
}

static const struct rlx_clk_ops rlx_clk_ck60_ops = {
	.enable = rlx_ck60_enable_clk,
	.disable = rlx_ck60_disable_clk,
	.get_status = rlx_ck60_get_status,
};

static const struct rlx_clk_ops rlx_clk_gate_ops = {
	.enable = rlx_enable_clk,
	.disable = rlx_disable_clk,
	.get_status = rlx_get_status,
};

DEFINE_CLK_RLX(syspll, RTS_FP_CLK_SYS_PLL, rlx_clk_syspll_ops, 0, 0);
DEFINE_CLK_RLX(bus_clk, RTS_FP_CLK_BUS, rlx_clk_bus_ops, R_SYS_BUS_CLK_CFG_REG,
	       CHANGE_BUS_CLK_PRE_OFFSET);
DEFINE_CLK_RLX(spi_cache_clk, RTS_FP_CLK_SPI_CACHE, rlx_clk_common_ops, R_SYS_SPI_CACHE_CLK_CFG_REG,
	       CHANGE_SPI_CACHE_CLK_OFFSET);
DEFINE_CLK_RLX(spi_ssor_clk, RTS_FP_CLK_SPI_SSOR, rlx_clk_spi_ops, R_SYS_SPI_SSOR_CLK_CFG_REG,
	       CHANGE_SPI_SSOR_CLK_OFFSET);
DEFINE_CLK_RLX(ssi_m_clk, RTS_FP_CLK_SPI_SSI_M, rlx_clk_spi_ops, R_SYS_SPI_SSI_M_CLK_CFG_REG,
	       CHANGE_SPI_SSI_M_CLK_OFFSET);
DEFINE_CLK_RLX(ssi_s_clk, RTS_FP_CLK_SPI_SSI_S, rlx_clk_spi_ops, R_SYS_SPI_SSI_S_CLK_CFG_REG,
	       CHANGE_SPI_SSI_S_CLK_OFFSET);
DEFINE_CLK_RLX(sha_clk, RTS_FP_CLK_SHA, rlx_clk_common_ops, R_SYS_SHA_CLK_CFG_REG,
	       CHANGE_SHA_CLK_OFFSET);
DEFINE_CLK_RLX(aes_clk, RTS_FP_CLK_AES, rlx_clk_common_ops, R_SYS_AES_CLK_CFG_REG,
	       CHANGE_AES_CLK_OFFSET);
DEFINE_CLK_RLX(pke_clk, RTS_FP_CLK_PKE, rlx_clk_pke_ops, R_SYS_PKE_CLK_CFG_REG, 0);
DEFINE_CLK_RLX(i2c0_clk, RTS_FP_CLK_I2C0, rlx_clk_i2c_ops, R_SYS_I2C_CLK_CFG_REG, 0);
DEFINE_CLK_RLX(i2c1_clk, RTS_FP_CLK_I2C1, rlx_clk_i2c_ops, R_SYS_I2C_CLK_CFG_REG, 0);
DEFINE_CLK_RLX(trng_clk, RTS_FP_CLK_TRNG, rlx_clk_ck60_ops, R_SYS_CK60_CFG_REG, 0);
DEFINE_CLK_RLX(i2c_s_clk, RTS_FP_CLK_I2C_S, rlx_clk_ck60_ops, R_SYS_CK60_CFG_REG, 0);
DEFINE_CLK_RLX(uart0_clk, RTS_FP_CLK_UART0, rlx_clk_uart_ops, R_SYS_UART0_CLK_CFG_REG,
	       CHANGE_UART0_CLK_OFFSET);
DEFINE_CLK_RLX(uart1_clk, RTS_FP_CLK_UART1, rlx_clk_uart_ops, R_SYS_UART1_CLK_CFG_REG,
	       CHANGE_UART1_CLK_OFFSET);
DEFINE_CLK_RLX(sie_clk, RTS_FP_CLK_SIE, rlx_clk_gate_ops, R_SYS_SIE_CLK_CFG_REG, 0);
DEFINE_CLK_RLX(puf_clk, RTS_FP_CLK_PUF, rlx_clk_gate_ops, R_SYS_PUF_CLK_CFG_REG, 0);
DEFINE_CLK_RLX(ge_clk, RTS_FP_CLK_GE, rlx_clk_gate_ops, R_SYS_BUS_CLK_CFG_REG, 0);

static const struct clk_rlx *const m_clks[RLX_CLK_NUM_SIZE] = {
	&syspll,    &bus_clk,   &spi_cache_clk, &spi_ssor_clk, &ssi_m_clk, &ssi_s_clk,
	&sha_clk,   &aes_clk,   &pke_clk,       &i2c0_clk,     &i2c1_clk,  &trng_clk,
	&i2c_s_clk, &uart0_clk, &uart1_clk,     &sie_clk,      &puf_clk,   &ge_clk,
};
static int rts_fp_clk_on(const struct device *dev, clock_control_subsys_t sys)
{
	uint16_t clk_id = (uint32_t)sys;
	const struct clk_rlx *clk;

	if (clk_id >= RLX_CLK_NUM_SIZE) {
		return -EINVAL;
	}

	clk = m_clks[clk_id];
	if (!clk->ops->enable) {
		return -ENOSYS;
	}

	return clk->ops->enable(dev, clk);
}

static int rts_fp_clk_off(const struct device *dev, clock_control_subsys_t sys)
{
	uint16_t clk_id = (uint32_t)sys;
	const struct clk_rlx *clk;

	if (clk_id >= RLX_CLK_NUM_SIZE) {
		return -EINVAL;
	}

	clk = m_clks[clk_id];
	if (!clk->ops->disable) {
		return -ENOSYS;
	}

	return clk->ops->disable(dev, clk);
}

static enum clock_control_status rts_fp_clk_get_status(const struct device *dev,
						       clock_control_subsys_t sys)
{
	uint16_t clk_id = (uint32_t)sys;
	const struct clk_rlx *clk;

	if (clk_id >= RLX_CLK_NUM_SIZE) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	clk = m_clks[clk_id];
	if (!clk->ops->get_status) {
		return CLOCK_CONTROL_STATUS_UNKNOWN;
	}

	return clk->ops->get_status(dev, clk);
}

static int rts_fp_clk_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	uint16_t clk_id = (uint32_t)sys;
	const struct clk_rlx *clk;

	if (clk_id >= RLX_CLK_NUM_SIZE) {
		return -EINVAL;
	}

	clk = m_clks[clk_id];
	if (!clk->ops->get_rate) {
		return -ENOSYS;
	}

	*rate = clk->ops->get_rate(dev, clk);
	return 0;
}

static int rts_fp_clk_set_rate(const struct device *dev, clock_control_subsys_t sys,
			       clock_control_subsys_rate_t rate)
{
	uint16_t clk_id = (uint32_t)sys;
	const struct clk_rlx *clk;

	if (clk_id >= RLX_CLK_NUM_SIZE) {
		return -EINVAL;
	}

	clk = m_clks[clk_id];
	if (!clk->ops->set_rate) {
		return -ENOSYS;
	}

	return clk->ops->set_rate(dev, clk, (uint32_t)rate);
}

static const struct clock_control_driver_api rts_fp_clk_api = {
	.on = rts_fp_clk_on,
	.off = rts_fp_clk_off,
	.get_status = rts_fp_clk_get_status,
	.get_rate = rts_fp_clk_get_rate,
	.set_rate = rts_fp_clk_set_rate,
};

/* HACK: enable uart clock for uart_ns16550.c driver */
static void rts_fp_clk_enable_uart(const struct device *dev)
{
	/* Set uart clock source to 120MHz */
	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart0))) {
		rts_fp_clk_write_reg(dev, 1 | COMMON_CLK_EN, R_SYS_UART0_CLK_CFG_REG);
	}

	if (DT_NODE_HAS_STATUS_OKAY(DT_NODELABEL(uart1))) {
		rts_fp_clk_write_reg(dev, 1 | COMMON_CLK_EN, R_SYS_UART1_CLK_CFG_REG);
	}
}

static int rts_fp_clk_init(const struct device *dev)
{
	struct rts_fp_clock_data *clk_data = dev->data;

	if (syspll.ops->get_status(dev, &syspll) == CLOCK_CONTROL_STATUS_ON) {
		clk_data->syspll_frequency = syspll.ops->get_rate(dev, &syspll);
	}

	/* HACK: enable uart clock for uart_ns16550.c driver */
	rts_fp_clk_enable_uart(dev);

	/* Set bus rate according to clock-frequency in cpu node */
	clock_control_subsys_t sys = (clock_control_subsys_t)RTS_FP_CLK_BUS;
	clock_control_subsys_rate_t rate =
		(clock_control_subsys_t)DT_PROP(DT_PATH(cpus, cpu_0), clock_frequency);

	rts_fp_clk_set_rate(dev, sys, rate);

	return 0;
}

static struct rts_fp_clock_data rts_fp_clock_data = {
	.syspll_frequency = 0,
};

static const struct rts_fp_clock_config rts_fp_clock_config = {
	.base = DT_INST_REG_ADDR(0),
	.syspll_base = DT_INST_REG_ADDR_BY_IDX(0, 1),
	.sck_div_base = DT_INST_REG_ADDR_BY_IDX(0, 2),
};

DEVICE_DT_INST_DEFINE(0, rts_fp_clk_init, NULL, &rts_fp_clock_data, &rts_fp_clock_config,
		      PRE_KERNEL_1, CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &rts_fp_clk_api);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) <= 1,
	     "Only one clock control instance can be supported");
