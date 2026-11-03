/*
 * SPDX-FileCopyrightText: Copyright The Zephyr Project Contributors
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Xilinx Zynq-7000 clock controller, the clock part of the SLCR.
 */

#define DT_DRV_COMPAT xlnx_ps7_clkc

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/syscon.h>
#include <zephyr/dt-bindings/clock/xlnx-zynq-clkc.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_xlnx_zynq, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

BUILD_ASSERT(DT_NUM_INST_STATUS_OKAY(DT_DRV_COMPAT) == 1, "Unsupported number of instances");

/* Registers, offsets from the clock control part of the SLCR (Zynq-7000 TRM, B.28) */
#define ARM_PLL_CTRL  0x00
#define DDR_PLL_CTRL  0x04
#define IO_PLL_CTRL   0x08
#define ARM_CLK_CTRL  0x20
#define APER_CLK_CTRL 0x2c
#define GEM0_CLK_CTRL 0x40
#define GEM1_CLK_CTRL 0x44
#define CLK_621_TRUE  0xc4

#define PLL_CTRL_RESET        BIT(0)
#define PLL_CTRL_PWRDWN       BIT(1)
#define PLL_CTRL_BYPASS_FORCE BIT(4)
#define PLL_CTRL_FDIV         GENMASK(18, 12)

/* ARM_CLK_CTRL: the source is a PLL, the divisor feeds cpu_6or4x */
#define ARM_CLK_CTRL_SRCSEL  GENMASK(5, 4)
#define ARM_CLK_CTRL_DIVISOR GENMASK(13, 8)

/* GEMx_CLK_CTRL: a PLL or the EMIO clock through two dividers and a gate */
#define GEM_CLK_CTRL_CLKACT   BIT(0)
#define GEM_CLK_CTRL_SRCSEL   GENMASK(5, 4)
#define GEM_CLK_CTRL_EMIO     BIT(6)
#define GEM_CLK_CTRL_DIVISOR0 GENMASK(13, 8)
#define GEM_CLK_CTRL_DIVISOR1 GENMASK(25, 20)

#define GEM_CLK_DIV_MAX 63U

#define APER_CLK_CTRL_GEM0 BIT(6)
#define APER_CLK_CTRL_GEM1 BIT(7)

/* The CPU clocks run at the 6:2:1 ratio instead of 4:2:1 */
#define CLK_621_TRUE_621 BIT(0)

struct zynq_clkc_config {
	const struct device *syscon;
	/* Offset of the clock control registers in the SLCR */
	uint32_t base;
	uint32_t ps_clk;
};

struct zynq_clkc_data {
	struct k_spinlock lock;
};

static int zynq_clkc_read(const struct device *dev, uint32_t reg, uint32_t *val)
{
	const struct zynq_clkc_config *cfg = dev->config;

	return syscon_read_reg(cfg->syscon, cfg->base + reg, val);
}

static int zynq_clkc_write(const struct device *dev, uint32_t reg, uint32_t val)
{
	const struct zynq_clkc_config *cfg = dev->config;

	return syscon_write_reg(cfg->syscon, cfg->base + reg, val);
}

static int zynq_clkc_pll_rate(const struct device *dev, uint32_t reg, uint32_t *rate)
{
	const struct zynq_clkc_config *cfg = dev->config;
	uint32_t val;
	int ret;

	ret = zynq_clkc_read(dev, reg, &val);
	if (ret < 0) {
		return ret;
	}

	if ((val & PLL_CTRL_BYPASS_FORCE) != 0U) {
		*rate = cfg->ps_clk;
	} else if ((val & (PLL_CTRL_RESET | PLL_CTRL_PWRDWN)) != 0U) {
		*rate = 0U;
	} else {
		*rate = cfg->ps_clk * FIELD_GET(PLL_CTRL_FDIV, val);
	}

	return 0;
}

/* The PLL a SRCSEL field of the CPU clock selects: ARM PLL, DDR PLL or IO PLL */
static int zynq_clkc_cpu_src_rate(const struct device *dev, uint32_t srcsel, uint32_t *rate)
{
	switch (srcsel) {
	case 2U:
		return zynq_clkc_pll_rate(dev, DDR_PLL_CTRL, rate);
	case 3U:
		return zynq_clkc_pll_rate(dev, IO_PLL_CTRL, rate);
	default:
		return zynq_clkc_pll_rate(dev, ARM_PLL_CTRL, rate);
	}
}

/* The PLL a SRCSEL field of a peripheral clock selects: IO PLL, ARM PLL or DDR PLL */
static int zynq_clkc_periph_src_rate(const struct device *dev, uint32_t srcsel, uint32_t *rate)
{
	switch (srcsel) {
	case 2U:
		return zynq_clkc_pll_rate(dev, ARM_PLL_CTRL, rate);
	case 3U:
		return zynq_clkc_pll_rate(dev, DDR_PLL_CTRL, rate);
	default:
		return zynq_clkc_pll_rate(dev, IO_PLL_CTRL, rate);
	}
}

