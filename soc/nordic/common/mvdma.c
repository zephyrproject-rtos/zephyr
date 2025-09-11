/*
 * Copyright (c) 2025 Nordic Semiconductor ASA
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <mvdma.h>
#include <zephyr/pm/policy.h>
#include <hal/nrf_cache.h>
#include <hal/nrf_mvdma.h>
#include <zephyr/cache.h>
#include <zephyr/toolchain.h>

/* To be removed when nrfx comes with those symbols. */
#define NRF_MVDMA_INT_COMPLETED0_MASK MVDMA_INTENSET_COMPLETED0_Msk
#define NRF_MVDMA_EVENT_COMPLETED0 offsetof(NRF_MVDMA_Type, EVENTS_COMPLETED[0])

#define MVDMA_DO_COUNT(node) 1 +

BUILD_ASSERT((DT_FOREACH_STATUS_OKAY(nordic_nrf_mvdma, MVDMA_DO_COUNT) 0) == 1);

#define MVDMA_NODE() DT_COMPAT_GET_ANY_STATUS_OKAY(nordic_nrf_mvdma)

static sys_slist_t list;
static atomic_t hw_err;
static struct mvdma_ctrl *curr_ctrl;
static NRF_MVDMA_Type *reg = (NRF_MVDMA_Type *)DT_REG_ADDR(MVDMA_NODE());

static uint32_t dummy_jobs[] __aligned(CONFIG_DCACHE_LINE_SIZE) = {
	NRF_MVDMA_JOB_TERMINATE,
	NRF_MVDMA_JOB_TERMINATE,
};

static void xfer_start(uint32_t src, uint32_t sink)
{
	nrf_mvdma_event_clear(reg, NRF_MVDMA_EVENT_COMPLETED0);
	nrf_mvdma_source_list_ptr_set(reg, (void *)src);
	nrf_mvdma_sink_list_ptr_set(reg, (void *)sink);
	nrf_mvdma_task_trigger(reg, NRF_MVDMA_TASK_START0);
}

static int xfer(struct mvdma_ctrl *ctrl, uint32_t src, uint32_t sink, bool queue)
{
	int rv, key;
	bool int_en = true;

	key = irq_lock();
	if (nrf_mvdma_activity_check(reg) || (curr_ctrl && curr_ctrl->handler)) {
		if (queue) {
			ctrl->source = src;
			ctrl->sink = sink;
			sys_slist_append(&list, &ctrl->node);
			rv = 1;
		} else {
			irq_unlock(key);
			return -EBUSY;
		}
	} else {
		/* There might be some pending request that need to be marked as finished. */
		if (curr_ctrl != NULL) {
			sys_snode_t *node;
			struct mvdma_ctrl *prev;

			curr_ctrl->handler = (mvdma_handler_t)1;
			while ((node = sys_slist_get(&list)) != NULL) {
				prev = CONTAINER_OF(node, struct mvdma_ctrl, node);
				prev->handler = (mvdma_handler_t)1;
			}
		}

		curr_ctrl = ctrl;
		xfer_start(src, sink);
		if (ctrl->handler == NULL) {
			int_en = false;
		}
		rv = 0;
	}
	irq_unlock(key);

	if (int_en) {
		nrf_mvdma_int_enable(reg, NRF_MVDMA_INT_COMPLETED0_MASK);
	}

	pm_policy_state_all_lock_get();

	return rv;
}

int mvdma_xfer(struct mvdma_ctrl *ctrl, struct mvdma_jobs_desc *desc, bool queue)
{
	sys_cache_data_flush_range(desc->source, desc->source_desc_size);
	sys_cache_data_flush_range(desc->sink, desc->sink_desc_size);
	return xfer(ctrl, (uint32_t)desc->source, (uint32_t)desc->sink, queue);
}

