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

static void (*callback)(void *) = NULL;
static void *callback_data;

static void pt_reset(const qm_aonc_t aonc)
{
	static bool first_run = true;
	uint32_t aonc_cfg;

	/* After POR, it is required to wait for one RTC clock cycle before
	 * asserting QM_AONPT_CTRL_RST.  Note the AON counter is enabled with an
	 * initial value of 0 at POR.
	 */
	if (first_run) {
		first_run = false;

		/* Ensure the AON counter is enabled */
		aonc_cfg = QM_AONC[aonc].aonc_cfg;
		QM_AONC[aonc].aonc_cfg = BIT(0);

		while (0 == QM_AONC[aonc].aonc_cnt) {
		}

		QM_AONC[aonc].aonc_cfg = aonc_cfg;
	}

	QM_AONC[aonc].aonpt_ctrl |= BIT(1);
}

QM_ISR_DECLARE(qm_aonpt_isr_0)
{
	if (callback) {
		(*callback)(callback_data);
	}
	QM_AONC[0].aonpt_ctrl |= BIT(0); /* Clear pending interrupts */
	QM_ISR_EOI(QM_IRQ_AONPT_0_VECTOR);
}

int qm_aonc_enable(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);

	QM_AONC[aonc].aonc_cfg = 0x1;

	return 0;
}

int qm_aonc_disable(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);

	QM_AONC[aonc].aonc_cfg = 0x0;

	return 0;
}

int qm_aonc_get_value(const qm_aonc_t aonc, uint32_t *const val)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(val != NULL, -EINVAL);

	*val = QM_AONC[aonc].aonc_cnt;
	return 0;
}

int qm_aonpt_set_config(const qm_aonc_t aonc,
			const qm_aonpt_config_t *const cfg)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	QM_AONC[aonc].aonpt_cfg = cfg->count;
	if (cfg->int_en) {
		callback = cfg->callback;
		callback_data = cfg->callback_data;
	} else {
		callback = NULL;
	}
	pt_reset(aonc);

	return 0;
}

int qm_aonpt_get_value(const qm_aonc_t aonc, uint32_t *const val)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(val != NULL, -EINVAL);

	*val = QM_AONC[aonc].aonpt_cnt;
	return 0;
}

int qm_aonpt_get_status(const qm_aonc_t aonc, qm_aonpt_status_t *const status)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);
	QM_CHECK(status != NULL, -EINVAL);

#if (HAS_AONPT_BUSY_BIT)
	if (QM_AON_COUNTER[aonc]->aonpt_stat & BIT(1)) {
		*status = QM_AONPT_BUSY;
	} else
#endif
	    if (QM_AONC[aonc].aonpt_stat & BIT(0)) {
		*status = QM_AONPT_EXPIRED;
	} else {
		*status = QM_AONPT_READY;
	}

	return 0;
}

int qm_aonpt_clear(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);

	QM_AONC[aonc].aonpt_ctrl |= BIT(0);

	return 0;
}

int qm_aonpt_reset(const qm_aonc_t aonc)
{
	QM_CHECK(aonc < QM_AONC_NUM, -EINVAL);

	pt_reset(aonc);

	return 0;
}
