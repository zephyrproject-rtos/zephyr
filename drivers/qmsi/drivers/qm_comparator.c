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

#include "qm_comparator.h"

static void (*callback)(uint32_t) = NULL;

void qm_ac_isr(void)
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
		(*callback)(int_status);
	}

	/* Clear all pending interrupts */
	QM_SCSS_CMP->cmp_stat_clr = int_status;

	QM_ISR_EOI(QM_IRQ_AC_VECTOR);
}

qm_rc_t qm_ac_get_config(qm_ac_config_t *const config)
{
	QM_CHECK(config != NULL, QM_RC_EINVAL);

	config->callback = callback;
	config->reference = QM_SCSS_CMP->cmp_ref_sel;
	config->polarity = QM_SCSS_CMP->cmp_ref_pol;
	config->power = QM_SCSS_CMP->cmp_pwr;
	config->int_en = QM_SCSS_CMP->cmp_en;

	return QM_RC_OK;
}

qm_rc_t qm_ac_set_config(const qm_ac_config_t *const config)
{
	QM_CHECK(config != NULL, QM_RC_EINVAL);

	callback = config->callback;
	QM_SCSS_CMP->cmp_ref_sel = config->reference;
	QM_SCSS_CMP->cmp_ref_pol = config->polarity;
	QM_SCSS_CMP->cmp_pwr = config->power;

	/* Clear all pending interrupts before we enable */
	QM_SCSS_CMP->cmp_stat_clr = 0x7FFFF;
	QM_SCSS_CMP->cmp_en = config->int_en;

	return QM_RC_OK;
}
