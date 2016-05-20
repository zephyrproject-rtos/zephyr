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

#include "qm_wdt.h"

#define QM_WDT_RELOAD_VALUE (0x76)

static void (*callback[QM_WDT_NUM])(void *data);
static void *callback_data[QM_WDT_NUM];

QM_ISR_DECLARE(qm_wdt_isr_0)
{
	if (callback[QM_WDT_0]) {
		callback[QM_WDT_0](callback_data);
	}
	QM_ISR_EOI(QM_IRQ_WDT_0_VECTOR);
}

int qm_wdt_start(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);

	QM_WDT[wdt].wdt_cr |= QM_WDT_ENABLE;

#if (QUARK_SE)
	/* Enable the peripheral clock. */
	QM_SCSS_CCU->ccu_periph_clk_gate_ctl |= BIT(1);

#elif(QUARK_D2000)
	QM_SCSS_CCU->ccu_periph_clk_gate_ctl |= BIT(10);

#else
#error("Unsupported / unspecified processor detected.");
#endif

	QM_SCSS_PERIPHERAL->periph_cfg0 |= BIT(1);

	return 0;
}

int qm_wdt_set_config(const qm_wdt_t wdt, const qm_wdt_config_t *const cfg)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);
	QM_CHECK(cfg != NULL, -EINVAL);

	if (cfg->mode == QM_WDT_MODE_INTERRUPT_RESET) {
		callback[wdt] = cfg->callback;
		callback_data[wdt] = cfg->callback_data;
	}

	QM_WDT[wdt].wdt_cr &= ~QM_WDT_MODE;
	QM_WDT[wdt].wdt_cr |= cfg->mode << QM_WDT_MODE_OFFSET;
	QM_WDT[wdt].wdt_torr = cfg->timeout;
	/* kick the WDT to load the Timeout Period(TOP) value */
	qm_wdt_reload(wdt);

	return 0;
}

int qm_wdt_reload(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, -EINVAL);

	QM_WDT[wdt].wdt_crr = QM_WDT_RELOAD_VALUE;

	return 0;
}
