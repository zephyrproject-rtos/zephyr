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
#include <soc.h>

#include "power_states.h"

#define _REG_TIMER_ICR ((volatile u32_t *) \
			(CONFIG_LOAPIC_BASE_ADDRESS + LOAPIC_TIMER_ICR))

/* Variables used to save CPU state */
u64_t _pm_save_gdtr;
u64_t _pm_save_idtr;
u32_t _pm_save_esp;

extern void z_power_soc_sleep(void);
extern void z_power_restore_cpu_context(void);
extern void z_power_soc_deep_sleep(void);

#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES))
static u32_t  *__x86_restore_info =
	(u32_t *)CONFIG_BSP_SHARED_RESTORE_INFO_RAM_ADDR;

static void deep_sleep(enum power_states state)
{
	/*
	 * Setting resume vector inside the restore_cpu_context
	 * function since we have nothing to do before cpu context
	 * is restored. If necessary, it is possible to set the
	 * resume vector to a location where additional processing
	 * can be done before cpu context is restored and control
	 * transferred to _sys_suspend.
	 */
	qm_x86_set_resume_vector(z_power_restore_cpu_context,
				 *__x86_restore_info);

	qm_power_soc_set_x86_restore_flag();

	switch (state) {
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		z_power_soc_sleep();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		z_power_soc_deep_sleep();
		break;
	default:
		break;
	}
}
#endif

void sys_set_power_state(enum power_states state)
{
	switch (state) {
#if (defined(CONFIG_SYS_POWER_SLEEP_STATES))
	case SYS_POWER_STATE_SLEEP_1:
		qm_power_cpu_c1();
		break;
	case SYS_POWER_STATE_SLEEP_2:
		qm_power_cpu_c2();
		break;
	case SYS_POWER_STATE_SLEEP_3:
		qm_power_cpu_c2lp();
		break;
#endif
#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES))
	case SYS_POWER_STATE_DEEP_SLEEP_1:
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		deep_sleep(state);
		break;
#endif
	default:
		break;
	}
}

void _sys_pm_power_state_exit_post_ops(enum power_states state)
{
	switch (state) {
#if (defined(CONFIG_SYS_POWER_SLEEP_STATES))
	case SYS_POWER_STATE_SLEEP_3:
		*_REG_TIMER_ICR = 1U;
	case SYS_POWER_STATE_SLEEP_2:
	case SYS_POWER_STATE_SLEEP_1:
		__asm__ volatile("sti");
		break;
#endif
#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP_STATES))
	case SYS_POWER_STATE_DEEP_SLEEP_2:
#ifdef CONFIG_ARC_INIT
		z_arc_init(NULL);
#endif /* CONFIG_ARC_INIT */
		/* Fallthrough */
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		__asm__ volatile("sti");
		break;
#endif
	default:
		break;
	}
}

bool sys_power_state_is_arc_ready(void)
{
	return QM_SCSS_GP->gp0 & GP0_BIT_SLEEP_READY ? true : false;
}
