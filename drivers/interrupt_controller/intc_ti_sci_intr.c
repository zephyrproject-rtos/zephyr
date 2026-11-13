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
#include <zephyr/irq.h>
#include <zephyr/irq_multilevel.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sw_isr_table.h>
#include <zephyr/sys/util.h>

#include "sw_isr_common.h"

LOG_MODULE_REGISTER(ti_sci_intr, CONFIG_INTC_LOG_LEVEL);

#if !defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY) && !defined(CONFIG_GEN_SW_ISR_TABLE_SWITCH)
#error "ti_sci_intr requires CONFIG_GEN_SW_ISR_TABLE_ARRAY or CONFIG_GEN_SW_ISR_TABLE_SWITCH"
#endif

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
 * TISCI and unmasks the parent controller line (VIM, GIC, NVIC, ...) the demux
 * ISR dispatches the L2 client.
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
	unsigned int parent_irq;
	uint32_t parent_flags;
	uint16_t active_input;
	unsigned int pending_irq;
	uint32_t generation;
	struct k_work_delayable pending_work;
	enum ti_sci_intr_out_state state;
	bool rearm_after_release;
};

static int ti_sci_intr_set(const struct ti_sci_intr_config *cfg, uint16_t input, uint16_t output)
{
	struct tisci_irq_set_req req = {
		.valid_params =
			TISCI_MSG_VALUE_RM_DST_ID_VALID | TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};
	struct tisci_irq_release_req rel = {
		.valid_params =
			TISCI_MSG_VALUE_RM_DST_ID_VALID | TISCI_MSG_VALUE_RM_DST_HOST_IRQ_VALID,
		.src_id = cfg->sci_dev_id,
		.src_index = input,
		.dst_id = cfg->sci_dev_id,
		.dst_host_irq = output,
		.secondary_host = TISCI_IRQ_SECONDARY_HOST_INVALID,
	};
	int ret;

	/* Prefer set; on failure release this input→output mapping and retry once. */
	ret = tisci_cmd_rm_irq_set(cfg->sci, &req);
	if (ret != 0) {
		(void)tisci_cmd_rm_irq_release(cfg->sci, &rel);
		ret = tisci_cmd_rm_irq_set(cfg->sci, &req);
	}

	return ret;
}

static int ti_sci_intr_release(const struct ti_sci_intr_config *cfg, uint16_t input,
			       uint16_t output)
{
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
	static struct ti_sci_intr_out CONCAT(ti_sci_intr_out_, DT_DEP_ORD(child)) = {              \
		.router = DEVICE_DT_GET(DT_PARENT(child)),                                         \
		.output = DT_REG_ADDR(child),                                                      \
		.parent_irq = DT_IRQ(child, irq),                                                  \
		.parent_flags = DT_IRQ(child, flags),                                              \
		.state = TI_SCI_INTR_OUT_IDLE,                                                     \
	};

#define TI_SCI_INTR_OUT_DECLARE_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_DECLARE)

#define TI_SCI_INTR_OUT_POINTER(child) &CONCAT(ti_sci_intr_out_, DT_DEP_ORD(child)),

#define TI_SCI_INTR_OUT_POINTERS_INST(n)                                                           \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_POINTER)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_DECLARE_INST)

static struct ti_sci_intr_out *ti_sci_intr_outs[] = {
	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_POINTERS_INST)};

static void ti_sci_intr_enable_work(struct ti_sci_intr_out *out, unsigned int irq,
				    uint32_t generation);
static void ti_sci_intr_release_work(struct ti_sci_intr_out *out, uint16_t input,
				     uint32_t generation);

static void ti_sci_intr_pending_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_sci_intr_out *out = CONTAINER_OF(dwork, struct ti_sci_intr_out, pending_work);
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key;
	enum ti_sci_intr_out_state state;
	unsigned int irq;
	uint16_t input;
	uint32_t generation;

	key = k_spin_lock(&data->lock);
	state = out->state;
	irq = out->pending_irq;
	input = out->active_input;
	generation = out->generation;
	k_spin_unlock(&data->lock, key);

	if (state == TI_SCI_INTR_OUT_PENDING) {
		ti_sci_intr_enable_work(out, irq, generation);
	} else if (state == TI_SCI_INTR_OUT_RELEASING) {
		ti_sci_intr_release_work(out, input, generation);
	}
}

