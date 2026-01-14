/*
 * Copyright 2026 NXP
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT nxp_xbar

#include <zephyr/device.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/util_macro.h>
#include <zephyr/drivers/misc/interconn/nxp_xbar/nxp_xbar.h>
#include <zephyr/drivers/clock_control.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(nxp_xbar, CONFIG_NXP_XBAR_LOG_LEVEL);

/* Register bit definitions */
#define NXP_XBAR_CTRL_STAT_SHIFT    4U
#define NXP_XBAR_CTRL_STAT_MASK     BIT(4)
#define NXP_XBAR_CTRL_DEN_SHIFT     0U
#define NXP_XBAR_CTRL_DEN_MASK      0x3U
#define NXP_XBAR_CTRL_EDGE_SHIFT    2U
#define NXP_XBAR_CTRL_EDGE_MASK     (0x3U << NXP_XBAR_CTRL_EDGE_SHIFT)
#define NXP_XBAR_CTRL_WP_MASK       BIT(31)

#if defined(FSL_FEATURE_XBAR_DSC_REG_WIDTH) && (FSL_FEATURE_XBAR_DSC_REG_WIDTH == 32)
#define NXP_XBAR_REG_WIDTH          32
#define NXP_XBAR_SEL_MASK           0x1FFU
#define NXP_XBAR_CTRL_ALL_STAT_MASK (NXP_XBAR_CTRL_STAT_MASK)
#define NXP_XBAR_REG_READ(base, offset) sys_read32((base) + (offset))
#define NXP_XBAR_REG_WRITE(base, offset, value) sys_write32((value), (base) + (offset))
#else
#define NXP_XBAR_REG_WIDTH          16
#define NXP_XBAR_SEL_MASK           0xFFU
#define NXP_XBAR_CTRL_ALL_STAT_MASK (NXP_XBAR_CTRL_STAT_MASK | (NXP_XBAR_CTRL_STAT_MASK << 8U))
#define NXP_XBAR_REG_READ(base, offset) sys_read16((base) + (offset))
#define NXP_XBAR_REG_WRITE(base, offset, value) sys_write16((value), (base) + (offset))
#endif

struct nxp_xbar_config {
	DEVICE_MMIO_ROM;
	const struct device *clock_dev;
	clock_control_subsys_t clock_subsys;
	uint16_t num_outputs;
	uint16_t num_inputs;
	uint8_t num_ctrl_regs;
	bool has_write_protect;
};

struct nxp_xbar_data {
	DEVICE_MMIO_RAM;
	struct k_mutex lock;
};

static void mcux_xbar_calc_sel_offset_shift(uint32_t output, uint32_t *offset, uint32_t *shift)
{
#if (NXP_XBAR_REG_WIDTH == 32)
	/* 32-bit registers: one output per register */
	*offset = output * sizeof(uint32_t);
	*shift = 0;
#else
	/* 16-bit registers: two outputs per register */
	*offset = (output / 2) * sizeof(uint16_t);
	*shift = (output % 2) * 8;
#endif
}

static void mcux_xbar_calc_ctrl_offset_shift(uint16_t num_outputs, uint32_t output,
					 uint32_t *offset, uint32_t *shift)
{
#if (NXP_XBAR_REG_WIDTH == 32)
	/* 32-bit registers: calculate CTRL offset after SEL registers */
	*offset = (num_outputs * sizeof(uint32_t)) + (output * sizeof(uint32_t));
	*shift = 0;
#else
	/* 16-bit registers: calculate CTRL offset after SEL registers */
	*offset = (DIV_ROUND_UP(num_outputs, 2) * sizeof(uint16_t)) +
		  ((output / 2) * sizeof(uint16_t));
	*shift = (output % 2) * 8;
#endif
}

static int mcux_xbar_set_connection(const struct device *dev, uint32_t output, uint32_t input)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t sel_reg_offset;
	uint32_t sel_reg_val;
	uint32_t shift;

	if (output >= config->num_outputs || input >= config->num_inputs) {
		LOG_ERR("Invalid output (%u) or input (%u)", output, input);
		return -EINVAL;
	}

	mcux_xbar_calc_sel_offset_shift(output, &sel_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	sel_reg_val = NXP_XBAR_REG_READ(base, sel_reg_offset);
	sel_reg_val &= ~(NXP_XBAR_SEL_MASK << shift);
	sel_reg_val |= (input << shift);
	NXP_XBAR_REG_WRITE(base, sel_reg_offset, sel_reg_val);

	k_mutex_unlock(&data->lock);

	LOG_DBG("Set connection: output=%u, input=%u", output, input);

	return 0;
}

