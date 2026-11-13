/*
 * SPDX-FileCopyrightText: Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 CRU clock controller.
 */

#define DT_DRV_COMPAT rockchip_rk3588_cru_clk

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rk3588-cru.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(clock_control_rk3588_cru, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RK3588_CRU_CLKGATE_CON(n) ((n) * 4U + 0x800U)
#define RK3588_CRU_CLKSEL_CON(n)  ((n) * 4U + 0x300U)
#define RK3588_PHP_CLKGATE_CON0   0x8800U
#define RK3588_PPLL_CON0          0x8200U
#define RK3588_PPLL_CON1          0x8204U
#define RK3588_PPLL_CON2          0x8208U
#define RK3588_PPLL_PWRDOWN       BIT(13)
#define RK3588_XIN24M             24000000U
#define RK3588_PPLL_FRAC_DIV      65535U

struct clock_control_rk3588_config {
	DEVICE_MMIO_ROM;
};

struct clock_control_rk3588_data {
	DEVICE_MMIO_RAM;
};

static bool rk3588_cru_clk_gate_lookup(uint16_t clk_id, uint32_t *gate_off, uint8_t *gate_shift)
{
	switch (clk_id) {
	case PCLK_TOP_ROOT:
		*gate_off = RK3588_CRU_CLKGATE_CON(1);
		*gate_shift = 1;
		return true;
	case PCLK_GPIO3:
		*gate_off = RK3588_CRU_CLKGATE_CON(17);
		*gate_shift = 2;
		return true;
	case PCLK_PHP_ROOT:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 0;
		return true;
	case ACLK_PCIE_ROOT:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 6;
		return true;
	case ACLK_PHP_ROOT:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 7;
		return true;
	case ACLK_PCIE_BRIDGE:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 8;
		return true;
	case ACLK_PCIE_1L2_DBI:
		*gate_off = RK3588_CRU_CLKGATE_CON(33);
		*gate_shift = 1;
		return true;
	case ACLK_PCIE_1L2_MSTR:
		*gate_off = RK3588_CRU_CLKGATE_CON(33);
		*gate_shift = 6;
		return true;
	case ACLK_PCIE_1L2_SLV:
		*gate_off = RK3588_CRU_CLKGATE_CON(33);
		*gate_shift = 11;
		return true;
	case PCLK_PCIE_1L2:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 0;
		return true;
	case CLK_PCIE_AUX4:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 5;
		return true;
	case ACLK_PHP_GIC_ITS:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 6;
		return true;
	case ACLK_MMU_PCIE:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 7;
		return true;
	case CLK_PIPEPHY0_PIPE_G:
		*gate_off = RK3588_CRU_CLKGATE_CON(38);
		*gate_shift = 3;
		return true;
	case CLK_PCIE1L2_PIPE:
		*gate_off = RK3588_CRU_CLKGATE_CON(38);
		*gate_shift = 13;
		return true;
	case CLK_REF_PIPE_PHY0:
		/* Gated by the PPLL source branch of the ComboPHY0 reference mux. */
		*gate_off = RK3588_CRU_CLKGATE_CON(77);
		*gate_shift = 3;
		return true;
	case PCLK_PCIE_COMBO_PIPE_PHY0:
		*gate_off = RK3588_PHP_CLKGATE_CON0;
		*gate_shift = 5;
		return true;
	case MCLK_GMAC0_OUT:
		*gate_off = RK3588_CRU_CLKGATE_CON(5);
		*gate_shift = 3;
		return true;
	case CLK_GMAC_125M:
		*gate_off = RK3588_CRU_CLKGATE_CON(35);
		*gate_shift = 5;
		return true;
	case CLK_GMAC_50M:
		*gate_off = RK3588_CRU_CLKGATE_CON(35);
		*gate_shift = 6;
		return true;
	case CLK_GMAC0_PTP_REF:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 10;
		return true;
	case CLK_GMAC1_PTP_REF:
		*gate_off = RK3588_CRU_CLKGATE_CON(34);
		*gate_shift = 11;
		return true;
	case PCLK_GMAC0:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 3;
		return true;
	case PCLK_GMAC1:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 4;
		return true;
	case ACLK_GMAC0:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 10;
		return true;
	case ACLK_GMAC1:
		*gate_off = RK3588_CRU_CLKGATE_CON(32);
		*gate_shift = 11;
		return true;
	default:
		return false;
	}
}

static void rk3588_cru_gate_write(mm_reg_t base, uint32_t gate_off, uint8_t gate_shift, bool enable)
{
	uint32_t reg = sys_read32(base + gate_off);
	uint32_t val;

	if (enable) {
		val = (BIT(gate_shift) << 16) | (reg & ~BIT(gate_shift));
	} else {
		val = (BIT(gate_shift) << 16) | (reg | BIT(gate_shift));
	}

	sys_write32(val, base + gate_off);
}

static int rk3588_cru_clk_on_off(const struct device *dev, clock_control_subsys_t sys, bool enable)
{
	uint16_t clk_id = (uint16_t)(uintptr_t)sys;
	uint32_t gate_off;
	uint8_t gate_shift;

	if (!rk3588_cru_clk_gate_lookup(clk_id, &gate_off, &gate_shift)) {
		LOG_ERR("unknown clock id %u", clk_id);
		return -EINVAL;
	}

	rk3588_cru_gate_write(DEVICE_MMIO_GET(dev), gate_off, gate_shift, enable);

	return 0;
}

static int clock_control_rk3588_init(const struct device *dev)
{
	/*
	 * The GICv3 ITS is instantiated below the GIC node and has no clock
	 * phandle, so its clock gate and the gate of its parent have to be
	 * enabled here, before the interrupt controllers are initialized.
	 */
	static const uint16_t its_clocks[] = {ACLK_PCIE_ROOT, ACLK_PHP_GIC_ITS};
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (IS_ENABLED(CONFIG_GIC_V3_ITS)) {
		for (size_t i = 0; i < ARRAY_SIZE(its_clocks); i++) {
			ret = rk3588_cru_clk_on_off(
				dev, (clock_control_subsys_t)(uintptr_t)its_clocks[i], true);
			if (ret != 0) {
				return ret;
			}
		}
	}

	return 0;
}

static int clock_control_rk3588_on(const struct device *dev, clock_control_subsys_t sys)
{
	return rk3588_cru_clk_on_off(dev, sys, true);
}

static int clock_control_rk3588_off(const struct device *dev, clock_control_subsys_t sys)
{
	return rk3588_cru_clk_on_off(dev, sys, false);
}

static int rk3588_ppll_get_rate(mm_reg_t base, uint32_t *rate)
{
	uint32_t con1 = sys_read32(base + RK3588_PPLL_CON1);
	uint32_t m = FIELD_GET(GENMASK(9, 0), sys_read32(base + RK3588_PPLL_CON0));
	uint32_t p = FIELD_GET(GENMASK(5, 0), con1);
	uint32_t s = FIELD_GET(GENMASK(8, 6), con1);
	uint32_t k = FIELD_GET(GENMASK(15, 0), sys_read32(base + RK3588_PPLL_CON2));
	int64_t frequency;

	if ((con1 & RK3588_PPLL_PWRDOWN) != 0U || m == 0U || p == 0U) {
		return -EIO;
	}

	/* The fractional part of the PPLL rate is k / 65535. */
	frequency = (int64_t)RK3588_XIN24M * m / p;
	if (k != 0U) {
		frequency += (int64_t)RK3588_XIN24M * k / ((int64_t)p * RK3588_PPLL_FRAC_DIV);
	}
	frequency >>= s;

	if (frequency <= 0 || frequency > UINT32_MAX) {
		return -ERANGE;
	}
	*rate = (uint32_t)frequency;
	return 0;
}

static int clock_control_rk3588_get_rate(const struct device *dev, clock_control_subsys_t sys,
					 uint32_t *rate)
{
	uint16_t clk_id = (uint16_t)(uintptr_t)sys;
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	int ret;

	switch (clk_id) {
	case CLK_PCIE_AUX4:
		*rate = RK3588_XIN24M;
		return 0;
	case CLK_REF_PIPE_PHY0:
		if ((sys_read32(base + RK3588_CRU_CLKSEL_CON(177)) & BIT(6)) == 0U) {
			*rate = RK3588_XIN24M;
			return 0;
		}
		ret = rk3588_ppll_get_rate(base, rate);
		if (ret != 0) {
			return ret;
		}
		*rate /= FIELD_GET(GENMASK(5, 0), sys_read32(base + RK3588_CRU_CLKSEL_CON(176))) +
			 1U;
		return 0;
	case CLK_GMAC_125M:
	case MCLK_GMAC0_OUT:
		*rate = 125000000U;
		return 0;
	case CLK_GMAC_50M:
		*rate = 50000000U;
		return 0;
	case CLK_GMAC0_PTP_REF:
	case CLK_GMAC1_PTP_REF:
		*rate = 100000000U;
		return 0;
	case PCLK_GMAC0:
	case PCLK_GMAC1:
	case ACLK_GMAC0:
	case ACLK_GMAC1:
		*rate = 150000000U;
		return 0;
	default:
		return -ENOTSUP;
	}
}

/*
 * Only the ComboPHY0 reference clock is rate selectable. The PPLL feeding it is
 * configured by the boot firmware; this driver only programs the divider and
 * selects the PPLL branch of the reference clock mux. The requested rate is the
 * frequency in Hz passed as an integer through clock_control_subsys_rate_t.
 */
static int clock_control_rk3588_set_rate(const struct device *dev, clock_control_subsys_t sys,
					 clock_control_subsys_rate_t rate)
{
	mm_reg_t base = DEVICE_MMIO_GET(dev);
	uint32_t requested = (uint32_t)(uintptr_t)rate;
	uint32_t parent_rate, divider;
	int ret;

	if ((uintptr_t)sys != CLK_REF_PIPE_PHY0) {
		return -ENOTSUP;
	}
	if (requested == 0U) {
		return -EINVAL;
	}

	ret = rk3588_ppll_get_rate(base, &parent_rate);
	if (ret != 0) {
		return ret;
	}
	if (parent_rate % requested != 0U) {
		return -ENOTSUP;
	}
	divider = parent_rate / requested;
	if (divider == 0U || divider > 64U) {
		return -ERANGE;
	}

	sys_write32((GENMASK(5, 0) << 16) | (divider - 1U), base + RK3588_CRU_CLKSEL_CON(176));
	sys_write32(BIT(22) | BIT(6), base + RK3588_CRU_CLKSEL_CON(177));
	return 0;
}

static DEVICE_API(clock_control, clock_control_rk3588_api) = {
	.on = clock_control_rk3588_on,
	.off = clock_control_rk3588_off,
	.get_rate = clock_control_rk3588_get_rate,
	.set_rate = clock_control_rk3588_set_rate,
};

static struct clock_control_rk3588_data rk3588_cru_data;

static const struct clock_control_rk3588_config clock_control_rk3588_config = {
	DEVICE_MMIO_ROM_INIT(DT_INST_PARENT(0)),
};

DEVICE_DT_INST_DEFINE(0, clock_control_rk3588_init, NULL, &rk3588_cru_data,
		      &clock_control_rk3588_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_rk3588_api);
