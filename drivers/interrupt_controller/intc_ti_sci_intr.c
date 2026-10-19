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

#define TI_SCI_INTR_INPUT_NONE UINT16_MAX

struct ti_sci_intr_config {
	const struct device *sci;
	uint16_t sci_dev_id;
};

struct ti_sci_intr_data {
	struct k_spinlock lock;
};

/*
 * One host-side IR output (ti,sci-intr-out child). Peripherals name it as
 * interrupt-parent and pass the IR input in interrupts. irq_enable() programs
 * TISCI and unmasks the parent VIM line; the demux ISR dispatches the L2 client.
 */
enum ti_sci_intr_out_state {
	TI_SCI_INTR_OUT_IDLE,
	TI_SCI_INTR_OUT_PENDING,
	TI_SCI_INTR_OUT_ROUTING,
	TI_SCI_INTR_OUT_ROUTED,
	TI_SCI_INTR_OUT_RELEASING,
	TI_SCI_INTR_OUT_CANCELING,
};

struct ti_sci_intr_out {
	const struct device *router;
	uint16_t output;
	unsigned int vim_irq;
	uint32_t vim_flags;
	uint16_t active_input;
	unsigned int pending_irq;
	uint32_t generation;
	struct k_work_delayable pending_work;
	enum ti_sci_intr_out_state state;
};

static int ti_sci_intr_program(const struct ti_sci_intr_config *cfg, uint16_t input,
			       uint16_t output, bool set)
{
	if (set) {
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

	struct tisci_irq_release_req req = {
		.valid_params =
			TISCI_MSG_VALUE_RM_DST_ID_VALID | TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};

	return tisci_cmd_rm_irq_release(cfg->sci, &req);
}

#define TI_SCI_INTR_OUT_DECLARE(child)                                                             \
	static struct ti_sci_intr_out CONCAT(ti_sci_intr_out_, DT_NODE_FULL_NAME_TOKEN(child)) = { \
		.router = DEVICE_DT_GET(DT_PARENT(child)),                                         \
		.output = DT_REG_ADDR(child),                                                      \
		.vim_irq = DT_IRQ(child, irq),                                                     \
		.vim_flags = DT_IRQ(child, flags),                                                 \
		.active_input = TI_SCI_INTR_INPUT_NONE,                                            \
		.state = TI_SCI_INTR_OUT_IDLE,                                                     \
	};

#define TI_SCI_INTR_OUT_DECLARE_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_DECLARE)

#define TI_SCI_INTR_OUT_POINTER(child) &CONCAT(ti_sci_intr_out_, DT_NODE_FULL_NAME_TOKEN(child)),

#define TI_SCI_INTR_OUT_POINTERS_INST(n)                                                           \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_POINTER)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_DECLARE_INST)

static struct ti_sci_intr_out *ti_sci_intr_outs[] = {
	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_POINTERS_INST)};

static struct k_work_delayable ti_sci_intr_route_log_work;

static void ti_sci_intr_irq_enable_inner(unsigned int irq, bool from_work, uint32_t generation);

static void ti_sci_intr_pending_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_sci_intr_out *out = CONTAINER_OF(dwork, struct ti_sci_intr_out, pending_work);
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key;
	unsigned int irq;
	uint32_t generation;

	key = k_spin_lock(&data->lock);
	if (out->state != TI_SCI_INTR_OUT_PENDING) {
		k_spin_unlock(&data->lock, key);
		return;
	}
	irq = out->pending_irq;
	generation = out->generation;
	k_spin_unlock(&data->lock, key);

	ti_sci_intr_irq_enable_inner(irq, true, generation);
}

static void ti_sci_intr_route_log_handler(struct k_work *work)
{
	ARG_UNUSED(work);

	bool any = false;

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data = out->router->data;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool routed = out->state == TI_SCI_INTR_OUT_ROUTED;
		uint16_t input = out->active_input;

		k_spin_unlock(&data->lock, key);

		if (routed) {
			LOG_DBG("ti_sci_intr: IR in %u -> out %u (VIM %u)", input, out->output,
				out->vim_irq);
			any = true;
		}
	}

	if (!any) {
		LOG_DBG("ti_sci_intr: no active IR routes");
	}
}

