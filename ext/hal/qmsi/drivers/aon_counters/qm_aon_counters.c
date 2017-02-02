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

#include "qm_aon_counters.h"

#if (HAS_SOC_CONTEXT_RETENTION)
#include "power_states.h"
#endif /* HAS_SOC_CONTEXT_RETENTION */

static void (*callback[QM_AONC_NUM])(void *) = {NULL};
static void *callback_data[QM_AONC_NUM];

#ifndef UNIT_TEST
qm_aonc_reg_t *qm_aonc[QM_AONC_NUM] = {((qm_aonc_reg_t *)QM_AONC_0_BASE),
#if (NUM_AONC_CONTROLLERS > 1)
				       ((qm_aonc_reg_t *)QM_AONC_1_BASE)
#endif /* NUM_AONC_CONTROLLERS >1 */
};
#endif /* UNIT_TEST */

#define BUSY_CHECK(_aonc)

#if (FIX_2 || FIX_3)
/* Cannot write to clear bit twice in one RTC clock cycle. */

static void wait_single_cycle(const qm_aonc_t aonc)
{
	uint32_t aonc_cfg, initial_cnt;

	/* Ensure the AON counter is enabled */
	aonc_cfg = QM_AONC[aonc]->aonc_cfg;
	QM_AONC[aonc]->aonc_cfg |= QM_AONC_ENABLE;
	initial_cnt = QM_AONC[aonc]->aonc_cnt;

	while (initial_cnt == QM_AONC[aonc]->aonc_cnt) {
	}

	QM_AONC[aonc]->aonc_cfg = aonc_cfg;
}
#endif /* (FIX_2 || FIX_3) */

#if (FIX_3)
#define CLEAR_CHECK(_aonc)                                                     \
	while (QM_AONC[(_aonc)]->aonpt_ctrl & QM_AONPT_CLR) {                  \
	}                                                                      \
	wait_single_cycle(_aonc);

#define RESET_CHECK(_aonc)                                                     \
	while (QM_AONC[(_aonc)]->aonpt_ctrl & QM_AONPT_RST) {                  \
	}                                                                      \
	wait_single_cycle(_aonc);
#else /* FIX_3 */
#define CLEAR_CHECK(_aonc)                                                     \
	while (QM_AONC[(_aonc)]->aonpt_ctrl & QM_AONPT_CLR) {                  \
	}

#define RESET_CHECK(_aonc)                                                     \
	while (QM_AONC[(_aonc)]->aonpt_ctrl & QM_AONPT_RST) {                  \
	}
#endif /* FIX_3 */

#define AONPT_CLEAR(_aonc)                                                     \
	/* Clear the alarm and wait for it to complete. */                     \
	QM_AONC[(_aonc)]->aonpt_ctrl |= QM_AONPT_CLR;                          \
	CLEAR_CHECK(_aonc)                                                     \
	BUSY_CHECK(_aonc)

/* AONPT requires one RTC clock edge before first timer reset. */
static __inline__ void pt_reset(const qm_aonc_t aonc)
{
#if (FIX_2)
	uint32_t aonc_cfg;
	static bool first_run = true;

	/*
	 * After Power on Reset, it is required to wait for one RTC clock cycle
	 * before asserting QM_AONPT_CTRL_RST.  Note the AON counter is enabled
	 * with an initial value of 0 at Power on Reset.
	 */
	if (first_run) {
		first_run = false;
		/* Ensure the AON counter is enabled */
		aonc_cfg = QM_AONC[aonc]->aonc_cfg;
		QM_AONC[aonc]->aonc_cfg |= QM_AONC_ENABLE;

		while (0 == QM_AONC[aonc]->aonc_cnt) {
		}

		QM_AONC[aonc]->aonc_cfg = aonc_cfg;
	}
#endif /* FIX_2 */

	/* Reset the counter. */
	QM_AONC[aonc]->aonpt_ctrl |= QM_AONPT_RST;
	RESET_CHECK(aonc);
	BUSY_CHECK(aonc);
}

/* AONPT requires one RTC clock edge before first timer reset. */
#define AONPT_RESET(_aonc) pt_reset((_aonc))

