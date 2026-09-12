/* SPDX-License-Identifier: Apache-2.0 */
/*
 * Copyright (c) Dhruv Menon <dhruvmenon1104@gmail.com>
 */

#define DT_DRV_COMPAT ti_k3_intr_router

#include <zephyr/device.h>
#include <zephyr/init.h>
#include <zephyr/kernel.h>
#include <zephyr/spinlock.h>
#include <zephyr/sys/sys_io.h>
#include <zephyr/logging/log.h>
#include <zephyr/drivers/interrupt_controller/ti_k3_intr_router.h>

LOG_MODULE_REGISTER(ti_k3_intr_router, CONFIG_INTC_LOG_LEVEL);

#define INTR_ROUTER_PID_REG                 0x00U
#define INTR_ROUTER_MUXCNTL_REG(n)          (0x04U + ((uint32_t)(n) * 4U))

#define INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT  BIT(16)
#define INTR_ROUTER_MUXCNTL_MUX_CNTL_MASK   0x000000FFU

#define TI_K3_INTR_ROUTER_MAX_OUTPUTS       64U

struct ti_k3_intr_router_config {
	DEVICE_MMIO_NAMED_ROM(reg_base);
	uint16_t num_inputs;
	uint16_t num_outputs;
	uint16_t output_offset;
	const uint16_t *preroutes;
	size_t num_preroutes;
};

struct ti_k3_intr_router_data {
	DEVICE_MMIO_NAMED_RAM(reg_base);
	struct k_spinlock lock;
	uint32_t allocated_mask[DIV_ROUND_UP(TI_K3_INTR_ROUTER_MAX_OUTPUTS, 32)];
};

#define DEV_CFG(dev)  ((const struct ti_k3_intr_router_config *)((dev)->config))
#define DEV_DATA(dev) ((struct ti_k3_intr_router_data *)((dev)->data))

static inline mem_addr_t get_reg_base(const struct device *dev)
{
	return DEVICE_MMIO_NAMED_GET(dev, reg_base);
}

