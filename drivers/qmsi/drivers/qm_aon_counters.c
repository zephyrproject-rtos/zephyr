/*
 * Copyright (c) 2015, Intel Corporation
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

static void (*callback)() = NULL;

static void pt_reset(const qm_scss_aon_t aonc)
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
		aonc_cfg = QM_SCSS_AON[aonc].aonc_cfg;
		QM_SCSS_AON[aonc].aonc_cfg = BIT(0);

		while (0 == QM_SCSS_AON[aonc].aonc_cnt) {
		}

		QM_SCSS_AON[aonc].aonc_cfg = aonc_cfg;
	}

	QM_SCSS_AON[aonc].aonpt_ctrl |= BIT(1);
}

void qm_aonpt_isr_0(void)
{
	if (callback) {
		(*callback)();
	}

	QM_SCSS_AON[0].aonpt_ctrl |= BIT(0); /* Clear pending interrupts */
	QM_ISR_EOI(QM_IRQ_AONPT_0_VECTOR);
}

qm_rc_t qm_aonc_enable(const qm_scss_aon_t aonc)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);

	QM_SCSS_AON[aonc].aonc_cfg = 0x1;

	return QM_RC_OK;
}

qm_rc_t qm_aonc_disable(const qm_scss_aon_t aonc)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);

	QM_SCSS_AON[aonc].aonc_cfg = 0x0;

	return QM_RC_OK;
}

uint32_t qm_aonc_get_value(const qm_scss_aon_t aonc)
{
	return QM_SCSS_AON[aonc].aonc_cnt;
}

qm_rc_t qm_aonpt_set_config(const qm_scss_aon_t aonc,
			    const qm_aonpt_config_t *const cfg)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	QM_SCSS_AON[aonc].aonpt_ctrl |= BIT(0); /* Clear pending interrupts */
	QM_SCSS_AON[aonc].aonpt_cfg = cfg->count;
	if (cfg->int_en) {
		callback = cfg->callback;
	} else {
		callback = NULL;
	}
	pt_reset(aonc);

	return QM_RC_OK;
}

qm_rc_t qm_aonpt_get_config(const qm_scss_aon_t aonc,
			    qm_aonpt_config_t *const cfg)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->count = QM_SCSS_AON[aonc].aonpt_cfg;
	if (callback == NULL) {
		cfg->int_en = false;
	} else {
		cfg->int_en = true;
	}
	cfg->callback = callback;

	return QM_RC_OK;
}

uint32_t qm_aonpt_get_value(const qm_scss_aon_t aonc)
{
	return QM_SCSS_AON[aonc].aonpt_cnt;
}

bool qm_aonpt_get_status(const qm_scss_aon_t aonc)
{
	return QM_SCSS_AON[aonc].aonpt_stat & BIT(0);
}

qm_rc_t qm_aonpt_clear(const qm_scss_aon_t aonc)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);

	QM_SCSS_AON[aonc].aonpt_ctrl |= BIT(0);

	return QM_RC_OK;
}

qm_rc_t qm_aonpt_reset(const qm_scss_aon_t aonc)
{
	QM_CHECK(aonc < QM_SCSS_AON_NUM, QM_RC_EINVAL);

	pt_reset(aonc);

	return QM_RC_OK;
}
