/*
 * Copyright (c) 2026 Dhruv Menon <dhruvmenon1104@gmail.com>
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT ti_sci_intr

#include <errno.h>
#include <stdbool.h>
#include <stdint.h>

#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_sci_intr.h>
#include <zephyr/dt-bindings/interrupt-controller/ti-vim.h>
#include <zephyr/irq.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/util.h>

LOG_MODULE_REGISTER(ti_sci_intr, CONFIG_INTC_LOG_LEVEL);

/* Max <out parent limit> triples accepted from ti,interrupt-ranges. */
#define TI_SCI_INTR_MAX_RANGES 8U
/* Max concurrent routes tracked by this driver. */
#define TI_SCI_INTR_MAX_ROUTES 32U
/* Max IR outputs covered by the host resource-range bitmap. */
#define TI_SCI_INTR_MAX_OUTPUTS 256U

BUILD_ASSERT(TI_SCI_INTR_MAX_ROUTES <= 32U,
	     "route_busy bitmask assumes TI_SCI_INTR_MAX_ROUTES <= 32");

struct ti_sci_intr_route {
	uint16_t input;
	uint16_t output;
	unsigned int parent_irq;
	void (*isr)(const void *arg);
	const void *arg;
};

struct ti_sci_intr_config {
	const struct device *sci;
	uint16_t sci_dev_id;
	uint32_t trigger_flags;
	const uint16_t *range_cells;
	size_t num_range_cells;
};

struct ti_sci_intr_data {
	struct k_spinlock lock;
	uint16_t out_start;
	uint16_t out_num;
	uint32_t route_busy;
	uint32_t output_busy[DIV_ROUND_UP(TI_SCI_INTR_MAX_OUTPUTS, 32U)];
	struct ti_sci_intr_route routes[TI_SCI_INTR_MAX_ROUTES];
};

static inline bool ti_sci_intr_route_is_used(const struct ti_sci_intr_data *data, size_t idx)
{
	return (data->route_busy & BIT(idx)) != 0U;
}

static inline void ti_sci_intr_route_set_used(struct ti_sci_intr_data *data, size_t idx)
{
	data->route_busy |= BIT(idx);
}

static inline void ti_sci_intr_route_clear_used(struct ti_sci_intr_data *data, size_t idx)
{
	data->route_busy &= ~BIT(idx);
}

static inline bool ti_sci_intr_output_is_busy(const struct ti_sci_intr_data *data, uint16_t output)
{
	unsigned int bit = (unsigned int)(output - data->out_start);
	unsigned int word = bit / 32U;
	unsigned int offs = bit % 32U;

	return (data->output_busy[word] & BIT(offs)) != 0U;
}

static inline void ti_sci_intr_output_set_busy(struct ti_sci_intr_data *data, uint16_t output)
{
	unsigned int bit = (unsigned int)(output - data->out_start);
	unsigned int word = bit / 32U;
	unsigned int offs = bit % 32U;

	data->output_busy[word] |= BIT(offs);
}

static inline void ti_sci_intr_output_clear_busy(struct ti_sci_intr_data *data, uint16_t output)
{
	unsigned int bit = (unsigned int)(output - data->out_start);
	unsigned int word = bit / 32U;
	unsigned int offs = bit % 32U;

	data->output_busy[word] &= ~BIT(offs);
}

static struct ti_sci_intr_route *ti_sci_intr_find_input(struct ti_sci_intr_data *data,
							uint16_t input)
{
	for (size_t i = 0; i < ARRAY_SIZE(data->routes); i++) {
		if (ti_sci_intr_route_is_used(data, i) && data->routes[i].input == input) {
			return &data->routes[i];
		}
	}

	return NULL;
}

/*
 * Reject duplicate @p input, then take the first free route slot.
 * On success returns 0 and stores the free slot (not yet marked used).
 */
static int ti_sci_intr_find_slot(struct ti_sci_intr_data *data, uint16_t input,
				 struct ti_sci_intr_route **slot)
{
	struct ti_sci_intr_route *free_slot = NULL;

	for (size_t i = 0; i < ARRAY_SIZE(data->routes); i++) {
		if (ti_sci_intr_route_is_used(data, i) && data->routes[i].input == input) {
			return -EALREADY;
		}
	}

	for (size_t i = 0; i < ARRAY_SIZE(data->routes); i++) {
		if (!ti_sci_intr_route_is_used(data, i)) {
			free_slot = &data->routes[i];
			break;
		}
	}

	if (free_slot == NULL) {
		return -ENOSPC;
	}

	*slot = free_slot;
	return 0;
}

static void ti_sci_intr_clear_route(struct ti_sci_intr_data *data, struct ti_sci_intr_route *route)
{
	size_t idx = (size_t)(route - data->routes);

	ti_sci_intr_output_clear_busy(data, route->output);
	ti_sci_intr_route_clear_used(data, idx);
	*route = (struct ti_sci_intr_route){0};
}

