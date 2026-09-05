/*
 * Copyright (c) 2026 KylinSoft Corporation
 * SPDX-License-Identifier: Apache-2.0
 *
 * Rockchip RK3588 CRU reset controller.
 */

#define DT_DRV_COMPAT rockchip_rk3588_cru_rst

#include <zephyr/arch/cpu.h>
#include <zephyr/device.h>
#include <zephyr/drivers/reset.h>
#include <zephyr/sys/device_mmio.h>

#define RK3588_CRU_SOFTRST_CON(n) ((n) * 4U + 0xA00U)
/* Softreset registers sit in the first 4 KiB of the CRU block. */
#define RK3588_CRU_MAP_SIZE       0x1000U

struct reset_rk3588_config {
	DEVICE_MMIO_ROM;
};

struct reset_rk3588_data {
	DEVICE_MMIO_RAM;
};

static void rk3588_cru_reset_write(mm_reg_t base, uint32_t reg_off, uint8_t bit, bool assert)
{
	uint32_t reg = sys_read32(base + reg_off);
	uint32_t val;

	if (assert) {
		val = (BIT(bit) << 16) | (reg | BIT(bit));
	} else {
		val = (BIT(bit) << 16) | (reg & ~BIT(bit));
	}

	sys_write32(val, base + reg_off);
}

static int reset_rk3588_status(const struct device *dev, uint32_t id, uint8_t *status)
{
	uint32_t reg_off = RK3588_CRU_SOFTRST_CON(id / 16U);
	uint8_t bit = id % 16U;

	*status = !!sys_test_bit(DEVICE_MMIO_GET(dev) + reg_off, bit);

	return 0;
}

static int reset_rk3588_line_assert(const struct device *dev, uint32_t id)
{
	rk3588_cru_reset_write(DEVICE_MMIO_GET(dev), RK3588_CRU_SOFTRST_CON(id / 16U), id % 16U,
			       true);

	return 0;
}

static int reset_rk3588_line_deassert(const struct device *dev, uint32_t id)
{
	rk3588_cru_reset_write(DEVICE_MMIO_GET(dev), RK3588_CRU_SOFTRST_CON(id / 16U), id % 16U,
			       false);

	return 0;
}

static int reset_rk3588_line_toggle(const struct device *dev, uint32_t id)
{
	reset_rk3588_line_assert(dev, id);
	reset_rk3588_line_deassert(dev, id);

	return 0;
}

static DEVICE_API(reset, reset_rk3588_driver_api) = {
	.status = reset_rk3588_status,
	.line_assert = reset_rk3588_line_assert,
	.line_deassert = reset_rk3588_line_deassert,
	.line_toggle = reset_rk3588_line_toggle,
};

static int reset_rk3588_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	return 0;
}

static struct reset_rk3588_data rk3588_cru_reset_data;

static const struct reset_rk3588_config reset_rk3588_config = {
	._mmio = {
		.phys_addr = DT_REG_ADDR(DT_INST_PARENT(0)),
		.size = RK3588_CRU_MAP_SIZE,
	},
};

DEVICE_DT_INST_DEFINE(0, reset_rk3588_init, NULL, &rk3588_cru_reset_data, &reset_rk3588_config,
		      PRE_KERNEL_1, CONFIG_RESET_INIT_PRIORITY, &reset_rk3588_driver_api);
