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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif
#include <string.h>
#include <rtc.h>

#define SLEEPTICKS	SECONDS(5)
#define P_LVL2		0xb0800504

static int pm_state; /* 1 = LPS; 2 = Device suspend only */
static void quark_low_power(void);
static struct device *rtc_dev;
static uint32_t start_time, end_time;
static void create_device_list(void);
static void suspend_devices(int pm_policy);
static void resume_devices(int pm_policy);

#define DEVICE_POLICY_MAX 10
static struct device *device_list;
static int device_count;
static char device_policy_list[DEVICE_POLICY_MAX];
static char device_retval[DEVICE_POLICY_MAX];

void main(void)
{
	struct rtc_config config;

	PRINT("Power Management Demo\n");

	config.init_val = 0;
	config.alarm_enable = 0;
	config.alarm_val = RTC_ALARM_SECOND;
	config.cb_fn = NULL;

	rtc_dev = device_get_binding(CONFIG_RTC_DRV_NAME);
	rtc_enable(rtc_dev);
	rtc_set_config(rtc_dev, &config);

	create_device_list();

	while (1) {
		task_sleep(SLEEPTICKS);
	}
}

static int check_pm_policy(int32_t ticks)
{
	static int policy;

	/*
	 * Compare time available with wake latencies and select
	 * appropriate power saving policy
	 *
	 * For the demo we will alternate between following states
	 *
	 * 0 = no power saving operation
	 * 1 = low power state
	 * 2 = device suspend only
	 * 3 = deep sleep
	 *
	 */
	policy = (policy > 3 ? 0 : policy);

	return policy++;
}

static int low_power_state_entry(int32_t ticks)
{
	PRINT("\n\nLow power state policy entry!\n");

	/* Turn off peripherals/clocks here */
	suspend_devices(SYS_PM_LOW_POWER_STATE);

	quark_low_power();

	return SYS_PM_LOW_POWER_STATE;
}

static int device_suspend_only_entry(int32_t ticks)
{
	PRINT("Device suspend only policy entry!\n");

	/* Turn off peripherals/clocks here */
	suspend_devices(SYS_PM_DEVICE_SUSPEND_ONLY);

	return SYS_PM_DEVICE_SUSPEND_ONLY;
}

int _sys_soc_suspend(int32_t ticks)
{
	int ret = SYS_PM_NOT_HANDLED;

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case 1:
		start_time = rtc_read(rtc_dev);
		ret = low_power_state_entry(ticks);
		break;
	case 2:
		start_time = rtc_read(rtc_dev);
		ret = device_suspend_only_entry(ticks);
		break;
	case 3:
		/*
		 * if the policy manager chooses to go to deep sleep, we need to
		 * check if any device is in the middle of a transaction
		 */
		if (!device_any_busy_check()) {
			/* Do deep sleep operations */
			/* break; (fall through for now) */
		}
	default:
		/* No PM operations */
		ret = SYS_PM_NOT_HANDLED;
		break;
	}

	return ret;
}

static void low_power_state_exit(void)
{
	resume_devices(SYS_PM_LOW_POWER_STATE);

	end_time = rtc_read(rtc_dev);
	PRINT("\nLow power state policy exit!\n");
	PRINT("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void device_suspend_only_exit(void)
{
	resume_devices(SYS_PM_DEVICE_SUSPEND_ONLY);

	end_time = rtc_read(rtc_dev);
	PRINT("\nDevice suspend only policy exit!\n");
	PRINT("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

void _sys_soc_resume(void)
{
	switch (pm_state) {
	case 1:
		low_power_state_exit();
		break;
	case 2:
		device_suspend_only_exit();
		break;
	default:
		break;
	}

	pm_state = 0;

}

static void quark_low_power(void)
{
	__asm__ volatile (
			"sti\n\t"
			/*
			 * Atomically enable interrupts and enter LPS.
			 *
			 * Reading P_LVL2 causes C2 transition.
			 */
			"movl (%%eax), %%eax\n\t"
			::"a"(P_LVL2));

}

static void suspend_devices(int pm_policy)
{
	int i;

	for (i = 0; i < device_count; i++) {
		int idx = device_policy_list[i];

		/* If necessary  the policy manager can check if a specific
		 * device in the policy list is busy as shown below :
		 * if(device_busy_check(&device_list[idx])) {do something}
		 */
		device_retval[i] = device_suspend(&device_list[idx], pm_policy);
	}
}

static void resume_devices(int pm_policy)
{
	int i;

	for (i = device_count - 1; i >= 0; i--) {
		if (!device_retval[i]) {
			int idx = device_policy_list[i];

			device_resume(&device_list[idx], pm_policy);
		}
	}
}

static void create_device_list(void)
{
	int count;
	int i;

	/*
	 * Create a list of devices that will be suspended.
	 * For the demo we will create a simple list containing
	 * gpio devices.
	 */
	device_list_get(&device_list, &count);

	for (i = 0; (i < count) && (device_count < DEVICE_POLICY_MAX); i++) {
		if (!strcmp(device_list[i].config->name, "GPIO_0") ||
				!strcmp(device_list[i].config->name,
					"GPIO_1")) {
			device_policy_list[device_count++] = i;
		}
	}
}
