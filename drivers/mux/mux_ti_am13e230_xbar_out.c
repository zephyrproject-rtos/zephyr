/*
 * SPDX-FileCopyrightText: 2026 Texas Instruments Incorporated
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_am13e230_xbar_out

#include <errno.h>

#include <zephyr/device.h>
#include <zephyr/drivers/mux.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/device_mmio.h>
#include <zephyr/sys/util.h>

#include <zephyr/dt-bindings/mux/ti_am13e230_xbar_out.h>

LOG_MODULE_REGISTER(mux_ti_am13e230_xbar_out, CONFIG_MUX_LOG_LEVEL);

#define AM13E_OUTPUT_XBAR_NUM_OUTPUTS  8U
#define AM13E_OUTPUT_XBAR_STRIDE       0x40U
#define AM13E_OUTPUT_XBAR_G0SEL_OFFSET 0x100U
#define AM13E_OUTPUT_XBAR_G1SEL_OFFSET 0x104U

#define AM13E_OUTPUT_XBAR_GSEL_ADDR(base, idx, group)                                              \
	((base) +                                                                                  \
	 ((group) == AM13E230_XBAR_OUT_GROUP_G0 ? AM13E_OUTPUT_XBAR_G0SEL_OFFSET                   \
						: AM13E_OUTPUT_XBAR_G1SEL_OFFSET) +                \
	 ((idx) * AM13E_OUTPUT_XBAR_STRIDE))

struct am13e230_xbar_config {
	DEVICE_MMIO_ROM;
};

struct am13e230_xbar_data {
	DEVICE_MMIO_RAM;
};

static int am13e230_xbar_set(const struct device *dev, const struct mux_control *control,
			     uint32_t state)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl = control->cells[0];
	uint32_t idx = AM13E230_XBAR_OUT_CTRL_IDX(ctrl);
	uint32_t group = AM13E230_XBAR_OUT_CTRL_GROUP(ctrl);
	uint32_t bit = AM13E230_XBAR_OUT_CTRL_BIT(ctrl);
	uint32_t addr;
	uint32_t reg_val;

	if (idx >= AM13E_OUTPUT_XBAR_NUM_OUTPUTS) {
		LOG_ERR("output xbar index %u out of range", idx);
		return -EINVAL;
	}

	if (state > 1U) {
		LOG_ERR("output xbar state %u must be 0 or 1", state);
		return -EINVAL;
	}

	addr = AM13E_OUTPUT_XBAR_GSEL_ADDR(base, idx, group);
	reg_val = sys_read32(addr);

	if (state == 1U) {
		reg_val |= BIT(bit);
	} else {
		reg_val &= ~BIT(bit);
	}

	sys_write32(reg_val, addr);

	LOG_DBG("output xbar %u group %u bit %u -> %u", idx, group, bit, state);

	return 0;
}

static int am13e230_xbar_get_state(const struct device *dev, const struct mux_control *control,
				   uint32_t *state)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl = control->cells[0];
	uint32_t idx = AM13E230_XBAR_OUT_CTRL_IDX(ctrl);
	uint32_t group = AM13E230_XBAR_OUT_CTRL_GROUP(ctrl);
	uint32_t bit = AM13E230_XBAR_OUT_CTRL_BIT(ctrl);
	uint32_t addr;

	if (idx >= AM13E_OUTPUT_XBAR_NUM_OUTPUTS) {
		LOG_ERR("output xbar index %u out of range", idx);
		return -EINVAL;
	}

	addr = AM13E_OUTPUT_XBAR_GSEL_ADDR(base, idx, group);
	*state = (sys_read32(addr) >> bit) & 1U;

	return 0;
}

static int am13e230_xbar_disconnect(const struct device *dev, const struct mux_control *control)
{
	uint32_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl = control->cells[0];
	uint32_t idx = AM13E230_XBAR_OUT_CTRL_IDX(ctrl);

	if (idx >= AM13E_OUTPUT_XBAR_NUM_OUTPUTS) {
		LOG_ERR("output xbar index %u out of range", idx);
		return -EINVAL;
	}

	sys_write32(0U, AM13E_OUTPUT_XBAR_GSEL_ADDR(base, idx, AM13E230_XBAR_OUT_GROUP_G0));
	sys_write32(0U, AM13E_OUTPUT_XBAR_GSEL_ADDR(base, idx, AM13E230_XBAR_OUT_GROUP_G1));

	LOG_DBG("output xbar %u disconnected (all sources cleared)", idx);

	return 0;
}

static DEVICE_API(mux_control, am13e230_xbar_api) = {
	.set = am13e230_xbar_set,
	.get_state = am13e230_xbar_get_state,
	.disconnect = am13e230_xbar_disconnect,
};

static int am13e230_xbar_init(const struct device *dev)
{
	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);
	return 0;
}

#define MUX_TI_AM13E230_XBAR_OUT_INIT(n)                                                           \
	static const struct am13e230_xbar_config am13e230_xbar_cfg##n = {                          \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(n)),                                              \
	};                                                                                         \
	static struct am13e230_xbar_data am13e230_xbar_data_##n;                                   \
	DEVICE_DT_INST_DEFINE(n, am13e230_xbar_init, NULL, &am13e230_xbar_data_##n,                \
			      &am13e230_xbar_cfg##n, POST_KERNEL, CONFIG_MUX_INIT_PRIORITY,        \
			      &am13e230_xbar_api);

DT_INST_FOREACH_STATUS_OKAY(MUX_TI_AM13E230_XBAR_OUT_INIT)
