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
#include <zephyr/devicetree/interrupt_controller.h>
#include <zephyr/drivers/firmware/tisci/tisci.h>
#include <zephyr/drivers/interrupt_controller/intc_ti_sci_intr.h>
#include <zephyr/dt-bindings/interrupt-controller/ti-vim.h>
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/util.h>

#include "sw_isr_common.h"

LOG_MODULE_REGISTER(ti_sci_intr, CONFIG_INTC_LOG_LEVEL);

/* Max <out parent limit> triples accepted from ti,interrupt-ranges. */
#define TI_SCI_INTR_MAX_RANGES 8U
/* Max concurrent routes tracked by the dynamic connect() API. */
#define TI_SCI_INTR_MAX_ROUTES 32U
/* Max IR outputs covered by the host resource-range bitmap. */
#define TI_SCI_INTR_MAX_OUTPUTS 256U
#define TI_SCI_INTR_INPUT_NONE  UINT16_MAX

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

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)

/*
 * One host-side IR output (ti,sci-intr-out child). Peripherals name it as
 * interrupt-parent and pass the IR input in interrupts. irq_enable() programs
 * TISCI and unmasks the parent VIM line; the demux ISR dispatches the L2 client.
 */
struct ti_sci_intr_out {
	const struct device *router;
	uint16_t output;
	unsigned int vim_irq;
	uint32_t vim_flags;
	uint16_t active_input;
	bool routed;
};

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && ti_sci_intr_out */

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
	struct tisci_irq_release_req rel = {
		.valid_params = TISCI_MSG_VALUE_RM_DST_ID_VALID |
				TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};
	int ret;

	/* Drop a stale Linux/SYSFW route on this input before claiming it. */
	(void)tisci_cmd_rm_irq_release(cfg->sci, &rel);
	ret = tisci_cmd_rm_irq_set(cfg->sci, &req);
	if (ret != 0) {
		rel.valid_params = 0;
		(void)tisci_cmd_rm_irq_release(cfg->sci, &rel);
		ret = tisci_cmd_rm_irq_set(cfg->sci, &req);
	}

	return ret;
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

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)

#define TI_SCI_INTR_OUT_ENTRY(child)                                                               \
	{                                                                                          \
		.router = DEVICE_DT_GET(DT_PARENT(child)),                                         \
		.output = DT_REG_ADDR(child),                                                      \
		.vim_irq = DT_IRQ(child, irq),                                                     \
		.vim_flags = DT_IRQ(child, flags),                                                 \
		.active_input = TI_SCI_INTR_INPUT_NONE,                                            \
		.routed = false,                                                                   \
	},

#define TI_SCI_INTR_OUT_ENTRIES_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_ENTRY)

static struct ti_sci_intr_out ti_sci_intr_outs[] = {
	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_ENTRIES_INST)
};

/*
 * Map each child to its slot in ti_sci_intr_outs[]. Assumes a single
 * ti,sci-intr instance contributes all outs (true on J722S Main R5).
 */
#define TI_SCI_INTR_OUT_IDX(child) DT_NODE_CHILD_IDX(child)

struct ti_sci_intr_pending {
	unsigned int irq;
	struct k_work_delayable work;
};

static struct ti_sci_intr_pending ti_sci_intr_pending_enables[ARRAY_SIZE(ti_sci_intr_outs)];
static struct k_work_delayable ti_sci_intr_route_log_work;
static bool ti_sci_intr_route_log_scheduled;

static void ti_sci_intr_irq_enable_inner(unsigned int irq, bool from_work);

static void ti_sci_intr_pending_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_sci_intr_pending *pending =
		CONTAINER_OF(dwork, struct ti_sci_intr_pending, work);

	ti_sci_intr_irq_enable_inner(pending->irq, true);
}

static void ti_sci_intr_route_log_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	bool any = false;

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = &ti_sci_intr_outs[i];

		if (out->routed && out->active_input != TI_SCI_INTR_INPUT_NONE) {
			printk("ti_sci_intr: IR in %u -> out %u (VIM %u)\n", out->active_input,
			       out->output, out->vim_irq);
			any = true;
		}
	}

	if (!any) {
		printk("ti_sci_intr: no active IR routes\n");
	}
}

void ti_sci_intr_log_routes(void)
{
	ti_sci_intr_route_log_handler(NULL);
}

static void ti_sci_intr_schedule_route_log(void)
{
	if (ti_sci_intr_route_log_scheduled) {
		return;
	}

	ti_sci_intr_route_log_scheduled = true;
	k_work_init_delayable(&ti_sci_intr_route_log_work, ti_sci_intr_route_log_handler);
	(void)k_work_schedule(&ti_sci_intr_route_log_work, K_MSEC(800));
}

