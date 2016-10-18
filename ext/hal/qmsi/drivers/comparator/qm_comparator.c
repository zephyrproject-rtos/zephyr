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

#include "qm_comparator.h"

static void (*callback)(void *, uint32_t) = NULL;
static void *callback_data;

QM_ISR_DECLARE(qm_comparator_0_isr)
{
	uint32_t int_status = QM_SCSS_CMP->cmp_stat_clr;

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
		QM_SCSS_CCU->osc0_cfg1 &= ~QM_OSC0_PD;
		QM_SCSS_CCU->osc1_cfg0 &= ~QM_OSC1_PD;

		/* HYB_OSC_PD_LATCH_EN = 1, RTC_OSC_PD_LATCH_EN=1 */
		QM_SCSS_CCU->ccu_lp_clk_ctl |=
		    (QM_HYB_OSC_PD_LATCH_EN | QM_RTC_OSC_PD_LATCH_EN);
	}
#endif
	if (callback) {
		(*callback)(callback_data, int_status);
	}

	/* Clear all pending interrupts */
	QM_SCSS_CMP->cmp_stat_clr = int_status;

	QM_ISR_EOI(QM_IRQ_COMPARATOR_0_INT_VECTOR);
}

int qm_ac_set_config(const qm_ac_config_t *const config)
{
	QM_CHECK(config != NULL, -EINVAL);

	/* Avoid interrupts while configuring the comparators.
	 * This can happen when the polarity is changed
	 * compared to a previously configured interrupt. */
	QM_SCSS_CMP->cmp_en = 0;

	callback = config->callback;
	callback_data = config->callback_data;
	QM_SCSS_CMP->cmp_ref_sel = config->reference;
	QM_SCSS_CMP->cmp_ref_pol = config->polarity;
	QM_SCSS_CMP->cmp_pwr = config->power;

	/* Clear all pending interrupts before we enable */
	QM_SCSS_CMP->cmp_stat_clr = 0x7FFFF;
	QM_SCSS_CMP->cmp_en = config->int_en;

	return 0;
}
