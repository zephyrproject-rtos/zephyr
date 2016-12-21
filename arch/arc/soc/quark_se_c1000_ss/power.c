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
extern void _power_soc_deep_sleep_2(void);

static void _deep_sleep(enum power_states state)
{
	power_soc_set_ss_restore_flag();

	switch (state) {
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		_power_soc_sleep();
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
		_power_soc_deep_sleep();
		break;
	default:
		break;
	}
}
#endif

#define SLEEP_MODE_CORE_OFF (0x0)
#define SLEEP_MODE_CORE_TIMERS_RTC_OFF (0x60)
#define ENABLE_INTERRUPTS (BIT(4) | _ARC_V2_STATUS32_E(_ARC_V2_DEF_IRQ_LEVEL))

#define ARC_SS1 (SLEEP_MODE_CORE_OFF | ENABLE_INTERRUPTS)
#define ARC_SS2 (SLEEP_MODE_CORE_TIMERS_RTC_OFF | ENABLE_INTERRUPTS)

/* QMSI does not set the interrupt enable bit in the sleep operand.
 * For the time being, implement this in Zephyr.
 * This will be removed once QMSI is fixed.
 */
static void enter_arc_state(int mode)
{
	/* Enter SSx */
	__asm__ volatile("sleep %0"
			: /* No output operands. */
			: /* Input operands. */
			"r"(mode) : "memory", "cc");
}

void _sys_soc_set_power_state(enum power_states state)
{
	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		ss_power_soc_lpss_enable();
		enter_arc_state(ARC_SS2);
		break;
	case SYS_POWER_STATE_CPU_LPS_1:
		enter_arc_state(ARC_SS2);
		break;
	case SYS_POWER_STATE_CPU_LPS_2:
		enter_arc_state(ARC_SS1);
		break;
#if (defined(CONFIG_SYS_POWER_DEEP_SLEEP))
	case SYS_POWER_STATE_DEEP_SLEEP:
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		_deep_sleep(state);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		ss_power_soc_lpss_enable();
		power_soc_set_ss_restore_flag();
		_power_soc_deep_sleep_2();
		break;
#endif
	default:
		break;
	}
}

void _sys_soc_power_state_post_ops(enum power_states state)
{
	uint32_t limit;

	switch (state) {
	case SYS_POWER_STATE_CPU_LPS:
		ss_power_soc_lpss_disable();
	case SYS_POWER_STATE_CPU_LPS_1:
		/* Expire the timer as it is disabled in SS2. */
		limit = _arc_v2_aux_reg_read(_ARC_V2_TMR0_LIMIT);
		_arc_v2_aux_reg_write(_ARC_V2_TMR0_COUNT, limit - 1);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP:
	case SYS_POWER_STATE_DEEP_SLEEP_1:
		__builtin_arc_seti(0);
		break;
	case SYS_POWER_STATE_DEEP_SLEEP_2:
		ss_power_soc_lpss_disable();

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
	default:
		break;
	}
}
