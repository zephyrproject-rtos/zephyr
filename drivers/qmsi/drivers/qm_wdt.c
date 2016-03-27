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

#include "qm_wdt.h"

#define QM_WDT_RELOAD_VALUE (0x76)

static void (*callback[QM_WDT_NUM])(void);

void qm_wdt_isr_0(void)
{
	if (callback[QM_WDT_0])
		callback[QM_WDT_0]();
	QM_ISR_EOI(QM_IRQ_WDT_0_VECTOR);
}

qm_rc_t qm_wdt_start(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, QM_RC_EINVAL);

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

	return QM_RC_OK;
}

qm_rc_t qm_wdt_set_config(const qm_wdt_t wdt, const qm_wdt_config_t *const cfg)
{
	QM_CHECK(wdt < QM_WDT_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	QM_WDT[wdt].wdt_cr &= ~QM_WDT_MODE;
	QM_WDT[wdt].wdt_cr |= cfg->mode << QM_WDT_MODE_OFFSET;
	QM_WDT[wdt].wdt_torr = cfg->timeout;
	/* kick the WDT to load the Timeout Period(TOP) value */
	qm_wdt_reload(wdt);
	callback[wdt] = cfg->callback;

	return QM_RC_OK;
}

qm_rc_t qm_wdt_get_config(const qm_wdt_t wdt, qm_wdt_config_t *const cfg)
{
	QM_CHECK(wdt < QM_WDT_NUM, QM_RC_EINVAL);
	QM_CHECK(cfg != NULL, QM_RC_EINVAL);

	cfg->timeout = QM_WDT[wdt].wdt_torr & QM_WDT_TIMEOUT_MASK;
	cfg->mode = (QM_WDT[wdt].wdt_cr & QM_WDT_MODE) >> QM_WDT_MODE_OFFSET;
	cfg->callback = callback[wdt];

	return QM_RC_OK;
}

qm_rc_t qm_wdt_reload(const qm_wdt_t wdt)
{
	QM_CHECK(wdt < QM_WDT_NUM, QM_RC_EINVAL);

	QM_WDT[wdt].wdt_crr = QM_WDT_RELOAD_VALUE;

	return QM_RC_OK;
}
