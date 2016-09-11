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

#include <string.h>

#include "power_states.h"

enum power_states {
	POWER_STATE_CPU_C1,
	POWER_STATE_CPU_C2,
	POWER_STATE_CPU_C2LP,
	POWER_STATE_SOC_SLEEP,
	POWER_STATE_SOC_DEEP_SLEEP,
	POWER_STATE_MAX
};

#define TIMEOUT 5 /* in seconds */
#define MAX_SUSPEND_DEVICE_COUNT 15

static struct device *rtc_dev;
static struct device *suspend_devices[MAX_SUSPEND_DEVICE_COUNT];
static int suspend_device_count;
static enum power_states last_state = POWER_STATE_SOC_DEEP_SLEEP;

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
	case POWER_STATE_SOC_SLEEP:
		return "SOC_SLEEP";
	case POWER_STATE_SOC_DEEP_SLEEP:
		return "SOC_DEEP_SLEEP";
	default:
		return "Unknown state";
	}
}

/*
 * This helper function handle both 'sleep' and 'deep sleep' states.
 *
 * In those states, the core voltage rail is turned off and the execution
 * context is lost. The WakeUp event will turn on the core voltage rail
 * and the x86 core will jump the its reset vector.
 *
 * In order to be able to continue the program execution from the point
 * it was before entering in Sleep states, we have to save the execution
 * context and restore it during system startup.
 *
 * The execution context is restored by the 'restore_trap' routine which
 * is called during system startup by _sys_soc_resume hook.
 *
 * In future, this save and restore functionality will be provided by QMSI
 * and we won't need to do it in Zephyr side.
 */
static void __do_soc_sleep(int deep)
{
	uint64_t saved_idt = 0;
	uint64_t saved_gdt = 0;

	/* Save execution context. This routine saves 'idtr', 'gdtr',
	 * EFLAGS and general purpose registers onto the stack, and
	 * current ESP register in GPS1 register.
	 */
	__asm__ __volatile__("sidt %[idt]\n\t"
			     "sgdt %[gdt]\n\t"
			     "pushfl\n\t"
			     "pushal\n\t"
			     "movl %%esp, %[gps1]\n\t"
			     : /* Output operands. */
			     [idt] "=m"(saved_idt),
			     [gdt] "=m"(saved_gdt),
			     [gps1] "=m"(QM_SCSS_GP->gps1)
			     : /* Input operands. */
			     : /* Clobbered registers list. */
			     );

	if (deep) {
		power_soc_deep_sleep();
	} else {
		power_soc_sleep();
	}

	/* Restore trap. This routine is called during system initialization
	 * to restore the execution context from this function.
	 */
	__asm__ __volatile__(".globl restore_trap\n\t"
			     "restore_trap:\n\t"
			     "    movl %[gps1], %%esp\n\t"
			     "    movl $0x00, %[gps1]\n\t"
			     "    popal\n\t"
			     "    popfl\n\t"
			     "    lgdt %[gdt]\n\t"
			     "    lidt %[idt]\n\t"
			     : /* Output operands. */
			     : /* Input operands. */
			     [gps1] "m"(QM_SCSS_GP->gps1),
			     [idt] "m"(saved_idt),
			     [gdt] "m"(saved_gdt)
			     : /* Clobbered registers list. */
			     );
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

static void do_soc_sleep(int deep)
{
	int i, devices_retval[suspend_device_count];

	/* Host processor will be turned off so we set up an RTC
	 * interrupt to wake up the SoC.
	 */
	set_rtc_alarm();

	for (i = suspend_device_count - 1; i >= 0; i--) {
		devices_retval[i] = device_set_power_state(suspend_devices[i],
						DEVICE_PM_SUSPEND_STATE);
	}

	__do_soc_sleep(deep);

	for (i = 0; i < suspend_device_count; i++) {
		if (!devices_retval[i]) {
			device_set_power_state(suspend_devices[i],
						DEVICE_PM_ACTIVE_STATE);
		}
	}
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
	case POWER_STATE_SOC_SLEEP:
		pm_operation = SYS_PM_DEEP_SLEEP;
		do_soc_sleep(0);
		break;
	case POWER_STATE_SOC_DEEP_SLEEP:
		pm_operation = SYS_PM_DEEP_SLEEP;
		do_soc_sleep(1);
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

static void build_suspend_device_list(void)
{
	int i, devcount;
	struct device *devices;

	device_list_get(&devices, &devcount);
	if (devcount > MAX_SUSPEND_DEVICE_COUNT) {
		printk("Error: List of devices exceeds what we can track "
		       "for suspend. Built: %d, Max: %d\n",
		       devcount, MAX_SUSPEND_DEVICE_COUNT);
		return;
	}

	suspend_device_count = 3;
	for (i = 0; i < devcount; i++) {
		if (!strcmp(devices[i].config->name, "loapic")) {
			suspend_devices[0] = &devices[i];
		} else if (!strcmp(devices[i].config->name, "ioapic")) {
			suspend_devices[1] = &devices[i];
		} else if (!strcmp(devices[i].config->name,
				 CONFIG_UART_CONSOLE_ON_DEV_NAME)) {
			suspend_devices[2] = &devices[i];
		} else {
			suspend_devices[suspend_device_count++] = &devices[i];
		}
	}
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

	build_suspend_device_list();

	/* All our application does is putting the task to sleep so the kernel
	 * triggers the suspend operation.
	 */
	while (1) {
		task_sleep(SECONDS(TIMEOUT));
	}
}
