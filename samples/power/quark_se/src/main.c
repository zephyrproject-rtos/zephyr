/*
 * Copyright (c) 2016 Intel Corporation.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <zephyr.h>
#include <power.h>
#include <misc/printk.h>
#include <rtc.h>

#include "power_states.h"

enum power_states {
	POWER_STATE_CPU_C1,
	POWER_STATE_CPU_C2,
	POWER_STATE_CPU_C2LP,
	POWER_STATE_MAX
};

#define TIMEOUT 5 /* in seconds */

static struct device *rtc_dev;
static enum power_states last_state = POWER_STATE_CPU_C2LP;

static enum power_states get_next_state(void)
{
	last_state = (last_state + 1) % POWER_STATE_MAX;
	return last_state;
}

static const char *state_to_string(int state)
{
	switch (state) {
	case POWER_STATE_CPU_C1:
		return "CPU_C1";
	case POWER_STATE_CPU_C2:
		return "CPU_C2";
	case POWER_STATE_CPU_C2LP:
		return "CPU_C2LP";
	default:
		return "Unknown state";
	}
}

static void set_rtc_alarm(void)
{
	uint32_t now = rtc_read(rtc_dev);
	uint32_t alarm = now + (RTC_ALARM_SECOND * TIMEOUT);

	rtc_set_alarm(rtc_dev, alarm);

	/* Wait a few ticks to ensure the 'Counter Match Register' was loaded
	 * with the 'alarm' value.
	 * Refer to the documentation in qm_rtc.h for more details.
	 */
	while (rtc_read(rtc_dev) < now + 5)
		;
}

int _sys_soc_suspend(int32_t ticks)
{
	int state = get_next_state();
	int pm_operation = SYS_PM_NOT_HANDLED;

	printk("Entering %s state\n", state_to_string(state));

	switch (state) {
	case POWER_STATE_CPU_C1:
		/* QMSI provides the power_cpu_c1() function to enter this
		 * state, but that just means halting the processor.
		 * Since Zephyr will do exactly that when the PMA doesn't
		 * handle suspend in any other way, there's no need to do
		 * so explicitly here.
		 */
		break;
	case POWER_STATE_CPU_C2:
		pm_operation = SYS_PM_LOW_POWER_STATE;
		/* Any interrupt works for taking the core out of plain C2,
		 * but if the ARC core is set to SS2, by going to C2 here
		 * we may enter LPSS mode, and then only a 'wake event' can
		 * bring us back, so set up the RTC to fire to be safe.
		 */
		set_rtc_alarm();
		power_cpu_c2();
		break;
	case POWER_STATE_CPU_C2LP:
		pm_operation = SYS_PM_LOW_POWER_STATE;
		/* Local APIC interrupts are not delivered in C2LP state so
		 * we set up the RTC interrupt as 'wake event'.
		 */
		set_rtc_alarm();
		power_cpu_c2lp();
		break;
	default:
		printk("State not supported\n");
		break;
	}

	last_state = state;

	if (pm_operation != SYS_PM_NOT_HANDLED) {
		/* We need to re-enable interrupts after we get out of the
		 * C2 states to ensure that they are serviced.
		 * This will eventually be moved into the kernel.
		 */
		__asm__ __volatile__ ("sti");
	}

	printk("Exiting %s state\n", state_to_string(state));

	return pm_operation;
}

void _sys_soc_resume(void)
{
}

void main(void)
{
	struct rtc_config cfg;

	printk("Quark SE: Power Management sample application\n");

	/* Configure RTC device. RTC interrupt is used as 'wake event' when we
	 * are in C2LP state.
	 */
	cfg.init_val = 0;
	cfg.alarm_enable = 0;
	cfg.alarm_val = 0;
	cfg.cb_fn = NULL;
	rtc_dev = device_get_binding(CONFIG_RTC_0_NAME);
	rtc_enable(rtc_dev);
	rtc_set_config(rtc_dev, &cfg);

	/* All our application does is putting the task to sleep so the kernel
	 * triggers the suspend operation.
	 */
	while (1) {
		task_sleep(SECONDS(TIMEOUT));
	}
}