static int ti_sci_intr_alloc_output(const struct ti_sci_intr_config *cfg,
				    struct ti_sci_intr_data *data, uint16_t *output,
				    unsigned int *parent_irq)
{
	static const size_t range_cell_size = 3U;

	for (size_t block_offset = 0; block_offset < cfg->num_range_cells / range_cell_size;
	     block_offset++) {
		size_t i = block_offset * range_cell_size;
		uint16_t out_base = cfg->range_cells[i];
		uint16_t parent_base = cfg->range_cells[i + 1U];
		uint16_t limit = cfg->range_cells[i + 2U];

		for (uint16_t off = 0; off < limit; off++) {
			uint16_t cand = out_base + off;

			if (cand < data->out_start ||
			    cand >= (uint16_t)(data->out_start + data->out_num)) {
				continue;
			}

			if (ti_sci_intr_output_is_busy(data, cand)) {
				continue;
			}

			*output = cand;
			*parent_irq = parent_base + off;
			return 0;
		}
	}

	return -ENOSPC;
}

static int ti_sci_intr_set(const struct ti_sci_intr_config *cfg, uint16_t input, uint16_t output)
{
	struct tisci_irq_set_req req = {
		.valid_params = TISCI_MSG_VALUE_RM_DST_ID_VALID |
				TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};

	return tisci_cmd_rm_irq_set(cfg->sci, &req);
}

static int ti_sci_intr_release(const struct ti_sci_intr_config *cfg, uint16_t input,
			       uint16_t output)
{
	struct tisci_irq_release_req req = {
		.valid_params = TISCI_MSG_VALUE_RM_DST_ID_VALID |
				TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};

	return tisci_cmd_rm_irq_release(cfg->sci, &req);
}

