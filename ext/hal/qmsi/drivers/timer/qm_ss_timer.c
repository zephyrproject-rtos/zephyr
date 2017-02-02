/*
 * Copyright (c) 2016, Intel Corporation
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of the Intel Corporation nor the names of its
 *    contributors may be used to endorse or promote products derived from this
 *    software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE INTEL CORPORATION OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */

#include "qm_ss_timer.h"

static void (*callback[QM_SS_TIMER_NUM])(void *data);
static void *callback_data[QM_SS_TIMER_NUM];
static uint32_t qm_ss_timer_base[QM_SS_TIMER_NUM] = {QM_SS_TIMER_0_BASE};

static __inline__ void qm_ss_timer_isr(qm_ss_timer_t timer)
{
	uint32_t ctrl = 0;

	if (callback[timer]) {
		callback[timer](callback_data[timer]);
	}

	ctrl = __builtin_arc_lr(qm_ss_timer_base[timer] + QM_SS_TIMER_CONTROL);
	ctrl &= ~BIT(QM_SS_TIMER_CONTROL_INT_PENDING_OFFSET);
	__builtin_arc_sr(ctrl, qm_ss_timer_base[timer] + QM_SS_TIMER_CONTROL);
}

QM_ISR_DECLARE(qm_ss_timer_0_isr)
{
	qm_ss_timer_isr(QM_SS_TIMER_0);
}

int qm_ss_timer_set_config(const qm_ss_timer_t timer,
			   const qm_ss_timer_config_t *const cfg)
{
	uint32_t ctrl = 0;
	QM_CHECK(cfg != NULL, -EINVAL);
	QM_CHECK(timer < QM_SS_TIMER_NUM, -EINVAL);

	ctrl = cfg->watchdog_mode << QM_SS_TIMER_CONTROL_WATCHDOG_OFFSET;
	ctrl |= cfg->inc_run_only << QM_SS_TIMER_CONTROL_NON_HALTED_OFFSET;
	ctrl |= cfg->int_en << QM_SS_TIMER_CONTROL_INT_EN_OFFSET;

	__builtin_arc_sr(ctrl, qm_ss_timer_base[timer] + QM_SS_TIMER_CONTROL);
	__builtin_arc_sr(cfg->count,
			 qm_ss_timer_base[timer] + QM_SS_TIMER_LIMIT);

	callback[timer] = cfg->callback;
	callback_data[timer] = cfg->callback_data;

	return 0;
}

int qm_ss_timer_set(const qm_ss_timer_t timer, const uint32_t count)
{
	QM_CHECK(timer < QM_SS_TIMER_NUM, -EINVAL);

	__builtin_arc_sr(count, qm_ss_timer_base[timer] + QM_SS_TIMER_COUNT);

	return 0;
}

int qm_ss_timer_get(const qm_ss_timer_t timer, uint32_t *const count)
{
	QM_CHECK(timer < QM_SS_TIMER_NUM, -EINVAL);
	QM_CHECK(count != NULL, -EINVAL);

	*count = __builtin_arc_lr(qm_ss_timer_base[timer] + QM_SS_TIMER_COUNT);

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_ss_timer_save_context(const qm_ss_timer_t timer,
			     qm_ss_timer_context_t *const ctx)
{
	uint32_t controller;

	QM_CHECK(timer < QM_SS_TIMER_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	controller = qm_ss_timer_base[timer];

	ctx->timer_control = __builtin_arc_lr(controller + QM_SS_TIMER_CONTROL);
	ctx->timer_limit = __builtin_arc_lr(controller + QM_SS_TIMER_LIMIT);
	ctx->timer_count = __builtin_arc_lr(controller + QM_SS_TIMER_COUNT);

	return 0;
}

int qm_ss_timer_restore_context(const qm_ss_timer_t timer,
				const qm_ss_timer_context_t *const ctx)
{
	uint32_t controller;

	QM_CHECK(timer < QM_SS_TIMER_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	controller = qm_ss_timer_base[timer];

	__builtin_arc_sr(ctx->timer_control, controller + QM_SS_TIMER_CONTROL);
	__builtin_arc_sr(ctx->timer_limit, controller + QM_SS_TIMER_LIMIT);
	__builtin_arc_sr(ctx->timer_count, controller + QM_SS_TIMER_COUNT);

	return 0;
}
#else
int qm_ss_timer_save_context(const qm_ss_timer_t timer,
			     qm_ss_timer_context_t *const ctx)
{
	(void)timer;
	(void)ctx;

	return 0;
}

int qm_ss_timer_restore_context(const qm_ss_timer_t timer,
				const qm_ss_timer_context_t *const ctx)
{
	(void)timer;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