void ti_sci_intr_log_routes(void)
{
	bool any = false;

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data = out->router->data;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool routed = out->state == TI_SCI_INTR_OUT_ROUTED;
		uint16_t input = out->active_input;

		k_spin_unlock(&data->lock, key);

		if (routed) {
			LOG_DBG("ti_sci_intr: IR in %u -> out %u (parent IRQ %u)", input,
				out->output, out->parent_irq);
			any = true;
		}
	}

	if (!any) {
		LOG_DBG("ti_sci_intr: no active IR routes");
	}
}

static void ti_sci_intr_demux(const void *arg)
{
	struct ti_sci_intr_out *out = (struct ti_sci_intr_out *)arg;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	uint32_t table_idx;
	void (*isr)(const void *arg);
	const void *isr_arg;

	if (out == NULL) {
		return;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);
	if (out->state != TI_SCI_INTR_OUT_ROUTED) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	table_idx = z_get_sw_isr_table_idx(irq_to_level_2(out->active_input) | out->parent_irq);
#if defined(CONFIG_GEN_SW_ISR_TABLE_ARRAY)
	isr = _sw_isr_table[table_idx].isr;
	isr_arg = _sw_isr_table[table_idx].arg;
#else
	{
		struct _isr_table_entry entry;

		get_isr_entry(table_idx, &entry);
		isr = entry.isr;
		isr_arg = entry.arg;
	}
#endif
	k_spin_unlock(&data->lock, key);

	isr(isr_arg);
}

static struct ti_sci_intr_out *ti_sci_intr_out_by_parent(unsigned int parent_irq)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		if (ti_sci_intr_outs[i]->parent_irq == parent_irq) {
			return ti_sci_intr_outs[i];
		}
	}

	return NULL;
}

static void ti_sci_intr_enable_work(struct ti_sci_intr_out *out, unsigned int irq,
				    uint32_t generation)
{
	unsigned int parent_irq = out->parent_irq;
	uint16_t input = (uint16_t)irq_from_level_2(irq);
	const struct ti_sci_intr_config *cfg = out->router->config;
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key;
	int ret;

	if (!device_is_ready(out->router) || !device_is_ready(cfg->sci)) {
		key = k_spin_lock(&data->lock);
		if (out->generation != generation || out->state != TI_SCI_INTR_OUT_PENDING ||
		    out->pending_irq != irq) {
			k_spin_unlock(&data->lock, key);
			return;
		}
		k_spin_unlock(&data->lock, key);

		ret = k_work_reschedule(&out->pending_work, K_MSEC(500));
		if (ret == -ENODEV) {
			/* sadly, the system workqueue has not started yet */
			return;
		}
		if (ret < 0) {
			key = k_spin_lock(&data->lock);
			if (out->generation == generation &&
			    out->state == TI_SCI_INTR_OUT_PENDING) {
				out->state = TI_SCI_INTR_OUT_IDLE;
				out->generation++;
			}
			k_spin_unlock(&data->lock, key);
			LOG_ERR("Failed to defer IR input %u enable (%d)", input, ret);
		}
		return;
	}

	key = k_spin_lock(&data->lock);
	if (out->generation != generation || out->state != TI_SCI_INTR_OUT_PENDING ||
	    out->pending_irq != irq) {
		k_spin_unlock(&data->lock, key);
		return;
	}
	out->state = TI_SCI_INTR_OUT_ROUTING;
	out->active_input = input;
	k_spin_unlock(&data->lock, key);

	ret = ti_sci_intr_set(cfg, input, out->output);
	if (ret != 0) {
		bool queue_rearm = false;
		uint32_t rearm_gen = 0;

		LOG_ERR("TISCI irq set IR in %u -> out %u failed (%d)", input, out->output, ret);
		key = k_spin_lock(&data->lock);
		/*
		 * Disable bumps generation when moving ROUTING -> CANCELING, so
		 * CANCELING is cleared without a generation match. remember, matching
		 * ROUTING means this enable still owns the attempt.
		 */
		if ((out->generation == generation && out->state == TI_SCI_INTR_OUT_ROUTING) ||
		    out->state == TI_SCI_INTR_OUT_CANCELING) {
			if (out->rearm_after_release) {
				out->rearm_after_release = false;
				out->active_input = (uint16_t)irq_from_level_2(out->pending_irq);
				out->state = TI_SCI_INTR_OUT_PENDING;
				out->generation++;
				rearm_gen = out->generation;
				queue_rearm = true;
			} else {
				out->state = TI_SCI_INTR_OUT_IDLE;
			}
		}
		k_spin_unlock(&data->lock, key);
		if (queue_rearm) {
			(void)k_work_reschedule(&out->pending_work, K_NO_WAIT);
		}
		return;
	}

	key = k_spin_lock(&data->lock);
	if (out->generation != generation || out->state != TI_SCI_INTR_OUT_ROUTING) {
		bool queue_enable = false;

		k_spin_unlock(&data->lock, key);
		ret = ti_sci_intr_release(cfg, input, out->output);
		key = k_spin_lock(&data->lock);
		if (ret != 0) {
			out->active_input = input;
			out->state = TI_SCI_INTR_OUT_ROUTED;
			if (out->rearm_after_release &&
			    (uint16_t)irq_from_level_2(out->pending_irq) == input) {
				out->rearm_after_release = false;
				irq_enable(parent_irq);
			} else if (out->rearm_after_release) {
				out->rearm_after_release = false;
				LOG_ERR("IR out %u rearm dropped; stale route retained",
					out->output);
			}
		} else if (out->state == TI_SCI_INTR_OUT_CANCELING) {
			if (out->rearm_after_release) {
				out->rearm_after_release = false;
				out->active_input = (uint16_t)irq_from_level_2(out->pending_irq);
				out->state = TI_SCI_INTR_OUT_PENDING;
				out->generation++;
				queue_enable = true;
			} else {
				out->state = TI_SCI_INTR_OUT_IDLE;
			}
		}
		k_spin_unlock(&data->lock, key);
		if (ret != 0) {
			LOG_ERR("TISCI stale route release IR in %u -> out %u failed (%d)", input,
				out->output, ret);
		}
		if (queue_enable) {
			(void)k_work_reschedule(&out->pending_work, K_NO_WAIT);
		}
		return;
	}
	out->state = TI_SCI_INTR_OUT_ROUTED;
	irq_enable(parent_irq);
	k_spin_unlock(&data->lock, key);

	LOG_DBG("ti_sci_intr: IR in %u -> out %u (parent IRQ %u)", input, out->output, parent_irq);
}