void ti_sci_intr_log_routes(void)
{
	ti_sci_intr_route_log_handler(NULL);
}

static void ti_sci_intr_schedule_route_log(void)
{
	(void)k_work_schedule(&ti_sci_intr_route_log_work, K_MSEC(800));
}

static void ti_sci_intr_demux(const void *arg)
{
	struct ti_sci_intr_out *out = (struct ti_sci_intr_out *)arg;
	struct ti_sci_intr_data *data;
	unsigned int zephyr_irq;
	uint16_t input;
	bool routed;
	k_spinlock_key_t key;
	uint32_t table_idx;

	if (out == NULL) {
		return;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);
	input = out->active_input;
	routed = out->state == TI_SCI_INTR_OUT_ROUTED;
	k_spin_unlock(&data->lock, key);
	if (!routed || input == TI_SCI_INTR_INPUT_NONE) {
		return;
	}

	zephyr_irq = irq_to_level_2(input) | out->vim_irq;
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
		if (ti_sci_intr_outs[i]->vim_irq == vim_irq) {
			return ti_sci_intr_outs[i];
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

	ti_sci_intr_irq_enable_inner(irq, false, 0U);
}

static void ti_sci_intr_irq_enable_inner(unsigned int irq, bool from_work, uint32_t generation)
{
	unsigned int vim_irq = irq_parent_level_2(irq);
	uint16_t input = (uint16_t)irq_from_level_2(irq);
	struct ti_sci_intr_out *out;
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	bool cancel_pending = false;
	int ret;

	out = ti_sci_intr_out_by_vim(vim_irq);
	if (out == NULL || out->router == NULL) {
		LOG_ERR("No ti,sci-intr-out for VIM %u", vim_irq);
		return;
	}

	cfg = out->router->config;
	data = out->router->data;

	if (!device_is_ready(out->router) || !device_is_ready(cfg->sci)) {
		key = k_spin_lock(&data->lock);
		if (from_work) {
			if (out->state == TI_SCI_INTR_OUT_PENDING &&
			    out->generation == generation && out->pending_irq == irq) {
				out->state = TI_SCI_INTR_OUT_IDLE;
				out->active_input = TI_SCI_INTR_INPUT_NONE;
				out->generation++;
			}
			k_spin_unlock(&data->lock, key);
			LOG_ERR("Router/TISCI not ready for IR in %u", input);
			return;
		}

		if (out->state != TI_SCI_INTR_OUT_IDLE &&
		    !(out->state == TI_SCI_INTR_OUT_PENDING && out->active_input == input)) {
			k_spin_unlock(&data->lock, key);
			LOG_ERR("IR out %u busy; cannot defer input %u", out->output, input);
			return;
		}
		out->generation++;
		out->active_input = input;
		out->pending_irq = irq;
		out->state = TI_SCI_INTR_OUT_PENDING;
		generation = out->generation;
		k_spin_unlock(&data->lock, key);
		ret = k_work_reschedule(&out->pending_work, K_MSEC(500));
		if (ret < 0) {
			key = k_spin_lock(&data->lock);
			if (out->generation == generation &&
			    out->state == TI_SCI_INTR_OUT_PENDING) {
				out->state = TI_SCI_INTR_OUT_IDLE;
				out->active_input = TI_SCI_INTR_INPUT_NONE;
				out->generation++;
			}
			k_spin_unlock(&data->lock, key);
			LOG_ERR("Failed to defer IR input %u enable (%d)", input, ret);
			return;
		}
		LOG_DBG("Deferring IR in %u enable (TISCI not ready)", input);
		return;
	}

	key = k_spin_lock(&data->lock);
	if (from_work) {
		if (out->generation != generation || out->state != TI_SCI_INTR_OUT_PENDING ||
		    out->pending_irq != irq) {
			k_spin_unlock(&data->lock, key);
			return;
		}
		out->state = TI_SCI_INTR_OUT_ROUTING;
	} else {
		if (out->state == TI_SCI_INTR_OUT_ROUTED && out->active_input == input) {
			z_vim_irq_enable(vim_irq);
			k_spin_unlock(&data->lock, key);
			return;
		}
		if (out->state != TI_SCI_INTR_OUT_IDLE &&
		    !(out->state == TI_SCI_INTR_OUT_PENDING && out->active_input == input)) {
			k_spin_unlock(&data->lock, key);
			LOG_ERR("IR out %u busy; cannot enable input %u", out->output, input);
			return;
		}
		cancel_pending = out->state == TI_SCI_INTR_OUT_PENDING;
		out->generation++;
		generation = out->generation;
		out->state = TI_SCI_INTR_OUT_ROUTING;
	}

	out->active_input = input;
	k_spin_unlock(&data->lock, key);

	if (cancel_pending) {
		(void)k_work_cancel_delayable(&out->pending_work);
	}

	ret = ti_sci_intr_program(cfg, input, out->output, true);
	if (ret != 0) {
		LOG_ERR("TISCI irq set IR in %u -> out %u failed (%d)", input, out->output, ret);
		key = k_spin_lock(&data->lock);
		if (out->generation == generation && out->state == TI_SCI_INTR_OUT_ROUTING) {
			out->state = TI_SCI_INTR_OUT_IDLE;
			out->active_input = TI_SCI_INTR_INPUT_NONE;
		} else if (out->state == TI_SCI_INTR_OUT_CANCELING) {
			out->state = TI_SCI_INTR_OUT_IDLE;
			out->active_input = TI_SCI_INTR_INPUT_NONE;
		}
		k_spin_unlock(&data->lock, key);
		return;
	}

	key = k_spin_lock(&data->lock);
	if (out->generation != generation || out->state != TI_SCI_INTR_OUT_ROUTING) {
		k_spin_unlock(&data->lock, key);
		ret = ti_sci_intr_program(cfg, input, out->output, false);
		key = k_spin_lock(&data->lock);
		if (ret != 0) {
			out->state = TI_SCI_INTR_OUT_ROUTED;
			out->active_input = input;
		} else if (out->state == TI_SCI_INTR_OUT_CANCELING) {
			out->state = TI_SCI_INTR_OUT_IDLE;
			out->active_input = TI_SCI_INTR_INPUT_NONE;
		}
		k_spin_unlock(&data->lock, key);
		if (ret != 0) {
			LOG_ERR("TISCI stale route release IR in %u -> out %u failed (%d)", input,
				out->output, ret);
		}
		return;
	}
	out->state = TI_SCI_INTR_OUT_ROUTED;
	z_vim_irq_enable(vim_irq);
	k_spin_unlock(&data->lock, key);

	LOG_DBG("ti_sci_intr: IR in %u -> out %u (VIM %u)", input, out->output, vim_irq);
	ti_sci_intr_schedule_route_log();
}

void ti_sci_intr_irq_disable(unsigned int irq)
{
	unsigned int vim_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	const struct ti_sci_intr_config *cfg;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	uint32_t generation;
	bool release_route;
	int ret;

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

	cfg = out->router->config;
	data = out->router->data;

	key = k_spin_lock(&data->lock);
	if (out->active_input != input || out->state == TI_SCI_INTR_OUT_IDLE) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state == TI_SCI_INTR_OUT_RELEASING) {
		z_vim_irq_disable(vim_irq);
		k_spin_unlock(&data->lock, key);
		return;
	}

	out->generation++;
	generation = out->generation;
	z_vim_irq_disable(vim_irq);

	if (out->state == TI_SCI_INTR_OUT_ROUTING) {
		out->state = TI_SCI_INTR_OUT_CANCELING;
		k_spin_unlock(&data->lock, key);
		return;
	}

	release_route = out->state == TI_SCI_INTR_OUT_ROUTED;
	if (release_route) {
		out->state = TI_SCI_INTR_OUT_RELEASING;
	} else {
		out->state = TI_SCI_INTR_OUT_IDLE;
		out->active_input = TI_SCI_INTR_INPUT_NONE;
	}
	k_spin_unlock(&data->lock, key);

	(void)k_work_cancel_delayable(&out->pending_work);
	if (!release_route) {
		return;
	}

	ret = ti_sci_intr_program(cfg, input, out->output, false);

	key = k_spin_lock(&data->lock);
	if (out->state == TI_SCI_INTR_OUT_RELEASING && out->generation == generation) {
		if (ret == 0) {
			out->state = TI_SCI_INTR_OUT_IDLE;
			out->active_input = TI_SCI_INTR_INPUT_NONE;
		} else {
			out->state = TI_SCI_INTR_OUT_ROUTED;
		}
	}
	k_spin_unlock(&data->lock, key);

	if (ret != 0) {
		LOG_ERR("TISCI irq release IR in %u -> out %u failed (%d); route retained", input,
			out->output, ret);
	}
}

