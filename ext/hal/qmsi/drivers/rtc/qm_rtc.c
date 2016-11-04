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

#include "qm_rtc.h"
#include "clk.h"
#include "power_states.h"

static void (*callback[QM_RTC_NUM])(void *data);
static void *callback_data[QM_RTC_NUM];

QM_ISR_DECLARE(qm_rtc_0_isr)
{
	/*  Disable RTC interrupt */
	QM_RTC[QM_RTC_0].rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_ENABLE;

#if (HAS_SOC_CONTEXT_RETENTION)
	if (QM_SCSS_GP->gps0 & QM_GPS0_POWER_STATES_MASK) {
		power_soc_restore();
	}
#endif
	if (callback[QM_RTC_0]) {
		(callback[QM_RTC_0])(callback_data[QM_RTC_0]);
	}

	/*  clear interrupt */
	QM_RTC[QM_RTC_0].rtc_eoi;
	QM_ISR_EOI(QM_IRQ_RTC_0_INT_VECTOR);
}

int qm_rtc_set_config(const qm_rtc_t rtc, const qm_rtc_config_t *const cfg)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	/* set rtc divider */
	clk_rtc_set_div(cfg->prescaler);

	QM_RTC[rtc].rtc_clr = cfg->init_val;

	/*  clear any pending interrupts */
	QM_RTC[rtc].rtc_eoi;

	callback[rtc] = cfg->callback;
	callback_data[rtc] = cfg->callback_data;

	if (cfg->alarm_en) {
		qm_rtc_set_alarm(rtc, cfg->alarm_val);
	} else {
		/*  Disable RTC interrupt */
		QM_RTC[rtc].rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_ENABLE;
	}

	return 0;
}

int qm_rtc_set_alarm(const qm_rtc_t rtc, const uint32_t alarm_val)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);

	/*  Enable RTC interrupt */
	QM_RTC[rtc].rtc_ccr |= QM_RTC_CCR_INTERRUPT_ENABLE;

	/*  set alarm val */
	QM_RTC[rtc].rtc_cmr = alarm_val;

	return 0;
}