static void ti_sci_intr_release_work(struct ti_sci_intr_out *out, uint16_t input,
				     uint32_t generation)
{
	const struct ti_sci_intr_config *cfg = out->router->config;
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key;
	bool queue_enable = false;
	int ret;

	ret = ti_sci_intr_release(cfg, input, out->output);

	key = k_spin_lock(&data->lock);
	if (out->state == TI_SCI_INTR_OUT_RELEASING && out->generation == generation) {
		if (ret == 0) {
			if (out->rearm_after_release) {
				out->rearm_after_release = false;
				out->active_input = (uint16_t)irq_from_level_2(out->pending_irq);
				out->state = TI_SCI_INTR_OUT_PENDING;
				out->generation++;
				queue_enable = true;
			} else {
				out->state = TI_SCI_INTR_OUT_IDLE;
			}
		} else if (out->rearm_after_release &&
			   (uint16_t)irq_from_level_2(out->pending_irq) == input) {
			out->rearm_after_release = false;
			out->state = TI_SCI_INTR_OUT_ROUTED;
			irq_enable(out->parent_irq);
		} else {
			/* Retain firmware route, parent stays masked. */
			out->state = TI_SCI_INTR_OUT_ROUTED;
			if (out->rearm_after_release) {
				out->rearm_after_release = false;
				LOG_ERR("IR out %u rearm dropped; release failed", out->output);
			}
		}
	}
	k_spin_unlock(&data->lock, key);

	if (queue_enable) {
		(void)k_work_reschedule(&out->pending_work, K_NO_WAIT);
	} else if (ret != 0) {
		LOG_ERR("TISCI irq release IR in %u -> out %u failed (%d). route retained", input,
			out->output, ret);
	}
}