static int zynq_clkc_cpu_rate(const struct device *dev, uint32_t id, uint32_t *rate)
{
	uint32_t ctrl, ratio, divisor, pll_rate;
	bool is_621;
	int ret;

	ret = zynq_clkc_read(dev, ARM_CLK_CTRL, &ctrl);
	if (ret < 0) {
		return ret;
	}

	ret = zynq_clkc_read(dev, CLK_621_TRUE, &ratio);
	if (ret < 0) {
		return ret;
	}

	ret = zynq_clkc_cpu_src_rate(dev, FIELD_GET(ARM_CLK_CTRL_SRCSEL, ctrl), &pll_rate);
	if (ret < 0) {
		return ret;
	}

	is_621 = (ratio & CLK_621_TRUE_621) != 0U;
	divisor = MAX(FIELD_GET(ARM_CLK_CTRL_DIVISOR, ctrl), 1U);

	switch (id) {
	case ZYNQ_CLK_CPU_6OR4X:
		break;
	case ZYNQ_CLK_CPU_3OR2X:
		divisor *= 2U;
		break;
	case ZYNQ_CLK_CPU_2X:
		divisor *= is_621 ? 3U : 2U;
		break;
	default:
		divisor *= is_621 ? 6U : 4U;
		break;
	}

	*rate = pll_rate / divisor;

	return 0;
}

static int zynq_clkc_gem_rate(const struct device *dev, uint32_t reg, uint32_t *rate)
{
	uint32_t ctrl, div0, div1, pll_rate;
	int ret;

	ret = zynq_clkc_read(dev, reg, &ctrl);
	if (ret < 0) {
		return ret;
	}

	if ((ctrl & GEM_CLK_CTRL_EMIO) != 0U) {
		return -ENOTSUP;
	}

	ret = zynq_clkc_periph_src_rate(dev, FIELD_GET(GEM_CLK_CTRL_SRCSEL, ctrl), &pll_rate);
	if (ret < 0) {
		return ret;
	}

	div0 = MAX(FIELD_GET(GEM_CLK_CTRL_DIVISOR0, ctrl), 1U);
	div1 = MAX(FIELD_GET(GEM_CLK_CTRL_DIVISOR1, ctrl), 1U);

	*rate = pll_rate / (div0 * div1);

	return 0;
}

/* Set the two dividers of a GEM clock to the pair closest to the target rate */
static int zynq_clkc_gem_set_rate(const struct device *dev, uint32_t reg, uint32_t target)
{
	struct zynq_clkc_data *data = dev->data;
	uint32_t best_err = UINT32_MAX, best_div0 = 0U, best_div1 = 0U;
	uint32_t ctrl, pll_rate;
	k_spinlock_key_t key;
	int ret;

	if (target == 0U) {
		return -EINVAL;
	}

	ret = zynq_clkc_read(dev, reg, &ctrl);
	if (ret < 0) {
		return ret;
	}

	if ((ctrl & GEM_CLK_CTRL_EMIO) != 0U) {
		return -ENOTSUP;
	}

	ret = zynq_clkc_periph_src_rate(dev, FIELD_GET(GEM_CLK_CTRL_SRCSEL, ctrl), &pll_rate);
	if (ret < 0) {
		return ret;
	}

	for (uint32_t div0 = 1U; (div0 <= GEM_CLK_DIV_MAX) && (best_err > 0U); div0++) {
		for (uint32_t div1 = 1U; div1 <= GEM_CLK_DIV_MAX; div1++) {
			uint32_t rate = pll_rate / (div0 * div1);
			uint32_t err = (rate > target) ? (rate - target) : (target - rate);

			if (err < best_err) {
				best_err = err;
				best_div0 = div0;
				best_div1 = div1;
			}
			if (err == 0U) {
				break;
			}
		}
	}

	LOG_DBG("dividers %u/%u for %u Hz from %u Hz", best_div0, best_div1, target, pll_rate);

	key = k_spin_lock(&data->lock);

	ret = zynq_clkc_read(dev, reg, &ctrl);
	if (ret == 0) {
		ctrl &= ~(GEM_CLK_CTRL_DIVISOR0 | GEM_CLK_CTRL_DIVISOR1);
		ctrl |= FIELD_PREP(GEM_CLK_CTRL_DIVISOR0, best_div0) |
			FIELD_PREP(GEM_CLK_CTRL_DIVISOR1, best_div1);
		ret = zynq_clkc_write(dev, reg, ctrl);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int zynq_clkc_gate(const struct device *dev, uint32_t reg, uint32_t mask, bool on)
{
	struct zynq_clkc_data *data = dev->data;
	k_spinlock_key_t key;
	uint32_t val;
	int ret;

	key = k_spin_lock(&data->lock);

	ret = zynq_clkc_read(dev, reg, &val);
	if (ret == 0) {
		if (on) {
			val |= mask;
		} else {
			val &= ~mask;
		}
		ret = zynq_clkc_write(dev, reg, val);
	}

	k_spin_unlock(&data->lock, key);

	return ret;
}

static int zynq_clkc_on_off(const struct device *dev, clock_control_subsys_t sys, bool on)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case ZYNQ_CLK_ARMPLL:
	case ZYNQ_CLK_DDRPLL:
	case ZYNQ_CLK_IOPLL:
	case ZYNQ_CLK_CPU_6OR4X:
	case ZYNQ_CLK_CPU_3OR2X:
	case ZYNQ_CLK_CPU_2X:
	case ZYNQ_CLK_CPU_1X:
		/* Always running, the system depends on them */
		return on ? 0 : -ENOTSUP;
	case ZYNQ_CLK_GEM0:
		return zynq_clkc_gate(dev, GEM0_CLK_CTRL, GEM_CLK_CTRL_CLKACT, on);
	case ZYNQ_CLK_GEM1:
		return zynq_clkc_gate(dev, GEM1_CLK_CTRL, GEM_CLK_CTRL_CLKACT, on);
	case ZYNQ_CLK_GEM0_APER:
		return zynq_clkc_gate(dev, APER_CLK_CTRL, APER_CLK_CTRL_GEM0, on);
	case ZYNQ_CLK_GEM1_APER:
		return zynq_clkc_gate(dev, APER_CLK_CTRL, APER_CLK_CTRL_GEM1, on);
	default:
		return -ENOTSUP;
	}
}