static void ti_sci_intr_demux(const void *arg)
{
	struct ti_sci_intr_out *out = (struct ti_sci_intr_out *)arg;
	unsigned int zephyr_irq;
	uint32_t table_idx;

	if (out == NULL || out->active_input == TI_SCI_INTR_INPUT_NONE) {
		return;
	}

	zephyr_irq = irq_to_level_2(out->active_input) | out->vim_irq;
	table_idx = z_get_sw_isr_table_idx(zephyr_irq);

#if defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
	_sw_isr_table[table_idx].isr(_sw_isr_table[table_idx].arg);
#elif defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
	struct _isr_table_entry entry;

	get_isr_entry(table_idx, &entry);
	entry.isr(entry.arg);
#endif
}

static struct ti_sci_intr_out *ti_sci_intr_out_by_vim(unsigned int vim_irq)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		if (ti_sci_intr_outs[i].vim_irq == vim_irq) {
			return &ti_sci_intr_outs[i];
		}
	}

	return NULL;
}

static struct ti_sci_intr_out *ti_sci_intr_out_by_output(const struct device *router,
							uint16_t output)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		if (ti_sci_intr_outs[i].router == router &&
		    ti_sci_intr_outs[i].output == output) {
			return &ti_sci_intr_outs[i];
		}
	}

	return NULL;
}

void ti_sci_intr_irq_enable(unsigned int irq)
{
	if (irq_get_level(irq) == 1U) {
		z_vim_irq_enable(irq);
		return;
	}

	ti_sci_intr_irq_enable_inner(irq, false);
}

static void ti_sci_intr_irq_enable_inner(unsigned int irq, bool from_work)
{
	unsigned int vim_irq = irq_parent_level_2(irq);
	uint16_t input = (uint16_t)irq_from_level_2(irq);
	struct ti_sci_intr_out *out;
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	size_t out_idx;
	int ret;

	out = ti_sci_intr_out_by_vim(vim_irq);
	if (out == NULL || out->router == NULL) {
		LOG_ERR("No ti,sci-intr-out for VIM %u", vim_irq);
		return;
	}

	out_idx = (size_t)(out - ti_sci_intr_outs);
	cfg = out->router->config;
	data = out->router->data;

	if (!device_is_ready(out->router) || !device_is_ready(cfg->sci)) {
		if (!from_work && out_idx < ARRAY_SIZE(ti_sci_intr_pending_enables)) {
			struct ti_sci_intr_pending *pending = &ti_sci_intr_pending_enables[out_idx];

			pending->irq = irq;
			k_work_init_delayable(&pending->work, ti_sci_intr_pending_work_handler);
			(void)k_work_schedule(&pending->work, K_MSEC(500));
			LOG_DBG("Deferring IR in %u enable (TISCI not ready)", input);
		} else {
			LOG_ERR("Router/TISCI not ready for IR in %u", input);
		}
		return;
	}

	key = k_spin_lock(&data->lock);
	if (out->active_input != TI_SCI_INTR_INPUT_NONE && out->active_input != input) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("IR out %u busy (in %u), rejecting in %u", out->output, out->active_input,
			input);
		return;
	}
	out->active_input = input;
	ti_sci_intr_output_set_busy(data, out->output);
	k_spin_unlock(&data->lock, key);

	if (!out->routed) {
		ret = ti_sci_intr_set(cfg, input, out->output);
		if (ret != 0) {
			LOG_ERR("TISCI irq set IR in %u -> out %u failed (%d)", input, out->output,
				ret);
			key = k_spin_lock(&data->lock);
			out->active_input = TI_SCI_INTR_INPUT_NONE;
			ti_sci_intr_output_clear_busy(data, out->output);
			k_spin_unlock(&data->lock, key);
			return;
		}
		out->routed = true;
		printk("ti_sci_intr: IR in %u -> out %u (VIM %u)\n", input, out->output, vim_irq);
		ti_sci_intr_schedule_route_log();
	}

	z_vim_irq_enable(vim_irq);
}

void ti_sci_intr_irq_disable(unsigned int irq)
{
	unsigned int vim_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;

	if (irq_get_level(irq) == 1U) {
		z_vim_irq_disable(irq);
		return;
	}

	vim_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_vim(vim_irq);
	if (out == NULL || out->router == NULL) {
		return;
	}

	if (out->active_input != input) {
		return;
	}

	cfg = out->router->config;
	data = out->router->data;

	z_vim_irq_disable(vim_irq);

	if (out->routed) {
		(void)ti_sci_intr_release(cfg, input, out->output);
		out->routed = false;
	}

	key = k_spin_lock(&data->lock);
	out->active_input = TI_SCI_INTR_INPUT_NONE;
	ti_sci_intr_output_clear_busy(data, out->output);
	k_spin_unlock(&data->lock, key);
}

