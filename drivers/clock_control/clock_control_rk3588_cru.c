/*
 * Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 CRU clock gate controller.
 */

#define DT_DRV_COMPAT rockchip_rk3588_cru_clk

#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/dt-bindings/clock/rk3588-cru.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>

LOG_MODULE_REGISTER(clock_control_rk3588_cru, CONFIG_CLOCK_CONTROL_LOG_LEVEL);

#define RK3588_CRU_CLKGATE_CON(n) ((n) * 4U + 0x800U)
/* Gate registers sit in the first 4 KiB of the CRU block. */
#define RK3588_CRU_MAP_SIZE       0x1000U

struct clock_control_rk3588_config {
	DEVICE_MMIO_ROM;
};

struct clock_control_rk3588_data {
	DEVICE_MMIO_RAM;
};

static bool rk3588_cru_clk_gate_lookup(uint16_t clk_id, uint32_t *gate_off, uint8_t *gate_shift)
{
	switch (clk_id) {
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

static void rk3588_cru_gate_write(mm_reg_t base, uint32_t gate_off, uint8_t gate_shift,
				  bool enable)
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
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

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

static int clock_control_rk3588_get_rate(const struct device *dev, clock_control_subsys_t sys,
					 uint32_t *rate)
{
	ARG_UNUSED(dev);
	ARG_UNUSED(sys);
	ARG_UNUSED(rate);

	return -ENOSYS;
}

static DEVICE_API(clock_control, clock_control_rk3588_api) = {
	.on = clock_control_rk3588_on,
	.off = clock_control_rk3588_off,
	.get_rate = clock_control_rk3588_get_rate,
};

static struct clock_control_rk3588_data rk3588_cru_data;

static const struct clock_control_rk3588_config clock_control_rk3588_config = {
	._mmio = {
		.phys_addr = DT_REG_ADDR(DT_INST_PARENT(0)),
		.size = RK3588_CRU_MAP_SIZE,
	},
};

DEVICE_DT_INST_DEFINE(0, clock_control_rk3588_init, NULL, &rk3588_cru_data,
		      &clock_control_rk3588_config, PRE_KERNEL_1,
		      CONFIG_CLOCK_CONTROL_INIT_PRIORITY, &clock_control_rk3588_api);