static int mcux_xbar_check_output_for_ctrl_reg(uint8_t num_ctrl_regs, uint32_t output)
{
	uint32_t max_ctrl_outputs;
#if (NXP_XBAR_REG_WIDTH == 32)
	/* 32-bit registers: one output per control register */
	max_ctrl_outputs = num_ctrl_regs;
#else
	/* 16-bit registers: two outputs per control register */
	max_ctrl_outputs = num_ctrl_regs * 2;
#endif

	if (output >= max_ctrl_outputs) {
		LOG_ERR("Invalid output: %u (max: %u)", output, max_ctrl_outputs - 1);
		return -EINVAL;
	}

	return 0;
}

static int mcux_xbar_set_output_config(const struct device *dev, uint32_t output,
				     const struct nxp_xbar_control_config *ctrl_config)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl_reg_offset;
	uint32_t ctrl_reg_val;
	uint32_t shift;
	uint32_t ctrl_value;

	if (mcux_xbar_check_output_for_ctrl_reg(config->num_ctrl_regs, output) != 0) {
		return -EINVAL;
	}

	/* Set new configuration */
	ctrl_value = ((ctrl_config->active_edge << NXP_XBAR_CTRL_EDGE_SHIFT) |
		     (ctrl_config->request_type << NXP_XBAR_CTRL_DEN_SHIFT));

	mcux_xbar_calc_ctrl_offset_shift(config->num_outputs, output, &ctrl_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	ctrl_reg_val = NXP_XBAR_REG_READ(base, ctrl_reg_offset);
	ctrl_reg_val &= ~NXP_XBAR_CTRL_ALL_STAT_MASK;
	/* Clear the DMA/IRQ enable and edge bits for this output */
	ctrl_reg_val &= ~((NXP_XBAR_CTRL_DEN_MASK | NXP_XBAR_CTRL_EDGE_MASK) << shift);
	ctrl_reg_val |= (ctrl_value << shift);

	NXP_XBAR_REG_WRITE(base, ctrl_reg_offset, ctrl_reg_val);

	k_mutex_unlock(&data->lock);

	LOG_DBG("Set output config: output=%u, edge=%u, request=%u",
		output, ctrl_config->active_edge, ctrl_config->request_type);

	return 0;
}

static int mcux_xbar_get_status_flag(const struct device *dev, uint32_t output, bool *flag)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl_reg_offset;
	uint32_t ctrl_reg_val;
	uint32_t shift;

	if (mcux_xbar_check_output_for_ctrl_reg(config->num_ctrl_regs, output) != 0) {
		return -EINVAL;
	}

	if (flag == NULL) {
		return -EINVAL;
	}

	mcux_xbar_calc_ctrl_offset_shift(config->num_outputs, output, &ctrl_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	ctrl_reg_val = NXP_XBAR_REG_READ(base, ctrl_reg_offset);
	*flag = (ctrl_reg_val & (NXP_XBAR_CTRL_STAT_MASK << shift)) != 0;

	k_mutex_unlock(&data->lock);

	return 0;
}

static int mcux_xbar_clear_status_flag(const struct device *dev, uint32_t output)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl_reg_offset;
	uint32_t ctrl_reg_val;
	uint32_t shift;

	if (mcux_xbar_check_output_for_ctrl_reg(config->num_ctrl_regs, output) != 0) {
		return -EINVAL;
	}

	mcux_xbar_calc_ctrl_offset_shift(config->num_outputs, output, &ctrl_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	ctrl_reg_val = NXP_XBAR_REG_READ(base, ctrl_reg_offset);
	ctrl_reg_val &= ~NXP_XBAR_CTRL_ALL_STAT_MASK;
	ctrl_reg_val |= (NXP_XBAR_CTRL_STAT_MASK << shift);
	NXP_XBAR_REG_WRITE(base, ctrl_reg_offset, ctrl_reg_val);

	k_mutex_unlock(&data->lock);

	LOG_DBG("Clear status flag: output=%u", output);

	return 0;
}

#if defined(CONFIG_NXP_XBAR_WRITE_PROTECT)
/* Write protection is only supported for 32-bit register width */
BUILD_ASSERT(NXP_XBAR_REG_WIDTH == 32,
	     "Write protection is only supported for 32-bit XBAR registers");
