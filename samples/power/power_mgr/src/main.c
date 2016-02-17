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

#if defined(CONFIG_STDOUT_CONSOLE)
#include <stdio.h>
#define PRINT           printf
#else
#include <misc/printk.h>
#define PRINT           printk
#endif
#include <rtc.h>

#define SLEEPTICKS	SECONDS(5)
#define P_LVL2		0xb0800504

static int pm_state; /* 1 = LPS; 2 = Tickless Idle */
static void quark_low_power(void);
static struct device *rtc_dev;
static uint32_t start_time, end_time;

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
	 * 2 = tickless idle power saving
	 *
	 */
	policy = (policy > 2 ? 0 : policy);

	return policy++;
}

static int do_low_power(int32_t ticks)
{
	PRINT("\n\nGoing to low power state!\n");

	/* Turn off peripherals/clocks here */

	quark_low_power();

	return 1; /* non-zero so kernel does not do idle wait */
}

static int do_tickless_idle(int32_t ticks)
{
	PRINT("Tickless idle power saving!\n");

	/* Turn off peripherals/clocks here */

	return 0; /* zero to let kernel do idle wait */
}

int _sys_soc_suspend(int32_t ticks)
{
	int ret = 0;

	pm_state = check_pm_policy(ticks);

	switch (pm_state) {
	case 1:
		start_time = rtc_read(rtc_dev);
		ret = do_low_power(ticks);
		break;
	case 2:
		start_time = rtc_read(rtc_dev);
		ret = do_tickless_idle(ticks);
		break;
	default:
		/* No PM operations */
		ret = 0;
		break;
	}

	return ret;
}

static void low_power_resume(void)
{
	end_time = rtc_read(rtc_dev);
	PRINT("\nResume from low power state\n");
	PRINT("Total Elapsed From Suspend To Resume = %d RTC Cycles\n",
			end_time - start_time);
}

static void tickless_idle_resume(void)
{
	end_time = rtc_read(rtc_dev);
	PRINT("\nExit from tickless idle\n");
	PRINT("Total Elapsed From Suspend To Tickless Resume = %d RTC Cycles\n",
			end_time - start_time);
}

void _sys_soc_resume(void)
{
	switch (pm_state) {
	case 1:
		low_power_resume();
		break;
	case 2:
		tickless_idle_resume();
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