int ti_sci_intr_irq_is_enabled(unsigned int irq)
{
	unsigned int vim_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	bool enabled;

	if (irq_get_level(irq) == 1U) {
		return z_vim_irq_is_enabled(irq);
	}

	vim_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_vim(vim_irq);

	if (out == NULL || out->router == NULL) {
		return 0;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);
	enabled = out->active_input == input && out->state == TI_SCI_INTR_OUT_ROUTED;
	k_spin_unlock(&data->lock, key);

	return enabled && z_vim_irq_is_enabled(vim_irq);
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

int ti_sci_intr_get_parent_irq(const struct device *dev, uint16_t input, unsigned int *parent_irq)
{
	if (dev == NULL || parent_irq == NULL) {
		return -EINVAL;
	}

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data;
		k_spinlock_key_t key;
		bool active;

		if (out->router != dev) {
			continue;
		}

		data = out->router->data;
		key = k_spin_lock(&data->lock);
		active = out->active_input == input && out->state == TI_SCI_INTR_OUT_ROUTED;
		k_spin_unlock(&data->lock, key);
		if (active) {
			*parent_irq = out->vim_irq;
			return 0;
		}
	}

	return -ENOENT;
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
		    &CONCAT(ti_sci_intr_out_, DT_NODE_FULL_NAME_TOKEN(child)),                     \
		    DT_IRQ(child, flags))