void ti_sci_intr_irq_enable(unsigned int irq)
{
	unsigned int parent_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	uint32_t generation;
	int ret;

	/* Level-1 IRQs are handled by the SoC/arch parent controller. */
	if (irq_get_level(irq) == 1U) {
		return;
	}

	parent_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_parent(parent_irq);
	if (out == NULL || out->router == NULL) {
		LOG_ERR("No ti,sci-intr-out for parent IRQ %u", parent_irq);
		return;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);

	if (out->state == TI_SCI_INTR_OUT_ROUTED && out->active_input == input) {
		irq_enable(parent_irq);
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state == TI_SCI_INTR_OUT_RELEASING || out->state == TI_SCI_INTR_OUT_CANCELING) {
		out->pending_irq = irq;
		out->rearm_after_release = true;
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state != TI_SCI_INTR_OUT_IDLE &&
	    !(out->state == TI_SCI_INTR_OUT_PENDING && out->active_input == input)) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("IR out %u busy. cannot enable input %u", out->output, input);
		return;
	}

	out->generation++;
	generation = out->generation;
	out->active_input = input;
	out->pending_irq = irq;
	out->rearm_after_release = false;
	out->state = TI_SCI_INTR_OUT_PENDING;
	k_spin_unlock(&data->lock, key);

	ret = k_work_reschedule(&out->pending_work, K_NO_WAIT);
	if (ret == -ENODEV) {
		/* Keep PENDING until the system workqueue starts. */
		return;
	}
	if (ret < 0) {
		key = k_spin_lock(&data->lock);
		if (out->generation == generation && out->state == TI_SCI_INTR_OUT_PENDING) {
			out->state = TI_SCI_INTR_OUT_IDLE;
			out->generation++;
		}
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Failed to queue IR input %u enable (%d)", input, ret);
	}
}

void ti_sci_intr_irq_disable(unsigned int irq)
{
	unsigned int parent_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	uint32_t generation;
	bool queue_release = false;
	int ret;

	if (irq_get_level(irq) == 1U) {
		return;
	}

	parent_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_parent(parent_irq);
	if (out == NULL || out->router == NULL) {
		return;
	}

	data = out->router->data;

	key = k_spin_lock(&data->lock);
	if (out->active_input != input || out->state == TI_SCI_INTR_OUT_IDLE) {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state == TI_SCI_INTR_OUT_RELEASING || out->state == TI_SCI_INTR_OUT_CANCELING) {
		if (out->rearm_after_release &&
		    (uint16_t)irq_from_level_2(out->pending_irq) == input) {
			out->rearm_after_release = false;
		}
		k_spin_unlock(&data->lock, key);
		return;
	}

	out->generation++;
	generation = out->generation;
	out->rearm_after_release = false;

	if (out->state == TI_SCI_INTR_OUT_ROUTING) {
		out->state = TI_SCI_INTR_OUT_CANCELING;
		irq_disable(parent_irq);
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state == TI_SCI_INTR_OUT_PENDING) {
		out->state = TI_SCI_INTR_OUT_IDLE;
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (out->state == TI_SCI_INTR_OUT_ROUTED) {
		out->state = TI_SCI_INTR_OUT_RELEASING;
		queue_release = true;
	} else {
		out->state = TI_SCI_INTR_OUT_IDLE;
	}
	irq_disable(parent_irq);
	k_spin_unlock(&data->lock, key);

	if (!queue_release) {
		return;
	}

	ret = k_work_reschedule(&out->pending_work, K_NO_WAIT);
	if (ret == -ENODEV) {
		/* Keep RELEASING until the system workqueue starts. */
		return;
	}
	if (ret < 0) {
		key = k_spin_lock(&data->lock);
		if (out->state == TI_SCI_INTR_OUT_RELEASING && out->generation == generation) {
			/* Keep ROUTED so a later disable can retry the release. */
			out->state = TI_SCI_INTR_OUT_ROUTED;
		}
		k_spin_unlock(&data->lock, key);
		LOG_ERR("Failed to queue IR input %u release (%d)", input, ret);
	}
}

int ti_sci_intr_irq_is_enabled(unsigned int irq)
{
	unsigned int parent_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	bool enabled;

	if (irq_get_level(irq) == 1U) {
		return 0;
	}

	parent_irq = irq_parent_level_2(irq);
	input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_parent(parent_irq);

	if (out == NULL || out->router == NULL) {
		return 0;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);
	enabled = out->active_input == input && out->state == TI_SCI_INTR_OUT_ROUTED;
	k_spin_unlock(&data->lock, key);

	return enabled && irq_is_enabled(parent_irq);
}

void ti_sci_intr_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	unsigned int parent_irq;
	struct ti_sci_intr_out *out;
	uint32_t parent_flags = flags;

	if (irq_get_level(irq) == 1U) {
		return;
	}

	parent_irq = irq_parent_level_2(irq);
	out = ti_sci_intr_out_by_parent(parent_irq);
	if (out == NULL) {
		return;
	}

	/*
	 * If the caller did not supply trigger flags, prefer DT flags from the
	 * ti,sci-intr-out node. Always program the parent controller.
	 */
	if (parent_flags == 0U) {
		parent_flags = out->parent_flags;
	}
	z_soc_irq_priority_set(parent_irq, prio, parent_flags);
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
			*parent_irq = out->parent_irq;
			return 0;
		}
	}

	return -ENOENT;
}