static int mcux_xbar_lock_sel_reg(const struct device *dev, uint32_t output)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t sel_reg_offset;
	uint32_t shift;
	uint32_t sel_reg_val;

	if (!config->has_write_protect) {
		LOG_ERR("Write protection not supported");
		return -ENOTSUP;
	}

	if (mcux_xbar_check_output_for_ctrl_reg(config->num_ctrl_regs, output) != 0) {
		return -EINVAL;
	}

	mcux_xbar_calc_sel_offset_shift(output, &sel_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	sel_reg_val = NXP_XBAR_REG_READ(base, sel_reg_offset);
	sel_reg_val |= NXP_XBAR_CTRL_WP_MASK;
	NXP_XBAR_REG_WRITE(base, sel_reg_offset, sel_reg_val);

	k_mutex_unlock(&data->lock);

	LOG_DBG("Lock SEL register: output=%u", output);

	return 0;
}

static int mcux_xbar_lock_ctrl_reg(const struct device *dev, uint32_t output)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	uintptr_t base = DEVICE_MMIO_GET(dev);
	uint32_t ctrl_reg_offset;
	uint32_t shift;
	uint32_t ctrl_reg_val;

	if (!config->has_write_protect) {
		LOG_ERR("Write protection not supported");
		return -ENOTSUP;
	}

	if (mcux_xbar_check_output_for_ctrl_reg(config->num_ctrl_regs, output) != 0) {
		return -EINVAL;
	}

	mcux_xbar_calc_ctrl_offset_shift(config->num_outputs, output, &ctrl_reg_offset, &shift);

	k_mutex_lock(&data->lock, K_FOREVER);

	ctrl_reg_val = NXP_XBAR_REG_READ(base, ctrl_reg_offset);
	ctrl_reg_val &= ~NXP_XBAR_CTRL_ALL_STAT_MASK;
	ctrl_reg_val |= (NXP_XBAR_CTRL_WP_MASK << shift);
	NXP_XBAR_REG_WRITE(base, ctrl_reg_offset, ctrl_reg_val);

	k_mutex_unlock(&data->lock);

	LOG_DBG("Lock CTRL register: output=%u", output);

	return 0;
}
#endif /* CONFIG_NXP_XBAR_WRITE_PROTECT */

static int nxp_xbar_init(const struct device *dev)
{
	const struct nxp_xbar_config *config = dev->config;
	struct nxp_xbar_data *data = dev->data;
	int ret;

	DEVICE_MMIO_MAP(dev, K_MEM_CACHE_NONE);

	if (!device_is_ready(config->clock_dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	ret = clock_control_on(config->clock_dev, config->clock_subsys);
	if (ret < 0) {
		LOG_ERR("Failed to enable clock: %d", ret);
		return ret;
	}

	k_mutex_init(&data->lock);

	LOG_INF("XBAR initialized: %u outputs, %u inputs",
		config->num_outputs, config->num_inputs);

	return 0;
}

static const struct nxp_xbar_driver_api nxp_xbar_driver_api = {
	.set_connection = mcux_xbar_set_connection,
	.set_output_config = mcux_xbar_set_output_config,
	.get_status_flag = mcux_xbar_get_status_flag,
	.clear_status_flag = mcux_xbar_clear_status_flag,
#if defined(CONFIG_NXP_XBAR_WRITE_PROTECT)
	.lock_sel_reg = mcux_xbar_lock_sel_reg,
	.lock_ctrl_reg = mcux_xbar_lock_ctrl_reg,
#endif
};

#define NXP_XBAR_INIT(inst)                                                                        \
	static const struct nxp_xbar_config nxp_xbar_config_##inst = {                             \
		DEVICE_MMIO_ROM_INIT(DT_DRV_INST(inst)),                                           \
		.clock_dev = DEVICE_DT_GET(DT_INST_CLOCKS_CTLR(inst)),                             \
		.clock_subsys = (clock_control_subsys_t)DT_INST_CLOCKS_CELL(inst, name),           \
		.num_outputs = DT_INST_PROP(inst, num_outputs),                                    \
		.num_inputs = DT_INST_PROP(inst, num_inputs),                                      \
		.num_ctrl_regs = DT_INST_PROP(inst, num_ctrl_regs),                                \
		.has_write_protect = DT_INST_PROP_OR(inst, has_write_protect, false),              \
	};                                                                                         \
	static struct nxp_xbar_data nxp_xbar_data_##inst;                                          \
	DEVICE_DT_INST_DEFINE(inst, nxp_xbar_init, NULL,                                           \
			     &nxp_xbar_data_##inst, &nxp_xbar_config_##inst,                       \
			     PRE_KERNEL_1, CONFIG_NXP_XBAR_INIT_PRIORITY,	\
			     &nxp_xbar_driver_api);

DT_INST_FOREACH_STATUS_OKAY(NXP_XBAR_INIT)
