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

static void (*callback[QM_RTC_NUM])(void *data);
static void *callback_data[QM_RTC_NUM];

QM_ISR_DECLARE(qm_rtc_isr_0)
{
	/*  Disable RTC interrupt */
	QM_RTC[QM_RTC_0].rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_ENABLE;

#if (QUARK_D2000)
	/*
	 * If the SoC is in deep sleep mode, all the clocks are gated, if the
	 * interrupt source is cleared before the oscillators are ungated, the
	 * oscillators return to a powered down state and the SoC will not
	 * return to an active state then.
	 */
	if ((QM_SCSS_GP->gps1 & QM_SCSS_GP_POWER_STATES_MASK) ==
	    QM_SCSS_GP_POWER_STATE_DEEP_SLEEP) {
		/* Return the oscillators to an active state. */
		QM_SCSS_CCU->osc0_cfg1 &=
		    ~QM_OSC0_PD; /* power on the oscillator. */
		QM_SCSS_CCU->osc1_cfg0 &=
		    ~QM_OSC1_PD; /* power on crystal oscillator. */

		/*
		 * datasheet 9.1.2 Low Power State to Active
		 *
		 * FW to program HYB_OSC_PD_LATCH_EN = 1, RTC_OSC_PD_LATCH_EN=1
		 * so that OSC0_PD and OSC1_PD values directly control the
		 * oscillators in active state.
		 */

		QM_SCSS_CCU->ccu_lp_clk_ctl |=
		    (QM_HYB_OSC_PD_LATCH_EN | QM_RTC_OSC_PD_LATCH_EN);
	}
#endif
	if (callback[QM_RTC_0]) {
		(callback[QM_RTC_0])(callback_data[QM_RTC_0]);
	}

	/*  clear interrupt */
	QM_RTC[QM_RTC_0].rtc_eoi;
	QM_ISR_EOI(QM_IRQ_RTC_0_VECTOR);
}

int qm_rtc_set_config(const qm_rtc_t rtc, const qm_rtc_config_t *const cfg)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	/* set rtc divider */
	clk_rtc_set_div(QM_RTC_DIVIDER);

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