/*
 * Map each output's parent IRQ to its index in CONFIG_2ND_LVL_INTR_*_OFFSET so
 * the SW ISR window matches gen_isr_tables.py
 */
#define TI_SCI_INTR_LVL2_INTR_OFFSET(i) CONFIG_2ND_LVL_INTR_0##i##_OFFSET

#define TI_SCI_INTR_AGG_INDEX_EQ(i, irq)                                                           \
	(((unsigned int)(irq) == (unsigned int)TI_SCI_INTR_LVL2_INTR_OFFSET(i)) ? (i) : 0)

#define TI_SCI_INTR_AGG_INDEX(irq)                                                                 \
	(LISTIFY(CONFIG_NUM_2ND_LEVEL_AGGREGATORS, TI_SCI_INTR_AGG_INDEX_EQ, (+), irq))

#define TI_SCI_INTR_OUT_ISR_TBL_OFFSET(child)                                                      \
	(INTC_BASE_ISR_TBL_OFFSET(child) +                                                         \
	 (TI_SCI_INTR_AGG_INDEX(DT_IRQN(child)) * CONFIG_MAX_IRQ_PER_AGGREGATOR))

#define TI_SCI_INTR_OUT_PARENT_ENTRY(child)                                                        \
	IRQ_PARENT_ENTRY_DEFINE(CONCAT(ti_sci_intr_agg_, DT_DEP_ORD(child)), NULL, DT_IRQN(child), \
				TI_SCI_INTR_OUT_ISR_TBL_OFFSET(child),                             \
				DT_INTC_GET_AGGREGATOR_LEVEL(child));

#define TI_SCI_INTR_OUT_PARENT_ENTRIES_INST(n)                                                     \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_PARENT_ENTRY)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_PARENT_ENTRIES_INST)

#define TI_SCI_INTR_OUT_CONNECT(child)                                                             \
	IRQ_CONNECT(DT_IRQN(child), DT_IRQ(child, priority), ti_sci_intr_demux,                    \
		    &CONCAT(ti_sci_intr_out_, DT_DEP_ORD(child)), DT_IRQ(child, flags))

#define TI_SCI_INTR_OUT_CONNECT_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, TI_SCI_INTR_OUT_CONNECT, (;))

BUILD_ASSERT(CONFIG_NUM_2ND_LEVEL_AGGREGATORS > 0,
	     "TI_SCI_INTR requires SoC/board multilevel layout "
	     "(NUM_2ND_LEVEL_AGGREGATORS, 2ND_LVL_INTR_*_OFFSET, NUM_IRQS, "
	     "and sufficient 1ST_LEVEL_INTERRUPT_BITS)");

static int ti_sci_intr_mli_setup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		k_work_init_delayable(&ti_sci_intr_outs[i]->pending_work,
				      ti_sci_intr_pending_work_handler);
	}

	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_CONNECT_INST);

	return 0;
}

SYS_INIT(ti_sci_intr_mli_setup, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY);

/* Submit enables/releases that arrived before the system workqueue started. */
static int ti_sci_intr_start_pending_work(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data = out->router->data;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		enum ti_sci_intr_out_state state = out->state;

		k_spin_unlock(&data->lock, key);

		if (state == TI_SCI_INTR_OUT_PENDING || state == TI_SCI_INTR_OUT_RELEASING) {
			(void)k_work_reschedule(&out->pending_work, K_NO_WAIT);
		}
	}

	return 0;
}

SYS_INIT(ti_sci_intr_start_pending_work, POST_KERNEL, CONFIG_INTC_INIT_PRIORITY);

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
