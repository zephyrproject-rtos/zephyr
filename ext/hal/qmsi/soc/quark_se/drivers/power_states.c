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

void power_soc_sleep()
{
	/* Go to sleep */
	QM_SCSS_PMU->slp_cfg &= ~QM_SCSS_SLP_CFG_LPMODE_EN;

	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_SLP_CFG);
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_SLEEP, 0);
	QM_SCSS_PMU->pm1c |= QM_SCSS_PM1C_SLPEN;
}

void power_soc_deep_sleep()
{
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

#if (ENABLE_RESTORE_CONTEXT) && (!QM_SENSOR)
/*
 * The restore trap address is stored in the variable __x86_restore_info.
 * The variable __x86_restore_info is defined in the linker script as a new
 * and independent memory segment.
 */
extern uint32_t *__x86_restore_info;
/*
 * The stack pointer is saved in the global variable sp_restore_storage
 * by qm_x86_save_context() before sleep and it is restored by
 * qm_x86_restore_context() after wake up.
 */
uint32_t sp_restore_storage;
void power_soc_sleep_restore()
{
	/*
	 * Save x86 restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_x86_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_x86_set_resume_vector(sleep_restore_trap, __x86_restore_info);

	/* Save x86 execution context. */
	qm_x86_save_context(sp_restore_storage);

	/* Set restore flags. */
	power_soc_set_x86_restore_flag();

	/* Enter sleep. */
	power_soc_sleep();

	/*
	 * Restore x86 execution context.
	 * The bootloader code will jump to this location after waking up from
	 * sleep. The restore trap address is the label defined in the macro.
	 * That label is exposed here through the first parameter.
	 */
	qm_x86_restore_context(sleep_restore_trap, sp_restore_storage);
}

void power_soc_deep_sleep_restore()
{
	/*
	 * Save x86 restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_x86_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_x86_set_resume_vector(deep_sleep_restore_trap, __x86_restore_info);

	/* Save x86 execution context. */
	qm_x86_save_context(sp_restore_storage);

	/* Set restore flags. */
	power_soc_set_x86_restore_flag();

	/* Enter sleep. */
	power_soc_deep_sleep();

	/*
	 * Restore x86 execution context.
	 * The bootloader code will jump to this location after waking up from
	 * sleep. The restore trap address is the label defined in the macro.
	 * That label is exposed here through the first parameter.
	 */
	qm_x86_restore_context(deep_sleep_restore_trap, sp_restore_storage);
}

void power_sleep_wait()
{
	/*
	 * Save x86 restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_x86_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_x86_set_resume_vector(sleep_restore_trap, __x86_restore_info);

	/* Save x86 execution context. */
	qm_x86_save_context(sp_restore_storage);

	/* Set restore flags. */
	power_soc_set_x86_restore_flag();

	/* Enter C2 and stay in it until sleep and wake-up. */
	while (1) {
		power_cpu_c2();
	}

	/*
	 * Restore x86 execution context.
	 * The bootloader code will jump to this location after waking up from
	 * sleep. The restore trap address is the label defined in the macro.
	 * That label is exposed here through the first parameter.
	 */
	qm_x86_restore_context(sleep_restore_trap, sp_restore_storage);
}

void power_soc_set_x86_restore_flag(void)
{
	QM_SCSS_GP->gps0 |= BIT(QM_GPS0_BIT_X86_WAKEUP);
}
#endif /* ENABLE_RESTORE_CONTEXT */

#if (!QM_SENSOR)
void power_cpu_c1()
{
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_HALT, 0);
	/*
	 * STI sets the IF flag. After the IF flag is set,
	 * the core begins responding to external,
	 * maskable interrupts after the next instruction is executed.
	 * When this function is called with interrupts disabled,
	 * this guarantees that an interrupt is caught only
	 * after the processor has transitioned into HLT.
	 */
	__asm__ __volatile__("sti\n\t"
			     "hlt\n\t");
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
