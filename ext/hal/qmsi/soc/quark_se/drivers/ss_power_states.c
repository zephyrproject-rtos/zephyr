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
#include "ss_power_states.h"
#include "qm_isr.h"
#include "qm_sensor_regs.h"
#include "soc_watch.h"
#include "qm_common.h"

/* Sensor Subsystem sleep operand definition.
 * Only a subset applies as internal sensor RTC
 * is not available.
 *
 *  OP | Core | Timers | RTC
 * 000 |    0 |      1 |   1 <-- used for SS1
 * 001 |    0 |      0 |   1
 * 010 |    0 |      1 |   0
 * 011 |    0 |      0 |   0 <-- used for SS2
 * 100 |    0 |      0 |   0
 * 101 |    0 |      0 |   0
 * 110 |    0 |      0 |   0
 * 111 |    0 |      0 |   0
 *
 * sleep opcode argument:
 *  - [7:5] : Sleep Operand
 *  - [4]   : Interrupt enable
 *  - [3:0] : Interrupt threshold value
 */

#define SLEEP_INT_EN BIT(4)
#define SLEEP_TIMER_ON (0x0)
#define SLEEP_TIMER_OFF (0x20)
#define SLEEP_TIMER_RTC_OFF (0x60)

#define SS_STATE_1_TIMER_ON (SLEEP_TIMER_ON | SLEEP_INT_EN)
#define SS_STATE_1_TIMER_OFF (SLEEP_TIMER_OFF | SLEEP_INT_EN)
#define SS_STATE_2 (SLEEP_TIMER_RTC_OFF | SLEEP_INT_EN)

void qm_ss_power_soc_lpss_enable()
{
	uint32_t creg_mst0_ctrl = 0;

	creg_mst0_ctrl = __builtin_arc_lr(QM_SS_CREG_BASE);

	/*
	 * Clock gate the sensor peripherals at CREG level.
	 * This clock gating is independent of the peripheral-specific clock
	 * gating provided in ss_clk.h .
	 */
	creg_mst0_ctrl |= (QM_SS_IO_CREG_MST0_CTRL_ADC_CLK_GATE |
			   QM_SS_IO_CREG_MST0_CTRL_I2C1_CLK_GATE |
			   QM_SS_IO_CREG_MST0_CTRL_I2C0_CLK_GATE |
			   QM_SS_IO_CREG_MST0_CTRL_SPI1_CLK_GATE |
			   QM_SS_IO_CREG_MST0_CTRL_SPI0_CLK_GATE);

	__builtin_arc_sr(creg_mst0_ctrl, QM_SS_CREG_BASE);

	QM_SCSS_CCU->ccu_lp_clk_ctl |= QM_SCSS_CCU_SS_LPS_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);
}

void qm_ss_power_soc_lpss_disable()
{
	uint32_t creg_mst0_ctrl = 0;

	creg_mst0_ctrl = __builtin_arc_lr(QM_SS_CREG_BASE);

	/*
	 * Restore clock gate of the sensor peripherals at CREG level.
	 * CREG is not used anywhere else so we can safely restore
	 * the configuration to its POR default.
	 */
	creg_mst0_ctrl &= ~(QM_SS_IO_CREG_MST0_CTRL_ADC_CLK_GATE |
			    QM_SS_IO_CREG_MST0_CTRL_I2C1_CLK_GATE |
			    QM_SS_IO_CREG_MST0_CTRL_I2C0_CLK_GATE |
			    QM_SS_IO_CREG_MST0_CTRL_SPI1_CLK_GATE |
			    QM_SS_IO_CREG_MST0_CTRL_SPI0_CLK_GATE);

	__builtin_arc_sr(creg_mst0_ctrl, QM_SS_CREG_BASE);

	QM_SCSS_CCU->ccu_lp_clk_ctl &= ~QM_SCSS_CCU_SS_LPS_EN;
	SOC_WATCH_LOG_EVENT(SOCW_EVENT_REGISTER, SOCW_REG_CCU_LP_CLK_CTL);
}

/* Enter SS1 :
 * SLEEP + sleep operand
 * __builtin_arc_sleep is not used here as it does not propagate sleep operand.
 */
