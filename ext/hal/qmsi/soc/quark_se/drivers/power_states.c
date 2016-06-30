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

#include "power_states.h"
#include "vreg.h"

#if (QM_SENSOR)
#include "qm_sensor_regs.h"
#endif
#include "soc_watch.h"

void power_soc_lpss_enable()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl |= QM_SCSS_CCU_SS_LPS_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);
}

void power_soc_lpss_disable()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl &= ~QM_SCSS_CCU_SS_LPS_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);
}

void power_soc_sleep()
{
#if (QM_SENSOR)
	/* The sensor cannot be woken up with an edge triggered
	 * interrupt from the RTC.
	 * Switch to Level triggered interrupts.
	 * When waking up, the ROM will configure the RTC back to
	 * its initial settings.
	 */
	__builtin_arc_sr(QM_IRQ_RTC_0_VECTOR, QM_SS_AUX_IRQ_SELECT);
	__builtin_arc_sr(QM_SS_IRQ_LEVEL_SENSITIVE, QM_SS_AUX_IRQ_TRIGGER);
#endif

	/* Go to sleep */
	QM_SCSS_PMU->slp_cfg &= ~QM_SCSS_SLP_CFG_LPMODE_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_SLP_CFG);
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_SLEEP, 0);
	QM_SCSS_PMU->pm1c |= QM_SCSS_PM1C_SLPEN;
}

void power_soc_deep_sleep()
{
#if (QM_SENSOR)
	/* The sensor cannot be woken up with an edge triggered
	 * interrupt from the RTC.
	 * Switch to Level triggered interrupts.
	 * When waking up, the ROM will configure the RTC back to
	 * its initial settings.
	 */
	__builtin_arc_sr(QM_IRQ_RTC_0_VECTOR, QM_SS_AUX_IRQ_SELECT);
	__builtin_arc_sr(QM_SS_IRQ_LEVEL_SENSITIVE, QM_SS_AUX_IRQ_TRIGGER);
#endif

	/* Switch to linear regulators.
	 * For low power deep sleep mode, it is a requirement that the platform
	 * voltage regulators are not in switching mode.
	 */
	vreg_plat1p8_set_mode(VREG_MODE_LINEAR);
	vreg_plat3p3_set_mode(VREG_MODE_LINEAR);

	/* Enable low power sleep mode */
	QM_SCSS_PMU->slp_cfg |= QM_SCSS_SLP_CFG_LPMODE_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_SLP_CFG);
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_SLEEP, 0);
	QM_SCSS_PMU->pm1c |= QM_SCSS_PM1C_SLPEN;
}

#if (!QM_SENSOR)
void power_cpu_c1()
{
	__asm__ __volatile__("hlt");
}

void power_cpu_c2()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl &= ~QM_SCSS_CCU_C2_LP_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);

	/* Read P_LVL2 to trigger a C2 request */
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_SLEEP, 0);
	QM_SCSS_PMU->p_lvl2;
}

void power_cpu_c2lp()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl |= QM_SCSS_CCU_C2_LP_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);

	/* Read P_LVL2 to trigger a C2 request */
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_SLEEP, 0);
	QM_SCSS_PMU->p_lvl2;
}
#endif