int ti_sci_intr_irq_is_enabled(unsigned int irq)
{
	unsigned int vim_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;

	if (irq_get_level(irq) == 1U) {
		return z_vim_irq_is_enabled(irq);
	}

	vim_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_vim(vim_irq);

	if (out == NULL || out->active_input != input || !out->routed) {
		return 0;
	}

	return z_vim_irq_is_enabled(vim_irq);
}

void ti_sci_intr_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	unsigned int vim_irq;
	struct ti_sci_intr_out *out;
	uint32_t vim_flags = flags;

	if (irq_get_level(irq) == 1U) {
		z_vim_irq_priority_set(irq, prio, flags);
		return;
	}

	vim_irq = irq_parent_level_2(irq);
	out = ti_sci_intr_out_by_vim(vim_irq);

	if (out != NULL && (vim_flags & (IRQ_TYPE_EDGE | IRQ_TYPE_LEVEL)) == 0U) {
		vim_flags = out->vim_flags;
	}

	z_vim_irq_priority_set(vim_irq, prio, vim_flags);
}

#define TI_SCI_INTR_OUT_PARENT_ENTRY(child)                                                        \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(ti_sci_intr_agg_, DT_NODE_FULL_NAME_TOKEN(child)), NULL,    \
				DT_IRQN(child), INTC_CHILD_ISR_TBL_OFFSET(child),                  \
				DT_INTC_GET_AGGREGATOR_LEVEL(child));

#define TI_SCI_INTR_OUT_PARENT_ENTRIES_INST(n)                                                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_PARENT_ENTRY)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_PARENT_ENTRIES_INST)

#define TI_SCI_INTR_OUT_CONNECT(child)                                                             \
	IRQ_CONNECT(DT_IRQN(child), DT_IRQ(child, priority), ti_sci_intr_demux,                    \
		    &ti_sci_intr_outs[TI_SCI_INTR_OUT_IDX(child)], DT_IRQ(child, flags))

#define TI_SCI_INTR_OUT_CONNECT_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, TI_SCI_INTR_OUT_CONNECT, (;))

static int ti_sci_intr_mli_setup(void)
{
	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_CONNECT_INST);

	return 0;
}

SYS_INIT(ti_sci_intr_mli_setup, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY);

#endif /* CONFIG_MULTI_LEVEL_INTERRUPTS && ti_sci_intr_out */

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

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	{
		struct ti_sci_intr_out *out = ti_sci_intr_out_by_output(dev, output);
		unsigned int ml_irq;

		if (out == NULL) {
			key = k_spin_lock(&data->lock);
			ti_sci_intr_clear_route(data, route);
			k_spin_unlock(&data->lock, key);
			return -ENODEV;
		}

		/*
		 * Demux owns the VIM ISR. Install the client on the L2 table and
		 * enable through the multi-level path (TISCI + VIM).
		 */
		ml_irq = irq_to_level_2(input) | parent_irq;
		ret = irq_connect_dynamic(ml_irq, priority, isr, arg, irq_flags);
		if (ret < 0) {
			key = k_spin_lock(&data->lock);
			ti_sci_intr_clear_route(data, route);
			k_spin_unlock(&data->lock, key);
			return ret;
		}

		irq_enable(ml_irq);
		LOG_DBG("MLI routed input %u -> output %u (VIM %u)", input, output, parent_irq);
		return 0;
	}
#else
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
#endif
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

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	{
		unsigned int ml_irq = irq_to_level_2(route_copy.input) | route_copy.parent_irq;

		irq_disable(ml_irq);
		z_isr_install(ml_irq, z_irq_spurious, NULL);
		ARG_UNUSED(cfg);
		ARG_UNUSED(route_idx);
		ARG_UNUSED(ret);
		return 0;
	}
#else
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
#endif
}

int ti_sci_intr_get_parent_irq(const struct device *dev, uint16_t input, unsigned int *parent_irq)
{
	struct ti_sci_intr_data *data;
	struct ti_sci_intr_route *route;
	k_spinlock_key_t key;

	if (dev == NULL || parent_irq == NULL) {
		return -EINVAL;
	}

#if defined(CONFIG_MULTI_LEVEL_INTERRUPTS) && DT_HAS_COMPAT_STATUS_OKAY(ti_sci_intr_out)
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		if (ti_sci_intr_outs[i].router == dev &&
		    ti_sci_intr_outs[i].active_input == input) {
			*parent_irq = ti_sci_intr_outs[i].vim_irq;
			return 0;
		}
	}
#endif

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
