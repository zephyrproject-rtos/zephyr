/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * NXP MCUX XBAR (cross-bar switch) MUX driver.
 *
 * Cell layout:
 *
 *   cells[0] = "output" -- xbar output signal id (which SEL register to
 *                          program; addressing).
 *   state              -- xbar input signal id (the value written into
 *                          the SEL register).
 */

#define DT_DRV_COMPAT nxp_mcux_xbar

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>
#include <zephyr/sys/util_macro.h>

#include <fsl_common.h>

LOG_MODULE_REGISTER(mux_nxp_xbar, CONFIG_MUX_LOG_LEVEL);

#if defined(FSL_FEATURE_XBAR_DSC_REG_WIDTH) && (FSL_FEATURE_XBAR_DSC_REG_WIDTH == 32)
#define NXP_XBAR_REG_WIDTH 32
#define NXP_XBAR_SEL_MASK  0x1FFU
#define NXP_XBAR_REG_READ(base, offset)         sys_read32((base) + (offset))
#define NXP_XBAR_REG_WRITE(base, offset, value) sys_write32((value), (base) + (offset))
#else
#define NXP_XBAR_REG_WIDTH 16
#define NXP_XBAR_SEL_MASK  0xFFU
#define NXP_XBAR_REG_READ(base, offset)         sys_read16((base) + (offset))
#define NXP_XBAR_REG_WRITE(base, offset, value) sys_write16((value), (base) + (offset))
#endif

struct mux_nxp_xbar_config {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
};

struct mux_nxp_xbar_data {
	DEVICE_MMIO_RAM;
	struct k_spinlock lock;
};

static void nxp_xbar_calc_sel_offset_shift(uint32_t output, uint32_t *offset,
					   uint32_t *shift)
{
#if (NXP_XBAR_REG_WIDTH == 32)
	/* 32-bit SEL register: one output per register. */
	*offset = output * sizeof(uint32_t);
	*shift  = 0;
#else
	/* 16-bit SEL register: two outputs packed in each register, low byte
	 * for even outputs and high byte for odd ones.
	 */
	*offset = (output / 2) * sizeof(uint16_t);
	*shift  = (output % 2) * 8;
#endif
}

static int mux_nxp_xbar_set(const struct device *dev,
			    const struct mux_control *control, uint32_t state)
{
	struct mux_nxp_xbar_data *data = dev->data;
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t output;
	uint32_t sel_reg_offset;
	uint32_t sel_reg_val;
	uint32_t shift;
	k_spinlock_key_t key;

	/* control->len is fixed at 1 by the nxp,mcux-xbar binding
	 * (#mux-control-cells = const: 1), enforced at DT validation.
	 */
	output = control->cells[0];

	nxp_xbar_calc_sel_offset_shift(output, &sel_reg_offset, &shift);

	key = k_spin_lock(&data->lock);

	sel_reg_val  = NXP_XBAR_REG_READ(base, sel_reg_offset);
	sel_reg_val &= ~(NXP_XBAR_SEL_MASK << shift);
	sel_reg_val |= (state & NXP_XBAR_SEL_MASK) << shift;
	NXP_XBAR_REG_WRITE(base, sel_reg_offset, sel_reg_val);

	k_spin_unlock(&data->lock, key);

	LOG_DBG("xbar routed: out=%u <- in=%u", output, state);

	return 0;
}

static DEVICE_API(mux_control, mux_nxp_xbar_driver_api) = {
	.set       = mux_nxp_xbar_set,
};

static int mux_nxp_xbar_init(const struct device *dev)
{
	const struct mux_nxp_xbar_config *cfg = dev->config;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(cfg->clock_dev)) {
		LOG_ERR("clock controller %s not ready", cfg->clock_dev->name);
		return -ENODEV;
	}

	ret = clock_control_on(cfg->clock_dev, cfg->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable XBAR clock: %d", ret);
		return ret;
	}

	return 0;
}

#define MUX_NXP_XBAR_INIT(n)                                                       \
	static const struct mux_nxp_xbar_config mux_nxp_xbar_cfg_##n = {           \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                              \
		.clock_dev    = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(n)),             \
		.clock_subsys = (clock_control_subsys_t)                           \
				DT_INST_CLOCKS_CELL(n, name),                      \
	};                                                                         \
	static struct mux_nxp_xbar_data mux_nxp_xbar_data_##n;                     \
	DEVICE_DT_INST_DEFINE(n, mux_nxp_xbar_init, NULL,                          \
			      &mux_nxp_xbar_data_##n, &mux_nxp_xbar_cfg_##n,       \
			      POST_KERNEL, CONFIG_MUX_INIT_PRIORITY,               \
			      &mux_nxp_xbar_driver_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_NXP_XBAR_INIT)