int ti_k3_intr_router_set_route(const struct device *dev, uint16_t output_idx, uint16_t input_idx)
{
	const struct ti_k3_intr_router_config *config;
	struct ti_k3_intr_router_data *data;
	mem_addr_t base;
	k_spinlock_key_t key;
	uint32_t val;

	if (dev == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	if (output_idx >= config->num_outputs || input_idx >= config->num_inputs) {
		return -EINVAL;
	}

	base = get_reg_base(dev);
	val = INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT |
	      (input_idx & INTR_ROUTER_MUXCNTL_MUX_CNTL_MASK);

	key = k_spin_lock(&data->lock);

	sys_write32(val, base + INTR_ROUTER_MUXCNTL_REG(output_idx));

	if (output_idx < TI_K3_INTR_ROUTER_MAX_OUTPUTS) {
		data->allocated_mask[output_idx / 32] |= BIT(output_idx % 32);
	}

	k_spin_unlock(&data->lock, key);

	LOG_DBG("Configured route: Output %u -> Input %u (val: 0x%08x)",
		output_idx, input_idx, val);

	return 0;
}

int ti_k3_intr_router_enable_route(const struct device *dev, uint16_t output_idx)
{
	const struct ti_k3_intr_router_config *config;
	struct ti_k3_intr_router_data *data;
	mem_addr_t base;
	k_spinlock_key_t key;
	uint32_t val;

	if (dev == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	if (output_idx >= config->num_outputs) {
		return -EINVAL;
	}

	base = get_reg_base(dev);

	key = k_spin_lock(&data->lock);

	val = sys_read32(base + INTR_ROUTER_MUXCNTL_REG(output_idx));
	val |= INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT;
	sys_write32(val, base + INTR_ROUTER_MUXCNTL_REG(output_idx));

	if (output_idx < TI_K3_INTR_ROUTER_MAX_OUTPUTS) {
		data->allocated_mask[output_idx / 32] |= BIT(output_idx % 32);
	}

	k_spin_unlock(&data->lock, key);

	return 0;
}

int ti_k3_intr_router_disable_route(const struct device *dev, uint16_t output_idx)
{
	const struct ti_k3_intr_router_config *config;
	struct ti_k3_intr_router_data *data;
	mem_addr_t base;
	k_spinlock_key_t key;
	uint32_t val;

	if (dev == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	if (output_idx >= config->num_outputs) {
		return -EINVAL;
	}

	base = get_reg_base(dev);

	key = k_spin_lock(&data->lock);

	val = sys_read32(base + INTR_ROUTER_MUXCNTL_REG(output_idx));
	val &= ~INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT;
	sys_write32(val, base + INTR_ROUTER_MUXCNTL_REG(output_idx));

	k_spin_unlock(&data->lock, key);

	return 0;
}

int ti_k3_intr_router_get_route(const struct device *dev, uint16_t output_idx,
				uint16_t *input_idx, bool *enabled)
{
	const struct ti_k3_intr_router_config *config;
	mem_addr_t base;
	uint32_t val;

	if (dev == NULL || input_idx == NULL || enabled == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);

	if (output_idx >= config->num_outputs) {
		return -EINVAL;
	}

	base = get_reg_base(dev);
	val = sys_read32(base + INTR_ROUTER_MUXCNTL_REG(output_idx));

	*enabled = (val & INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT) != 0U;
	*input_idx = (uint16_t)(val & INTR_ROUTER_MUXCNTL_MUX_CNTL_MASK);

	return 0;
}

int ti_k3_intr_router_alloc_and_route(const struct device *dev, uint16_t input_idx,
				      uint16_t *allocated_output)
{
	const struct ti_k3_intr_router_config *config;
	struct ti_k3_intr_router_data *data;
	mem_addr_t base;
	k_spinlock_key_t key;
	uint16_t out_idx;
	bool found = false;
	uint32_t val;

	if (dev == NULL || allocated_output == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	if (input_idx >= config->num_inputs) {
		return -EINVAL;
	}

	key = k_spin_lock(&data->lock);

	for (out_idx = config->output_offset; out_idx < config->num_outputs; out_idx++) {
		if (out_idx >= TI_K3_INTR_ROUTER_MAX_OUTPUTS) {
			break;
		}

		if ((data->allocated_mask[out_idx / 32] & BIT(out_idx % 32)) == 0U) {
			data->allocated_mask[out_idx / 32] |= BIT(out_idx % 32);
			found = true;
			break;
		}
	}

	if (!found) {
		k_spin_unlock(&data->lock, key);
		return -ENOSPC;
	}

	base = get_reg_base(dev);
	val = INTR_ROUTER_MUXCNTL_INT_ENABLE_BIT |
	      (input_idx & INTR_ROUTER_MUXCNTL_MUX_CNTL_MASK);

	sys_write32(val, base + INTR_ROUTER_MUXCNTL_REG(out_idx));

	k_spin_unlock(&data->lock, key);

	*allocated_output = out_idx;

	LOG_DBG("Allocated route: Output %u -> Input %u", out_idx, input_idx);

	return 0;
}

int ti_k3_intr_router_free_route(const struct device *dev, uint16_t output_idx)
{
	const struct ti_k3_intr_router_config *config;
	struct ti_k3_intr_router_data *data;
	mem_addr_t base;
	k_spinlock_key_t key;

	if (dev == NULL) {
		return -EINVAL;
	}

	config = DEV_CFG(dev);
	data = DEV_DATA(dev);

	if (output_idx >= config->num_outputs) {
		return -EINVAL;
	}

	base = get_reg_base(dev);

	key = k_spin_lock(&data->lock);

	sys_write32(0U, base + INTR_ROUTER_MUXCNTL_REG(output_idx));

	if (output_idx < TI_K3_INTR_ROUTER_MAX_OUTPUTS) {
		data->allocated_mask[output_idx / 32] &= ~BIT(output_idx % 32);
	}

	k_spin_unlock(&data->lock, key);

	LOG_DBG("Freed route: Output %u", output_idx);

	return 0;
}

static int ti_k3_intr_router_init(const struct device *dev)
{
	const struct ti_k3_intr_router_config *config = DEV_CFG(dev);
	size_t i;

	DEVICE_MMIO_NAMED_MAP(dev, reg_base, K_MEM_CACHE_NONE);

	LOG_INF("TI K3 Interrupt Router (Inputs: %u, Outputs: %u)",
		config->num_inputs, config->num_outputs);

	/* Apply any pre-configured static routes defined in Devicetree */
	for (i = 0; i + 1 < config->num_preroutes; i += 2) {
		uint16_t out_idx = config->preroutes[i];
		uint16_t in_idx = config->preroutes[i + 1];

		ti_k3_intr_router_set_route(dev, out_idx, in_idx);
	}

	return 0;
}

#define TI_K3_INTR_ROUTER_INIT(n)                                                                  \
	static const uint16_t preroutes_##n[] =                                                    \
		COND_CODE_1(DT_INST_NODE_HAS_PROP(n, ti_preroutes),                                \
			    (DT_INST_PROP(n, ti_preroutes)), ({}));                                \
                                                                                                   \
	static const struct ti_k3_intr_router_config ti_k3_intr_router_cfg_##n = {                 \
		DEVICE_MMIO_NAMED_ROM_INIT(reg_base, DT_DRV_INST(n)),                             \
		.num_inputs = DT_INST_PROP(n, ti_num_inputs),                                     \
		.num_outputs = DT_INST_PROP(n, ti_num_outputs),                                   \
		.output_offset = DT_INST_PROP_OR(n, ti_output_offset, 0),                          \
		.preroutes = preroutes_##n,                                                        \
		.num_preroutes = ARRAY_SIZE(preroutes_##n),                                       \
	};                                                                                         \
                                                                                                   \
	static struct ti_k3_intr_router_data ti_k3_intr_router_data_##n;                           \
                                                                                                   \
	DEVICE_DT_INST_DEFINE(n, &ti_k3_intr_router_init, NULL, &ti_k3_intr_router_data_##n,       \
			      &ti_k3_intr_router_cfg_##n, PRE_KERNEL_1,                           \
			      CONFIG_INTC_INIT_PRIORITY, NULL)

DT_INST_FOREACH_STATUS_OKAY(TI_K3_INTR_ROUTER_INIT)