int mvdma_basic_xfer(struct mvdma_ctrl *ctrl, struct mvdma_basic_desc *desc, bool queue)
{
	sys_cache_data_flush_range(desc, sizeof(*desc));
	return xfer(ctrl, (uint32_t)&desc->source, (uint32_t)&desc->sink, queue);
}

int mvdma_xfer_check(const struct mvdma_ctrl *ctrl)
{
	if (hw_err != NRF_MVDMA_ERR_NO_ERROR) {
		return -EIO;
	}

	if (nrf_mvdma_event_check(reg, NRF_MVDMA_EVENT_COMPLETED0)) {
		curr_ctrl = NULL;
	} else if (ctrl->handler == NULL) {
		return -EBUSY;
	}

	pm_policy_state_all_lock_put();

	return 0;
}

enum mvdma_err mvdma_error_check(void)
{
	return atomic_set(&hw_err, 0);
}

static void error_handler(void)
{
	if (nrf_mvdma_event_check(reg, NRF_MVDMA_EVENT_SOURCEBUSERROR)) {
		nrf_mvdma_event_clear(reg, NRF_MVDMA_EVENT_SOURCEBUSERROR);
		hw_err = NRF_MVDMA_ERR_SOURCE;
	}

	if (nrf_mvdma_event_check(reg, NRF_MVDMA_EVENT_SINKBUSERROR)) {
		nrf_mvdma_event_clear(reg, NRF_MVDMA_EVENT_SINKBUSERROR);
		hw_err = NRF_MVDMA_ERR_SINK;
	}
}

static void ch_handler(int status)
{
	int key;
	struct mvdma_ctrl *ctrl = curr_ctrl;
	sys_snode_t *node;
	bool int_dis = true;

	key = irq_lock();
	node = sys_slist_get(&list);
	if (node) {
		struct mvdma_ctrl *next = CONTAINER_OF(node, struct mvdma_ctrl, node);

		curr_ctrl = next;
		xfer_start((uint32_t)next->source, (uint32_t)next->sink);
		if (next->handler || !sys_slist_is_empty(&list)) {
			int_dis = false;
		}
	} else {
		curr_ctrl = NULL;
	}
	if (int_dis) {
		nrf_mvdma_int_disable(reg, NRF_MVDMA_INT_COMPLETED0_MASK);
	}
	irq_unlock(key);

	if (ctrl->handler) {
		pm_policy_state_all_lock_put();
		ctrl->handler(ctrl->user_data, hw_err == NRF_MVDMA_ERR_NO_ERROR ? 0 : -EIO);
	} else {
		/* Set handler variable to non-null to indicated that transfer has finished. */
		ctrl->handler = (mvdma_handler_t)1;
	}
}

static void mvdma_isr(const void *arg)
{
	uint32_t ints = nrf_mvdma_int_pending_get(reg);

	if (ints & NRF_MVDMA_INT_COMPLETED0_MASK) {
		ch_handler(0);
	} else {
		error_handler();
	}
}

void mvdma_resume(void)
{
	/* bus errors. */
	nrf_mvdma_int_enable(reg, NRF_MVDMA_INT_SOURCEBUSERROR_MASK |
				    NRF_MVDMA_INT_SINKBUSERROR_MASK);

	/* Dummy transfer to get COMPLETED event set. */
	nrf_mvdma_source_list_ptr_set(reg, (void *)&dummy_jobs[0]);
	nrf_mvdma_sink_list_ptr_set(reg, (void *)&dummy_jobs[1]);
	nrf_mvdma_task_trigger(reg, NRF_MVDMA_TASK_START0);
}

int mvdma_init(void)
{
	sys_cache_data_flush_range(dummy_jobs, sizeof(dummy_jobs));

	sys_slist_init(&list);

	IRQ_CONNECT(DT_IRQN(MVDMA_NODE()), DT_IRQ(MVDMA_NODE(), priority), mvdma_isr, 0, 0);
	irq_enable(DT_IRQN(MVDMA_NODE()));

	mvdma_resume();

	return 0;
}
