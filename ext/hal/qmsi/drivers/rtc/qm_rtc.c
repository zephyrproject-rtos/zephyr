/*
 * Copyright (c) 2017, Intel Corporation
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
#if (HAS_SOC_CONTEXT_RETENTION)
#include "power_states.h"
#endif /* HAS_SOC_CONTEXT_RETENTION */

static void (*callback[QM_RTC_NUM])(void *data);
static void *callback_data[QM_RTC_NUM];

#ifndef UNIT_TEST
qm_rtc_reg_t *qm_rtc[QM_RTC_NUM] = {(qm_rtc_reg_t *)(QM_RTC_BASE)};
#endif /* UNIT_TEST */

QM_ISR_DECLARE(qm_rtc_0_isr)
{
	/*  Disable RTC interrupt */
	QM_RTC[QM_RTC_0]->rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_ENABLE;

#if (HAS_SOC_CONTEXT_RETENTION)
	if (QM_SCSS_GP->gps0 & QM_GPS0_POWER_STATES_MASK) {
		qm_power_soc_restore();
	}
#endif /* HAS_SOC_CONTEXT_RETENTION */
	if (callback[QM_RTC_0]) {
		(callback[QM_RTC_0])(callback_data[QM_RTC_0]);
	}

	/* Clear interrupt. */
	QM_RTC[QM_RTC_0]->rtc_eoi;
	QM_ISR_EOI(QM_IRQ_RTC_0_INT_VECTOR);
}

int qm_rtc_set_config(const qm_rtc_t rtc, const qm_rtc_config_t *const cfg)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(rtc >= QM_RTC_0, -EINVAL);
	QM_CHECK(NULL != cfg, -EINVAL);

	/* Disable the RTC before re-configuration. */
	QM_RTC[rtc]->rtc_ccr &= ~QM_RTC_CCR_ENABLE;

	QM_RTC[rtc]->rtc_clr = cfg->init_val;

	/* Clear any pending interrupts. */
	QM_RTC[rtc]->rtc_eoi;

/* Perform if the IP used has the prescaler feature. */
#if (HAS_RTC_PRESCALER)
	/* With the RTC prescaler, the minimum value that can be set is 2. */
	if (QM_RTC_MIN_PRESCALER <= cfg->prescaler) {
		/* Enable RTC prescaler in CCR. */
		QM_RTC[rtc]->rtc_ccr |= QM_RTC_PRESCLR_ENABLE;
		QM_RTC[rtc]->rtc_cpsr = BIT(cfg->prescaler);
	} else {
		/* Disable RTC prescaler in CCR. */
		QM_RTC[rtc]->rtc_ccr &= ~QM_RTC_PRESCLR_ENABLE;
	}
#else /* HAS_RTC_PRESCALER */
	clk_rtc_set_div(cfg->prescaler); /* Set RTC divider. */
#endif /* HAS_RTC_PRESCALER */

	if (cfg->alarm_en) {
		callback[rtc] = cfg->callback;
		callback_data[rtc] = cfg->callback_data;
		qm_rtc_set_alarm(rtc, cfg->alarm_val);
	} else {
		callback[rtc] = NULL;
		callback_data[rtc] = NULL;
		/* Disable RTC interrupt. */
		QM_RTC[rtc]->rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_ENABLE;
		/* Internally mask the RTC interrupt. */
		QM_RTC[rtc]->rtc_ccr |= QM_RTC_CCR_INTERRUPT_MASK;
	}

	/* Enable the RTC upon completion. */
	QM_RTC[rtc]->rtc_ccr |= QM_RTC_CCR_ENABLE;

	return 0;
}

int qm_rtc_set_alarm(const qm_rtc_t rtc, const uint32_t alarm_val)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(rtc >= QM_RTC_0, -EINVAL);

	/* Enable RTC interrupt. */
	QM_RTC[rtc]->rtc_ccr |= QM_RTC_CCR_INTERRUPT_ENABLE;
	/* Internally unmask the RTC interrupt. */
	QM_RTC[rtc]->rtc_ccr &= ~QM_RTC_CCR_INTERRUPT_MASK;

	/* Set alarm value. */
	QM_RTC[rtc]->rtc_cmr = alarm_val;

	return 0;
}

int qm_rtc_read(const qm_rtc_t rtc, uint32_t *const value)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(rtc >= QM_RTC_0, -EINVAL);
	QM_CHECK(NULL != value, -EINVAL);

	*value = QM_RTC[rtc]->rtc_ccvr;

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_rtc_save_context(const qm_rtc_t rtc, qm_rtc_context_t *const ctx)
{
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	(void)rtc;
	(void)ctx;

	return 0;
}

int qm_rtc_restore_context(const qm_rtc_t rtc,
			   const qm_rtc_context_t *const ctx)
{
	uint32_t int_rtc_mask;
	QM_CHECK(rtc < QM_RTC_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	(void)rtc;
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
	int_rtc_mask = QM_INTERRUPT_ROUTER->rtc_0_int_mask;
	QM_INTERRUPT_ROUTER->rtc_0_int_mask = 0xFFFFFFFF;
	QM_INTERRUPT_ROUTER->rtc_0_int_mask = int_rtc_mask;

	return 0;
}
#else
int qm_rtc_save_context(const qm_rtc_t rtc, qm_rtc_context_t *const ctx)
{
	(void)rtc;
	(void)ctx;

	return 0;
}

int qm_rtc_restore_context(const qm_rtc_t rtc,
			   const qm_rtc_context_t *const ctx)
{
	(void)rtc;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