QM_ISR_DECLARE(qm_aonpt_0_isr)
{
	qm_aonc_t aonc;

#if (HAS_SOC_CONTEXT_RETENTION)
	if (QM_SCSS_GP->gps0 & QM_GPS0_POWER_STATES_MASK) {
		qm_power_soc_restore();
	}
#endif

	/*
	 * Check each always on counter for the interrupt status and calls
	 * the callback if it has been set.
	 */
	for (aonc = QM_AONC_0; aonc < QM_AONC_NUM; aonc++) {
		if ((QM_AONC[aonc]->aonpt_stat & QM_AONPT_INTERRUPT)) {
			if (callback[aonc]) {
				(*callback[aonc])(callback_data[aonc]);
			}
			/* Clear pending interrupt. */
			AONPT_CLEAR(aonc);
		}
	}

	QM_ISR_EOI(QM_IRQ_AONPT_0_INT_VECTOR);
}

int qm_aonc_enable(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);

	QM_AONC[aonc]->aonc_cfg = QM_AONC_ENABLE;

	return 0;
}

int qm_aonc_disable(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);

	QM_AONC[aonc]->aonc_cfg = QM_AONC_DISABLE;

	return 0;
}

int qm_aonc_get_value(const qm_aonc_t aonc, uint32_t *const val)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);
	QM_CHECK(val != NULL, -EINVAL);

	*val = QM_AONC[aonc]->aonc_cnt;
	return 0;
}

int qm_aonpt_set_config(const qm_aonc_t aonc,
			const qm_aonpt_config_t *const cfg)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	QM_AONC[aonc]->aonpt_cfg = cfg->count;

	/* Clear pending interrupts. */
	AONPT_CLEAR(aonc);

	if (cfg->int_en) {
		callback[aonc] = cfg->callback;
		callback_data[aonc] = cfg->callback_data;
	} else {
		callback[aonc] = NULL;
	}

	AONPT_RESET(aonc);

	return 0;
}

int qm_aonpt_get_value(const qm_aonc_t aonc, uint32_t *const val)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);
	QM_CHECK(val != NULL, -EINVAL);

	*val = QM_AONC[aonc]->aonpt_cnt;

	return 0;
}

int qm_aonpt_get_status(const qm_aonc_t aonc, qm_aonpt_status_t *const status)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);
	QM_CHECK(status != NULL, -EINVAL);

	if (QM_AONC[aonc]->aonpt_stat & QM_AONPT_INTERRUPT) {
		*status = QM_AONPT_EXPIRED;
	} else {
		*status = QM_AONPT_READY;
	}

	return 0;
}

int qm_aonpt_clear(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);

	/*
	 * Clear pending interrupt and poll until the command has been
	 * completed.
	 */
	AONPT_CLEAR(aonc);

	return 0;
}

int qm_aonpt_reset(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(aonc >= QM_AONC_0, -EINVAL);

	AONPT_RESET(aonc);

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_aonpt_save_context(const qm_aonc_t aonc, qm_aonc_context_t *const ctx)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	(void)aonc;
	(void)ctx;

	return 0;
}

int qm_aonpt_restore_context(const qm_aonc_t aonc,
			     const qm_aonc_context_t *const ctx)
{
	uint32_t int_aonpt_mask;
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	(void)aonc;
	(void)ctx;

	/* The interrupt router registers are sticky and retain their
	 * values across warm resets, so we don't need to save them.
	 * But for wake capable peripherals, if their interrupts are
	 * configured to be edge sensitive, the wake event will be lost
	 * by the time the interrupt controller is reconfigured, while
	 * the interrupt is still pending. By masking and unmasking again
	 * the corresponding routing register, the interrupt is forwarded
	 * to the core and the ISR will be serviced as expected.
	 */
	int_aonpt_mask = QM_INTERRUPT_ROUTER->aonpt_0_int_mask;
	QM_INTERRUPT_ROUTER->aonpt_0_int_mask = 0xFFFFFFFF;
	QM_INTERRUPT_ROUTER->aonpt_0_int_mask = int_aonpt_mask;

	return 0;
}
#else
int qm_aonpt_save_context(const qm_aonc_t aonc, qm_aonc_context_t *const ctx)
{
	(void)aonc;
	(void)ctx;

	return 0;
}

int qm_aonpt_restore_context(const qm_aonc_t aonc,
			     const qm_aonc_context_t *const ctx)
{
	(void)aonc;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