#define TI_SCI_INTR_OUT_CONNECT_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, TI_SCI_INTR_OUT_CONNECT, (;))

static int ti_sci_intr_mli_setup(void)
{
	k_work_init_delayable(&ti_sci_intr_route_log_work, ti_sci_intr_route_log_handler);
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		k_work_init_delayable(&ti_sci_intr_outs[i]->pending_work,
				      ti_sci_intr_pending_work_handler);
	}

	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_CONNECT_INST);

	return 0;
}

SYS_INIT(ti_sci_intr_mli_setup, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY);

static int ti_sci_intr_init(const struct device *dev)
{
	const struct ti_sci_intr_config *cfg = dev->config;

	if (!device_is_ready(cfg->sci)) {
		LOG_ERR("TISCI device %s is not ready", cfg->sci->name);
		return -ENODEV;
	}

	return 0;
}

#define TI_SCI_INTR_INIT(n)                                                                        \
	static struct ti_sci_intr_data ti_sci_intr_data_##n;                                       \
	static const struct ti_sci_intr_config ti_sci_intr_cfg_##n = {                             \
		.sci = DEVICE_DT_GET(DT_INST_PHANDLE(n, ti_sci)),                                  \
		.sci_dev_id = DT_INST_PROP(n, ti_sci_dev_id),                                      \
	};                                                                                         \
	DEVICE_DT_INST_DEFINE(n, ti_sci_intr_init, NULL, &ti_sci_intr_data_##n,                    \
			      &ti_sci_intr_cfg_##n, POST_KERNEL, CONFIG_INTC_INIT_PRIORITY, NULL)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_INIT)