static int zynq_clkc_on(const struct device *dev, clock_control_subsys_t sys)
{
	return zynq_clkc_on_off(dev, sys, true);
}

static int zynq_clkc_off(const struct device *dev, clock_control_subsys_t sys)
{
	return zynq_clkc_on_off(dev, sys, false);
}

static int zynq_clkc_get_rate(const struct device *dev, clock_control_subsys_t sys, uint32_t *rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;

	switch (id) {
	case ZYNQ_CLK_ARMPLL:
		return zynq_clkc_pll_rate(dev, ARM_PLL_CTRL, rate);
	case ZYNQ_CLK_DDRPLL:
		return zynq_clkc_pll_rate(dev, DDR_PLL_CTRL, rate);
	case ZYNQ_CLK_IOPLL:
		return zynq_clkc_pll_rate(dev, IO_PLL_CTRL, rate);
	case ZYNQ_CLK_CPU_6OR4X:
	case ZYNQ_CLK_CPU_3OR2X:
	case ZYNQ_CLK_CPU_2X:
	case ZYNQ_CLK_CPU_1X:
	case ZYNQ_CLK_GEM0_APER:
	case ZYNQ_CLK_GEM1_APER:
		return zynq_clkc_cpu_rate(dev, (id <= ZYNQ_CLK_CPU_1X) ? id : ZYNQ_CLK_CPU_1X,
					  rate);
	case ZYNQ_CLK_GEM0:
		return zynq_clkc_gem_rate(dev, GEM0_CLK_CTRL, rate);
	case ZYNQ_CLK_GEM1:
		return zynq_clkc_gem_rate(dev, GEM1_CLK_CTRL, rate);
	default:
		return -ENOTSUP;
	}
}

static int zynq_clkc_set_rate(const struct device *dev, clock_control_subsys_t sys,
			      clock_control_subsys_rate_t rate)
{
	uint32_t id = (uint32_t)(uintptr_t)sys;
	uint32_t target = (uint32_t)(uintptr_t)rate;

	switch (id) {
	case ZYNQ_CLK_GEM0:
		return zynq_clkc_gem_set_rate(dev, GEM0_CLK_CTRL, target);
	case ZYNQ_CLK_GEM1:
		return zynq_clkc_gem_set_rate(dev, GEM1_CLK_CTRL, target);
	default:
		return -ENOTSUP;
	}
}

static DEVICE_API(clock_control, zynq_clkc_api) = {
	.on = zynq_clkc_on,
	.off = zynq_clkc_off,
	.get_rate = zynq_clkc_get_rate,
	.set_rate = zynq_clkc_set_rate,
};

static const struct zynq_clkc_config zynq_clkc_config_0 = {
	.syscon = DEVICE_DT_GET(DT_INST_PHANDLE(0, syscon)),
	.base = DT_INST_REG_ADDR(0),
	.ps_clk = DT_INST_PROP(0, ps_clk_frequency),
};

static struct zynq_clkc_data zynq_clkc_data_0;

/* At the priority of the syscon, the devicetree dependency orders it after the SLCR */
DEVICE_DT_INST_DEFINE(0, NULL, NULL, &zynq_clkc_data_0, &zynq_clkc_config_0, PRE_KERNEL_1,
		      CONFIG_SYSCON_INIT_PRIORITY, &zynq_clkc_api);
