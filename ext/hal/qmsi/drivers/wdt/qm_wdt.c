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

#include "qm_wdt.h"
#include "clk.h"
#include "soc_watch.h"

static void (*callback[QM_WDT_NUM])(void *data);
static void *callback_data[QM_WDT_NUM];

#ifndef UNIT_TEST
qm_wdt_reg_t *qm_wdt[QM_WDT_NUM] = {((qm_wdt_reg_t *)QM_WDT_0_BASE),
#if (NUM_WDT_CONTROLLERS > 1)
				    ((qm_wdt_reg_t *)QM_WDT_1_BASE)
#endif /* NUM_WDT_CONTROLLERS >1 */
};
#endif /* UNIT_TEST */

QM_ISR_DECLARE(qm_wdt_0_isr)
{
	if (callback[QM_WDT_0]) {
		callback[QM_WDT_0](callback_data[QM_WDT_0]);
	}
}

#if (NUM_WDT_CONTROLLERS > 1)
QM_ISR_DECLARE(qm_wdt_1_isr)
{
	if (NULL != callback[QM_WDT_1]) {
		(callback[QM_WDT_1])(callback_data[QM_WDT_1]);
	}

	/* Clear the interrupt by reading. */
	QM_WDT[QM_WDT_1]->wdt_eoi;
	QM_ISR_EOI(QM_IRQ_WDT_1_INT_VECTOR);
}
#endif /* (NUM_WDT_CONTROLLERS > 1) */

int qm_wdt_start(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);

	QM_WDT[wdt]->wdt_cr |= QM_WDT_CR_WDT_ENABLE;

#if (HAS_WDT_CLOCK_ENABLE)
	clk_periph_enable(CLK_PERIPH_WDT_REGISTER | CLK_PERIPH_CLK);
	QM_SCSS_PERIPHERAL->periph_cfg0 |= BIT(1);
#endif /* HAS_WDT_CLOCK_ENABLE */

	qm_wdt_reload(wdt);

	return 0;
}

int qm_wdt_set_config(const qm_wdt_t wdt, const qm_wdt_config_t *const cfg)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);
	QM_CHECK(cfg->timeout <= QM_WDT_TORR_TOP_MASK, -EINVAL);

	qm_wdt_reload(wdt);

	if (cfg->mode == QM_WDT_MODE_INTERRUPT_RESET) {
		callback[wdt] = cfg->callback;
		callback_data[wdt] = cfg->callback_data;
	}

	QM_WDT[wdt]->wdt_cr &= ~QM_WDT_CR_RMOD;
	QM_WDT[wdt]->wdt_cr |= cfg->mode << QM_WDT_CR_RMOD_OFFSET;

/* If the SoC has the C2 Pause enable bit for LMT WDT. */
#if (HAS_WDT_PAUSE)
	/* Make sure WDT0 is the one requested to be configured. */
	QM_CHECK(QM_WDT_0 == wdt, -EINVAL);
	(QM_SCU_AON_CRU_BLOCK)->wdt0_tclk_en &= ~C2_WDT_PAUSE_EN_MASK;
	(QM_SCU_AON_CRU_BLOCK)->wdt0_tclk_en |= cfg->pause_en
						<< C2_WDT_PAUSE_EN_SHIFT;
#endif /* HAS_WDT_PAUSE */

	/*
	 * Timeout range register. Select the timeout from the pre-defined
	 * tables. These tables can be found in the SoC databook or register
	 * file.
	 */
	QM_WDT[wdt]->wdt_torr = cfg->timeout;

	/* kick the WDT to load the Timeout Period(TOP) value */
	qm_wdt_reload(wdt);

	return 0;
}

int qm_wdt_reload(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);

	/*
	 * This register is used to restart the WDT counter. As a safety feature
	 * to prevent accidental restarts the value 0x76 must be written.
	 * A restart also clears the WDT interrupt.
	 */
	QM_WDT[wdt]->wdt_crr = QM_WDT_RELOAD_VALUE;

	return 0;
}

#if (ENABLE_RESTORE_CONTEXT)
int qm_wdt_save_context(const qm_wdt_t wdt, qm_wdt_context_t *const ctx)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	ctx->wdt_torr = QM_WDT[wdt]->wdt_torr;
	ctx->wdt_cr = QM_WDT[wdt]->wdt_cr;

	return 0;
}

int qm_wdt_restore_context(const qm_wdt_t wdt,
			   const qm_wdt_context_t *const ctx)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);
	QM_CHECK(ctx != NULL, -EINVAL);

	/*
	 * TOP_INIT field has to be written before Watchdog Timer is enabled.
	 */
	QM_WDT[wdt]->wdt_torr = ctx->wdt_torr;
	QM_WDT[wdt]->wdt_cr = ctx->wdt_cr;

	/*
	 * Reload the wdt value to avoid interrupts to fire on wake up.
	 */
	QM_WDT[wdt]->wdt_crr = QM_WDT_RELOAD_VALUE;

	return 0;
}
#else
int qm_wdt_save_context(const qm_wdt_t wdt, qm_wdt_context_t *const ctx)
{
	(void)wdt;
	(void)ctx;

	return 0;
}

int qm_wdt_restore_context(const qm_wdt_t wdt,
			   const qm_wdt_context_t *const ctx)
{
	(void)wdt;
	(void)ctx;

	return 0;
}
#endif /* ENABLE_RESTORE_CONTEXT */