void qm_ss_power_cpu_ss1(const qm_ss_power_cpu_ss1_mode_t mode)
{
	uint32_t priority;

	priority =
	    (__builtin_arc_lr(QM_SS_AUX_STATUS32) & QM_SS_STATUS32_E_MASK) >> 1;

	SOC_WATCH_LOG_EVENT(SOCW_ARC_EVENT_SS1, 0);

	/* Enter SS1 */
	switch (mode) {
	case QM_SS_POWER_CPU_SS1_TIMER_OFF:
		__asm__ __volatile__("sleep %0"
				     :
				     : "r"(SS_STATE_1_TIMER_OFF | priority)
				     : "memory", "cc");
		break;
	case QM_SS_POWER_CPU_SS1_TIMER_ON:
	default:
		__asm__ __volatile__("sleep %0"
				     :
				     : "r"(SS_STATE_1_TIMER_ON | priority)
				     : "memory", "cc");
		break;
	}
}

/* Enter SS2 :
 * SLEEP + sleep operand
 * __builtin_arc_sleep is not used here as it does not propagate sleep operand.
 */
void qm_ss_power_cpu_ss2(void)
{
	uint32_t priority;

	priority =
	    (__builtin_arc_lr(QM_SS_AUX_STATUS32) & QM_SS_STATUS32_E_MASK) >> 1;

	SOC_WATCH_LOG_EVENT(SOCW_ARC_EVENT_SS2, 0);

	/* Enter SS2 */
	__asm__ __volatile__("sleep %0"
			     :
			     : "r"(SS_STATE_2 | priority)
			     : "memory", "cc");
}

#if (ENABLE_RESTORE_CONTEXT)
extern uint32_t arc_restore_addr;
uint32_t cpu_context[33];
void qm_ss_power_soc_sleep_restore(void)
{
	/*
	 * Save sensor restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_ss_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_ss_set_resume_vector(sleep_restore_trap, arc_restore_addr);

	/* Save ARC execution context. */
	qm_ss_save_context(cpu_context);

	/* Set restore flags. */
	qm_power_soc_set_ss_restore_flag();

	/* Enter sleep. */
	qm_power_soc_sleep();

	/*
	 * Restore sensor execution context.
	 * The sensor startup code will jump to this location after waking up
	 * from sleep. The restore trap address is the label defined in the
	 * macro and the label is exposed here through the first parameter.
	 */
	qm_ss_restore_context(sleep_restore_trap, cpu_context);
}
void qm_ss_power_soc_deep_sleep_restore(void)
{
	/*
	 * Save sensor restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_ss_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_ss_set_resume_vector(deep_sleep_restore_trap, arc_restore_addr);

	/* Save ARC execution context. */
	qm_ss_save_context(cpu_context);

	/* Set restore flags. */
	qm_power_soc_set_ss_restore_flag();

	/* Enter sleep. */
	qm_power_soc_deep_sleep();

	/*
	 * Restore sensor execution context.
	 * The sensor startup code will jump to this location after waking up
	 * from sleep. The restore trap address is the label defined in the
	 * macro and the label is exposed here through the first parameter.
	 */
	qm_ss_restore_context(deep_sleep_restore_trap, cpu_context);
}

void qm_ss_power_sleep_wait(void)
{
	/*
	 * Save sensor restore trap address.
	 * The first parameter in this macro represents the label defined in
	 * the qm_ss_restore_context() macro, which is actually the restore
	 * trap address.
	 */
	qm_ss_set_resume_vector(sleep_restore_trap, arc_restore_addr);

	/* Save ARC execution context. */
	qm_ss_save_context(cpu_context);

	/* Set restore flags. */
	qm_power_soc_set_ss_restore_flag();

	/* Enter SS1 and stay in it until sleep and wake-up. */
	while (1) {
		qm_ss_power_cpu_ss1(QM_SS_POWER_CPU_SS1_TIMER_ON);
	}

	/*
	 * Restore sensor execution context.
	 * The sensor startup code will jump to this location after waking up
	 * from sleep. The restore trap address is the label defined in the
	 * macro and the label is exposed here through the first parameter.
	 */
	qm_ss_restore_context(sleep_restore_trap, cpu_context);
}

void qm_power_soc_set_ss_restore_flag(void)
{
	QM_SCSS_GP->gps0 |= BIT(QM_GPS0_BIT_SENSOR_WAKEUP);
}

#endif /* ENABLE_RESTORE_CONTEXT */
