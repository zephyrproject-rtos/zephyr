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

#define TI_SCI_INTR_WORKQ_STACK_SIZE 2048

static K_KERNEL_STACK_DEFINE(ti_sci_intr_workq_stack, TI_SCI_INTR_WORKQ_STACK_SIZE);
static struct k_work_q ti_sci_intr_workq;
static bool ti_sci_intr_workq_ready;

/*
 * One host-side IR output (ti,sci-intr-out child). Peripherals name it as
 * interrupt-parent and pass the IR input in interrupts. irq_enable() programs
 * TISCI and unmasks the parent controller line (VIM, GIC, NVIC, ...) the demux
 * ISR dispatches the L2 client.
 *
 * Possible states:
 *   IDLE    - !fw_routed && !want_enable
 *   PENDING - want_enable && !fw_routed (set queued or in progress)
 *   ROUTED  - fw_routed && want_enable && inputs match (parent unmasked)
 *   DRAIN   - fw_routed && !want_enable (release queued or in progress)
 *
 * Happy path:
 *   IDLE --(irq_enable)--> PENDING --(work queued)--> ROUTED
 *
 * Teardown / rearm:
 *   PENDING --(irq_disable)--> IDLE (want cleared, so set becomes no-op)
 *   ROUTED  --(irq_disable)--> DRAIN --(release queued)--> IDLE
 *   DRAIN   --(irq_enable)-->  DRAIN then PENDING --(set queued)--> ROUTED
 *   Set failure leaves want_* set so the caller can retry
 *   Release -EBUSY is retried a few times, then re-unmasks if the same input is still wanted
 */
struct ti_sci_intr_out {
	/* Fixed from DT / IRQ_CONNECT */
	const struct device *router;
	uint16_t output;
	unsigned int parent_irq;

	/* current firmware state, i.e. route currently programmed in System Firmware */
	bool fw_routed;
	uint16_t active_input;

	/* Requested intent from the latest irq_enable / irq_disable call */
	bool want_enable;
	uint16_t want_input;
	unsigned int want_irq;

	/* Reconciles want_* to fw_* (keep TISCI only from this work) */
	struct k_work_delayable work;
	uint8_t tisci_retries;
};

#define TI_SCI_INTR_TISCI_RETRIES  3U
#define TI_SCI_INTR_TISCI_RETRY_MS 10
#define TI_SCI_INTR_READY_RETRY_MS 500

static int ti_sci_intr_queue_work(struct ti_sci_intr_out *out, k_timeout_t delay)
{
	if (!ti_sci_intr_workq_ready) {
		return -ENODEV;
	}

	return k_work_reschedule_for_queue(&ti_sci_intr_workq, &out->work, delay);
}

/* Queue work, and from a thread wait until it finishes so the route is ready */
static int ti_sci_intr_queue_work_maybe_flush(struct ti_sci_intr_out *out)
{
	struct k_work_sync sync;
	int ret;

	ret = ti_sci_intr_queue_work(out, K_NO_WAIT);
	if (ret < 0) {
		return ret;
	}

	if (!k_is_in_isr()) {
		(void)k_work_flush_delayable(&out->work, &sync);
	}

	return ret;
}

static bool ti_sci_intr_dev_init_failed(const struct device *dev)
{
	return (dev != NULL) && (dev->state != NULL) && dev->state->initialized &&
	       (dev->state->init_res != 0U);
}

static void ti_sci_intr_clear_parent_pending(unsigned int parent_irq)
{
	/*
	 * Parent stays masked here. Drop a stale edge so the next route on this
	 * output cannot dispatch the previous input's pending bit.
	 */
	k_irq_clear_pending(parent_irq);
}

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

	return tisci_cmd_rm_irq_set(cfg->sci, &req);
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
	};

#define TI_SCI_INTR_OUT_DECLARE_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_DECLARE)

#define TI_SCI_INTR_OUT_POINTER(child) &CONCAT(ti_sci_intr_out_, DT_DEP_ORD(child)),

#define TI_SCI_INTR_OUT_POINTERS_INST(n)                                                           \
	DT_INST_FOREACH_CHILD_STATUS_OKAY(n, TI_SCI_INTR_OUT_POINTER)

DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_DECLARE_INST)

static struct ti_sci_intr_out *ti_sci_intr_outs[] = {
	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_POINTERS_INST)};

void ti_sci_intr_log_routes(void)
{
	bool any = false;

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data = out->router->data;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool routed = out->fw_routed;
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
	if (!out->fw_routed || !out->want_enable || out->active_input != out->want_input) {
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

/* Look up the IR output for a level-2 IRQ, or NULL if it is not ours */
static struct ti_sci_intr_out *ti_sci_intr_lookup_l2(unsigned int irq, uint16_t *input,
						     unsigned int *parent_irq)
{
	struct ti_sci_intr_out *out;

	if (irq_get_level(irq) == 1U) {
		return NULL;
	}

	*parent_irq = irq_parent_level_2(irq);
	*input = (uint16_t)irq_from_level_2(irq);
	out = ti_sci_intr_out_by_parent(*parent_irq);
	if (out == NULL || out->router == NULL) {
		return NULL;
	}

	return out;
}

static void ti_sci_intr_clear_want_if_current(struct ti_sci_intr_out *out, unsigned int want_irq)
{
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key = k_spin_lock(&data->lock);

	if (out->want_enable && out->want_irq == want_irq) {
		out->want_enable = false;
	}
	k_spin_unlock(&data->lock, key);
}

/* Wait for router/TISCI to come up, or give up if init already failed */
static bool ti_sci_intr_work_defer_if_not_ready(struct ti_sci_intr_out *out,
						const struct ti_sci_intr_config *cfg,
						bool want_enable, uint16_t want_input,
						unsigned int want_irq)
{
	int ret;

	if (device_is_ready(out->router) && device_is_ready(cfg->sci)) {
		return false;
	}

	if (!want_enable) {
		return true;
	}

	if (ti_sci_intr_dev_init_failed(out->router) || ti_sci_intr_dev_init_failed(cfg->sci)) {
		ti_sci_intr_clear_want_if_current(out, want_irq);
		LOG_ERR("IR input %u enable aborted, router/TISCI init failed", want_input);
		return true;
	}

	ret = ti_sci_intr_queue_work(out, K_MSEC(TI_SCI_INTR_READY_RETRY_MS));
	if (ret == -ENODEV) {
		return true;
	}
	if (ret < 0) {
		ti_sci_intr_clear_want_if_current(out, want_irq);
		LOG_ERR("Failed to defer IR input %u enable (%d)", want_input, ret);
	}

	return true;
}

/* Drop the firmware route we no longer want, with a short retry on failure */
static bool ti_sci_intr_work_release(struct ti_sci_intr_out *out,
				     const struct ti_sci_intr_config *cfg,
				     struct ti_sci_intr_data *data, uint16_t active_input)
{
	bool clear_pending;
	bool same_input_rearm = false;
	k_spinlock_key_t key;
	int ret;

	ret = ti_sci_intr_release(cfg, active_input, out->output);
	if (ret != 0) {
		bool retry = false;

		LOG_ERR("TISCI irq release IR in %u -> out %u failed (%d)", active_input,
			out->output, ret);
		key = k_spin_lock(&data->lock);
		if (out->tisci_retries < TI_SCI_INTR_TISCI_RETRIES) {
			out->tisci_retries++;
			retry = true;
		} else {
			same_input_rearm = out->want_enable && out->want_input == active_input;
			if (out->fw_routed && out->active_input == active_input) {
				out->fw_routed = false;
			}
			out->tisci_retries = 0;
		}
		k_spin_unlock(&data->lock, key);
		if (retry) {
			(void)ti_sci_intr_queue_work(out, K_MSEC(TI_SCI_INTR_TISCI_RETRY_MS));
			return true;
		}
		if (!same_input_rearm) {
			ti_sci_intr_clear_parent_pending(out->parent_irq);
		}
		return false;
	}

	key = k_spin_lock(&data->lock);
	same_input_rearm = out->want_enable && out->want_input == active_input;
	clear_pending = !same_input_rearm;
	if (out->fw_routed && out->active_input == active_input) {
		out->fw_routed = false;
	}
	out->tisci_retries = 0;
	k_spin_unlock(&data->lock, key);

	/* Keep a pending edge across same-input disable/enable, clear it otherwise */
	if (clear_pending) {
		ti_sci_intr_clear_parent_pending(out->parent_irq);
	}

	return false;
}

/* Program the wanted route in firmware and unmask the parent when it sticks */
static bool ti_sci_intr_work_set(struct ti_sci_intr_out *out, const struct ti_sci_intr_config *cfg,
				 struct ti_sci_intr_data *data, uint16_t want_input)
{
	bool unmask = false;
	k_spinlock_key_t key;
	int ret;

	ret = ti_sci_intr_set(cfg, want_input, out->output);
	if (ret != 0) {
		LOG_ERR("TISCI irq set IR in %u -> out %u failed (%d)", want_input, out->output,
			ret);
		/* Drop the failed request so another input is not stuck on busy */
		key = k_spin_lock(&data->lock);
		if (out->want_enable && out->want_input == want_input) {
			out->want_enable = false;
		}
		k_spin_unlock(&data->lock, key);
		return true;
	}

	key = k_spin_lock(&data->lock);
	out->active_input = want_input;
	out->fw_routed = true;
	out->tisci_retries = 0;
	unmask = out->want_enable && out->want_input == want_input;
	k_spin_unlock(&data->lock, key);

	/* Unlock first, since unmask may run demux which takes the same lock */
	if (unmask) {
		irq_enable(out->parent_irq);
	}

	LOG_DBG("ti_sci_intr: IR in %u -> out %u (parent IRQ %u)", want_input, out->output,
		out->parent_irq);
	return false;
}

static void ti_sci_intr_work_handler(struct k_work *work)
{
	struct k_work_delayable *dwork = k_work_delayable_from_work(work);
	struct ti_sci_intr_out *out = CONTAINER_OF(dwork, struct ti_sci_intr_out, work);
	const struct ti_sci_intr_config *cfg = out->router->config;
	struct ti_sci_intr_data *data = out->router->data;
	k_spinlock_key_t key;

	for (;;) {
		bool want_enable;
		bool fw_routed;
		uint16_t want_input;
		uint16_t active_input;
		unsigned int want_irq;

		key = k_spin_lock(&data->lock);
		want_enable = out->want_enable;
		want_input = out->want_input;
		want_irq = out->want_irq;
		fw_routed = out->fw_routed;
		active_input = out->active_input;
		k_spin_unlock(&data->lock, key);

		if (!want_enable && !fw_routed) {
			return;
		}

		if (ti_sci_intr_work_defer_if_not_ready(out, cfg, want_enable, want_input,
							want_irq)) {
			return;
		}

		/*
		 * Release will run to completion whenever firmware still has a route
		 * that is no longer wanted (want_enable cleared, or different input).
		 */
		if (fw_routed && (!want_enable || active_input != want_input)) {
			if (ti_sci_intr_work_release(out, cfg, data, active_input)) {
				return;
			}
			continue;
		}

		if (want_enable && !fw_routed) {
			if (ti_sci_intr_work_set(out, cfg, data, want_input)) {
				return;
			}
			continue;
		}

		/* want_enable && fw_routed && inputs match. Ensure parent unmasked */
		key = k_spin_lock(&data->lock);
		want_enable =
			out->want_enable && out->fw_routed && out->active_input == out->want_input;
		k_spin_unlock(&data->lock, key);
		if (want_enable) {
			irq_enable(out->parent_irq);
		}
		return;
	}
}

void ti_sci_intr_irq_enable(unsigned int irq)
{
	unsigned int parent_irq;
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	k_spinlock_key_t key;
	int ret;

	out = ti_sci_intr_lookup_l2(irq, &input, &parent_irq);
	if (out == NULL) {
		if (irq_get_level(irq) > 1U) {
			LOG_ERR("No ti,sci-intr-out for parent IRQ %u", irq_parent_level_2(irq));
		}
		return;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);

	/*
	 * Another input is still requested on this output. It's better to refuse rather than
	 * tearing that route down. After disable clears want_enable, a new
	 * input may be requested while firmware drains the old route.
	 */
	if (out->want_enable && out->want_input != input) {
		k_spin_unlock(&data->lock, key);
		LOG_ERR("IR out %u busy. cannot enable input %u", out->output, input);
		return;
	}

	out->want_input = input;
	out->want_irq = irq;
	out->want_enable = true;
	out->tisci_retries = 0;

	/* Let the worker see the new want and finish any in-flight release first */
	k_spin_unlock(&data->lock, key);

	ret = ti_sci_intr_queue_work_maybe_flush(out);
	if (ret == -ENODEV) {
		/* Keep want_enable until the workqueue starts. */
		return;
	}
	if (ret < 0) {
		ti_sci_intr_clear_want_if_current(out, irq);
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
	bool need_work = false;
	int ret;

	out = ti_sci_intr_lookup_l2(irq, &input, &parent_irq);
	if (out == NULL) {
		return;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);

	if (out->want_enable && out->want_input == input) {
		out->want_enable = false;
		need_work = out->fw_routed;
	} else if (out->fw_routed && out->active_input == input) {
		need_work = true;
	} else {
		k_spin_unlock(&data->lock, key);
		return;
	}

	if (need_work) {
		out->tisci_retries = 0;
	}

	/* Mask while still holding the lock that cleared want_enable */
	irq_disable(parent_irq);
	k_spin_unlock(&data->lock, key);

	if (!need_work) {
		/* Pending set with want cleared becomes a no-op in the worker. */
		return;
	}

	ret = ti_sci_intr_queue_work(out, K_NO_WAIT);
	if (ret == -ENODEV) {
		return;
	}
	if (ret < 0) {
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

	out = ti_sci_intr_lookup_l2(irq, &input, &parent_irq);
	if (out == NULL) {
		return 0;
	}

	data = out->router->data;
	key = k_spin_lock(&data->lock);
	enabled = out->fw_routed && out->want_enable && out->active_input == input &&
		  out->want_input == input;
	k_spin_unlock(&data->lock, key);

	return enabled && irq_is_enabled(parent_irq);
}

/* Lock if this input owns the shared parent line (caller must unlock) */
static bool ti_sci_intr_lock_if_owner(unsigned int irq, struct ti_sci_intr_out **outp,
				      unsigned int *parent_irq, k_spinlock_key_t *key)
{
	uint16_t input;
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;

	out = ti_sci_intr_lookup_l2(irq, &input, parent_irq);
	if (out == NULL) {
		return false;
	}

	data = out->router->data;
	*key = k_spin_lock(&data->lock);
	if (!out->fw_routed || out->active_input != input) {
		k_spin_unlock(&data->lock, *key);
		return false;
	}

	*outp = out;
	return true;
}

void ti_sci_intr_irq_clear_pending(unsigned int irq)
{
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	unsigned int parent_irq;
	k_spinlock_key_t key;

	if (!ti_sci_intr_lock_if_owner(irq, &out, &parent_irq, &key)) {
		return;
	}
	data = out->router->data;
	k_irq_clear_pending(parent_irq);
	k_spin_unlock(&data->lock, key);
}

void ti_sci_intr_irq_mark_pending(unsigned int irq)
{
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	unsigned int parent_irq;
	k_spinlock_key_t key;

	if (!ti_sci_intr_lock_if_owner(irq, &out, &parent_irq, &key)) {
		return;
	}
	data = out->router->data;
	k_irq_set_pending(parent_irq);
	k_spin_unlock(&data->lock, key);
}

bool ti_sci_intr_irq_is_pending(unsigned int irq)
{
	struct ti_sci_intr_out *out;
	struct ti_sci_intr_data *data;
	unsigned int parent_irq;
	k_spinlock_key_t key;
	bool pending;

	if (!ti_sci_intr_lock_if_owner(irq, &out, &parent_irq, &key)) {
		return false;
	}
	data = out->router->data;
	pending = k_irq_is_pending(parent_irq);
	k_spin_unlock(&data->lock, key);
	return pending;
}

void ti_sci_intr_irq_priority_set(unsigned int irq, unsigned int prio, uint32_t flags)
{
	ARG_UNUSED(flags);

	if (irq_get_level(irq) == 1U) {
		return;
	}

	/*
	 * Leaf IR inputs share the parent VIM/GIC line's priority and trigger
	 * from the ti,sci-intr-out interrupts specifier. The L2 priority cell
	 * must be 0 (see binding), do not overwrite the parent programming.
	 */
	ARG_UNUSED(irq);
	__ASSERT(prio == 0U, "ti,sci-intr-out leaf priority must be 0");
	ARG_UNUSED(prio);
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
		active = out->fw_routed && out->active_input == input;
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
		    &CONCAT(ti_sci_intr_out_, DT_DEP_ORD(child)),                                  \
		    COND_CODE_1(DT_IRQ_HAS_CELL(child, flags), (DT_IRQ(child, flags)), (0)))

#define TI_SCI_INTR_OUT_CONNECT_INST(n)                                                            \
	DT_INST_FOREACH_CHILD_STATUS_OKAY_SEP(n, TI_SCI_INTR_OUT_CONNECT, (;))

BUILD_ASSERT(CONFIG_NUM_2ND_LEVEL_AGGREGATORS > 0,
	     "TI_SCI_INTR requires SoC/board multilevel layout "
	     "(NUM_2ND_LEVEL_AGGREGATORS, 2ND_LVL_INTR_*_OFFSET, NUM_IRQS, "
	     "and sufficient 1ST_LEVEL_INTERRUPT_BITS)");

static int ti_sci_intr_mli_setup(void)
{
	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		k_work_init_delayable(&ti_sci_intr_outs[i]->work, ti_sci_intr_work_handler);
	}

	DT_INST_FOREACH_STATUS_OKAY(TI_SCI_INTR_OUT_CONNECT_INST);

	return 0;
}

SYS_INIT(ti_sci_intr_mli_setup, PRE_KERNEL_2, CONFIG_INTC_INIT_PRIORITY);

/* Start the workqueue and flush enables/releases that arrived earlier. */
static int ti_sci_intr_start_pending_work(void)
{
	const struct k_work_queue_config cfg = {
		.name = "ti_sci_intr",
		.no_yield = false,
	};

	k_work_queue_start(&ti_sci_intr_workq, ti_sci_intr_workq_stack,
			   K_KERNEL_STACK_SIZEOF(ti_sci_intr_workq_stack),
			   CONFIG_SYSTEM_WORKQUEUE_PRIORITY, &cfg);
	ti_sci_intr_workq_ready = true;

	for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
		struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
		struct ti_sci_intr_data *data = out->router->data;
		k_spinlock_key_t key = k_spin_lock(&data->lock);
		bool need = out->want_enable || out->fw_routed;

		k_spin_unlock(&data->lock, key);

		if (need) {
			(void)ti_sci_intr_queue_work(out, K_NO_WAIT);
		}
	}

	return 0;
}

SYS_INIT(ti_sci_intr_start_pending_work, POST_KERNEL, CONFIG_TI_SCI_INTR_WORKQ_INIT_PRIORITY);

static int ti_sci_intr_init(const struct device *dev)
{
	const struct ti_sci_intr_config *cfg = dev->config;

	if (!device_is_ready(cfg->sci)) {
		LOG_ERR("TISCI device %s is not ready", cfg->sci->name);
		return -ENODEV;
	}

	if (ti_sci_intr_workq_ready) {
		for (size_t i = 0; i < ARRAY_SIZE(ti_sci_intr_outs); i++) {
			struct ti_sci_intr_out *out = ti_sci_intr_outs[i];
			struct ti_sci_intr_data *data;
			k_spinlock_key_t key;
			bool need;

			if (out->router != dev) {
				continue;
			}

			data = out->router->data;
			key = k_spin_lock(&data->lock);
			need = out->want_enable || out->fw_routed;
			k_spin_unlock(&data->lock, key);
			if (need) {
				(void)ti_sci_intr_queue_work_maybe_flush(out);
			}
		}
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