int ti_sci_intr_connect(const struct device *dev, uint16_t input,
			void (*isr)(const void *arg), const void *arg, uint32_t priority,
			uint32_t flags)
{
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	struct ti_sci_intr_route *route;
	k_spinlock_key_t key;
	uint16_t output;
	unsigned int parent_irq;
	uint32_t irq_flags;
	size_t route_idx;
	int ret;

	if (dev == NULL || isr == NULL) {
		return -EINVAL;
	}

	cfg = dev->config;
	data = dev->data;

	if (!device_is_ready(cfg->sci)) {
		return -ENODEV;
	}

	irq_flags = flags;
	if ((irq_flags & (IRQ_TYPE_EDGE | IRQ_TYPE_LEVEL)) == 0U) {
		irq_flags |= cfg->trigger_flags;
	}

	key = k_spin_lock(&data->lock);

	ret = ti_sci_intr_find_slot(data, input, &route);
	if (ret != 0) {
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	ret = ti_sci_intr_alloc_output(cfg, data, &output, &parent_irq);
	if (ret != 0) {
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	route_idx = (size_t)(route - data->routes);
	route->input = input;
	route->output = output;
	route->parent_irq = parent_irq;
	route->isr = isr;
	route->arg = arg;
	ti_sci_intr_output_set_busy(data, output);
	ti_sci_intr_route_set_used(data, route_idx);

	k_spin_unlock(&data->lock, key);

	ret = ti_sci_intr_set(cfg, input, output);
	if (ret != 0) {
		LOG_ERR("TISCI irq set failed for input %u output %u (%d)", input, output,
			ret);
		key = k_spin_lock(&data->lock);
		ti_sci_intr_clear_route(data, route);
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	ret = irq_connect_dynamic(parent_irq, priority, isr, arg, irq_flags);
	if (ret < 0) {
		LOG_ERR("Dynamic connect failed for parent IRQ %u (%d)", parent_irq, ret);
		(void)ti_sci_intr_release(cfg, input, output);
		key = k_spin_lock(&data->lock);
		ti_sci_intr_clear_route(data, route);
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	irq_enable(parent_irq);

	LOG_DBG("Routed input %u -> output %u (parent IRQ %u)", input, output, parent_irq);

	return 0;
}

int ti_sci_intr_disconnect(const struct device *dev, uint16_t input)
{
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	struct ti_sci_intr_route route_copy;
	struct ti_sci_intr_route *route;
	k_spinlock_key_t key;
	size_t route_idx;
	int ret;

	if (dev == NULL) {
		return -EINVAL;
	}

	cfg = dev->config;
	data = dev->data;

	key = k_spin_lock(&data->lock);
	route = ti_sci_intr_find_input(data, input);
	if (route == NULL) {
		k_spin_unlock(&data->lock, key);
		return -ENOENT;
	}

	route_copy = *route;
	route_idx = (size_t)(route - data->routes);
	ti_sci_intr_clear_route(data, route);
	k_spin_unlock(&data->lock, key);

	irq_disable(route_copy.parent_irq);
	z_isr_install(route_copy.parent_irq, z_irq_spurious, NULL);

	ret = ti_sci_intr_release(cfg, route_copy.input, route_copy.output);
	if (ret != 0) {
		LOG_ERR("TISCI irq release failed for input %u output %u (%d)",
			route_copy.input, route_copy.output, ret);

		/* Roll back local state so the caller can retry disconnect. */
		key = k_spin_lock(&data->lock);
		if (!ti_sci_intr_route_is_used(data, route_idx) &&
		    !ti_sci_intr_output_is_busy(data, route_copy.output)) {
			data->routes[route_idx] = route_copy;
			ti_sci_intr_output_set_busy(data, route_copy.output);
			ti_sci_intr_route_set_used(data, route_idx);
			z_isr_install(route_copy.parent_irq, route_copy.isr, route_copy.arg);
			irq_enable(route_copy.parent_irq);
		}
		k_spin_unlock(&data->lock, key);
		return ret;
	}

	return 0;
}

int ti_sci_intr_get_parent_irq(const struct device *dev, uint16_t input, unsigned int *parent_irq)
{
	struct ti_sci_intr_data *data;
	struct ti_sci_intr_route *route;
	k_spinlock_key_t key;

	if (dev == NULL || parent_irq == NULL) {
		return -EINVAL;
	}

	data = dev->data;

	key = k_spin_lock(&data->lock);
	route = ti_sci_intr_find_input(data, input);
	if (route == NULL) {
		k_spin_unlock(&data->lock, key);
		return -ENOENT;
	}

	*parent_irq = route->parent_irq;
	k_spin_unlock(&data->lock, key);

	return 0;
}

static int ti_sci_intr_init(const struct device *dev)
{
	const struct ti_sci_intr_config *cfg = dev->config;
	struct ti_sci_intr_data *data = dev->data;
	uint16_t start = 0;
	uint16_t num = 0;
	int ret;

	if (!device_is_ready(cfg->sci)) {
		LOG_ERR("TISCI device %s is not ready", cfg->sci->name);
		return -ENODEV;
	}

	if (cfg->num_range_cells == 0U || (cfg->num_range_cells % 3U) != 0U) {
		LOG_ERR("Invalid ti,interrupt-ranges cell count %zu",
			cfg->num_range_cells);
		return -EINVAL;
	}

	ret = tisci_cmd_get_resource_range(cfg->sci, cfg->sci_dev_id,
					   TISCI_RESASG_SUBTYPE_IR_OUTPUT,
					   &start, &num);
	if (ret != 0) {
		LOG_ERR("Failed to get IR output range for dev-id %u (%d)",
			cfg->sci_dev_id, ret);
		return ret;
	}

	if (num == 0U) {
		LOG_ERR("No IR outputs assigned for dev-id %u", cfg->sci_dev_id);
		return -ENOSPC;
	}

	if (num > TI_SCI_INTR_MAX_OUTPUTS) {
		LOG_ERR("IR output span %u exceeds driver max %u", num, TI_SCI_INTR_MAX_OUTPUTS);
		return -ENOMEM;
	}

	data->out_start = start;
	data->out_num = num;

	LOG_INF("ti,sci-intr dev-id %u: outputs %u..%u (%u total)", cfg->sci_dev_id, start,
		(uint16_t)(start + num - 1U), num);

	return 0;
}

/*
 * DT binding ti,intr-trigger-type uses Linux style IRQ_TYPE values (1=edge, 4=level).
 * Map those onto Zephyr VIM flags for irq_connect_dynamic().
 */
#define TI_SCI_INTR_TRIGGER_FLAGS(val)                                                             \
	((val) == 1 ? IRQ_TYPE_EDGE : ((val) == 4 ? IRQ_TYPE_LEVEL : 0))

#define TI_SCI_INTR_INIT(n)                                                                        \
	BUILD_ASSERT((DT_INST_PROP_LEN(n, ti_interrupt_ranges) % 3) == 0,                          \
		     "ti,interrupt-ranges must be <out parent limit> triples");                    \
	BUILD_ASSERT((DT_INST_PROP_LEN(n, ti_interrupt_ranges) / 3) <= TI_SCI_INTR_MAX_RANGES,     \
		     "too many ti,interrupt-ranges triples");                                      \
	static const uint16_t ti_sci_intr_range_cells_##n[] =                                      \
		DT_INST_PROP(n, ti_interrupt_ranges);                                              \
	static struct ti_sci_intr_data ti_sci_intr_data_##n;                                       \
	static const struct ti_sci_intr_config ti_sci_intr_cfg_##n = {                             \
		.sci = DEVICE_DT_GET(DT_INST_PHANDLE(n, ti_sci)),                                  \
		.sci_dev_id = DT_INST_PROP(n, ti_sci_dev_id),                                      \
		.trigger_flags =                                                                   \
			TI_SCI_INTR_TRIGGER_FLAGS(DT_INST_PROP_OR(n, ti_intr_trigger_type, 0)),    \
		.range_cells = ti_sci_intr_range_cells_##n,                                        \
		.num_range_cells = ARRAY_SIZE(ti_sci_intr_range_cells_##n),                        \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ti_sci_intr_init, NULL, &ti_sci_intr_data_##n,                    \
			      &ti_sci_intr_cfg_##n, POST_KERNEL, CONFIG_INTC_INIT_PRIORITY, NULL)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_INIT)
