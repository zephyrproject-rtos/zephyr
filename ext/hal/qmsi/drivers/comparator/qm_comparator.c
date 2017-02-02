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

#if (HAS_SOC_CONTEXT_RETENTION)
#include "power_states.h"
#endif /* HAS_SOC_CONTEXT_RETENTION */

static void (*callback)(void *, uint32_t) = NULL;
static void *callback_data;

#define cmp_en cmp_en
#define cmp_ref_pol cmp_ref_pol

QM_ISR_DECLARE(qm_comparator_0_isr)
{
	uint32_t int_status = QM_SCSS_CMP->cmp_stat_clr;

#if (HAS_SOC_CONTEXT_RETENTION)
	if (QM_SCSS_GP->gps0 & QM_GPS0_POWER_STATES_MASK) {
		qm_power_soc_restore();
	}
#endif
	if (callback) {
		(*callback)(callback_data, int_status);
	}

	/* Clear all pending interrupts. */
	QM_SCSS_CMP->cmp_stat_clr = int_status;

	QM_ISR_EOI(QM_IRQ_COMPARATOR_0_INT_VECTOR);
}

int qm_ac_set_config(const qm_ac_config_t *const config)
{
	QM_CHECK(config != NULL, -EINVAL);

	uint32_t reference = 0;

	reference = config->reference;

	/*
	 * Avoid interrupts while configuring the comparators.
	 * This can happen when the polarity is changed
	 * compared to a previously configured interrupt.
	 */
	QM_SCSS_CMP->cmp_en = 0;

	callback = config->callback;
	callback_data = config->callback_data;
	QM_SCSS_CMP->cmp_ref_sel = reference;
	QM_SCSS_CMP->cmp_ref_pol = config->polarity;
	QM_SCSS_CMP->cmp_pwr = config->power;

	/* Clear all pending interrupts before we enable. */
	QM_SCSS_CMP->cmp_stat_clr = 0x7FFFF;
	QM_SCSS_CMP->cmp_en = config->cmp_en;

	return 0;
}
