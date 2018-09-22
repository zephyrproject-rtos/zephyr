/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <zephyr.h>
#include <sys_io.h>
#include <misc/__assert.h>
#include <power.h>
#include <soc_power.h>
#include <init.h>
#include <kernel_structs.h>
#include <soc.h>

#include "power_states.h"
#include "ss_power_states.h"
#include "vreg.h"

#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP))
extern void _power_soc_sleep(void);
extern void _power_soc_deep_sleep(void);
extern void _power_soc_lpss_mode(void);

static void _deep_sleep(enum power_states state)
{
	qm_power_soc_set_ss_restore_flag();

	switch (state) {
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		_power_soc_sleep();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		_power_soc_deep_sleep();
		break;
	default:
		break;
	}
}
#endif

void _sys_soc_set_power_state(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		qm_ss_power_cpu_ss1(QM_SS_POWER_CPU_SS1_TIMER_ON);
		break;
	case SYS_POWER_STATE_CPU_LPS_1:
		qm_ss_power_cpu_ss2();
		break;
#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP))
	case SYS_POWER_STATE_DEEP_SLEEP:
		qm_ss_power_soc_lpss_enable();
		qm_power_soc_set_ss_restore_flag();
		_power_soc_lpss_mode();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_1:
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		_deep_sleep(state);
		break;
#endif
	default:
		break;
	}
}

void _sys_soc_power_state_post_ops(enum power_states state)
{
	u32_t limit;

	switch (state) {
	case SYS_POWER_STATE_CPU_LPS_1:
		/* Expire the timer as it is disabled in SS2. */
		limit = _arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
		_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, limit - 1);
	case SYS_POWER_STATE_CPU_LPS:
		__builtin_arc_seti(0);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		qm_ss_power_soc_lpss_disable();

		/* If flag is cleared it means the system entered in
		 * sleep state while we were in LPS. In that case, we
		 * must set ARC_READY flag so x86 core can continue
		 * its execution.
		 */
		if ((QM_SCSS_GP->gp0 & GP0_BIT_SLEEP_READY) == 0) {
			_quark_se_ss_ready();
			__builtin_arc_seti(0);
		} else {
			QM_SCSS_GP->gp0 &= ~GP0_BIT_SLEEP_READY;
			QM_SCSS_GP->gps0 &= ~QM_GPS0_BIT_SENSOR_WAKEUP;
		}
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_1:
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		/* Route RTC interrupt to the current core */
		QM_IR_UNMASK_INTERRUPTS(QM_INTERRUPT_ROUTER->rtc_0_int_mask);
		__builtin_arc_seti(0);
		break;
		break;
	default:
		break;
	}
}
