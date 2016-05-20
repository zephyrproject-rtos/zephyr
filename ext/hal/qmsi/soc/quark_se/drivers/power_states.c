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

void power_soc_lpss_enable()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl |= QM_SCSS_CCU_SS_LPS_EN;
}

void power_soc_lpss_disable()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl &= ~QM_SCSS_CCU_SS_LPS_EN;
}

void power_soc_sleep()
{
	/* Go to sleep */
	QM_SCSS_PMU->slp_cfg &= ~QM_SCSS_SLP_CFG_LPMODE_EN;
	QM_SCSS_PMU->pm1c |= QM_SCSS_PM1C_SLPEN;
}

void power_soc_deep_sleep()
{
	/* Switch to linear regulators */
	vreg_plat1p8_set_mode(VREG_MODE_LINEAR);
	vreg_plat3p3_set_mode(VREG_MODE_LINEAR);

	/* Enable low power sleep mode */
	QM_SCSS_PMU->slp_cfg |= QM_SCSS_SLP_CFG_LPMODE_EN;
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
	/* Read P_LVL2 to trigger a C2 request */
	QM_SCSS_PMU->p_lvl2;
}

void power_cpu_c2lp()
{
	QM_SCSS_CCU->ccu_lp_clk_ctl |= QM_SCSS_CCU_C2_LP_EN;
	/* Read P_LVL2 to trigger a C2 request */
	QM_SCSS_PMU->p_lvl2;
}
#endif
